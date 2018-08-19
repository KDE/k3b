/*
 *
 * Copyright (C) 2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2011 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3baudiotrackmodel.h"
#include "k3baudiocdtrackdrag.h"

#include "k3bmedium.h"
#include "k3bcdtext.h"

#include <KLocalizedString>

#include <KCddb/Cdinfo>

#include <QMimeData>
#include <QFont>

Q_DECLARE_METATYPE( K3b::Medium )
Q_DECLARE_METATYPE( K3b::Msf )


class K3b::AudioTrackModel::Private
{
public:
    K3b::Medium medium;
    KCDDB::CDInfo cddbCache;

    QVector<bool> itemCheckedList;

    QString getField( KCDDB::Type field ) const;
    QString getField( KCDDB::Type field, int trackIndex ) const;
};


QString K3b::AudioTrackModel::Private::getField( KCDDB::Type field ) const
{
    QString s = cddbCache.get( field ).toString();
    if ( s.isEmpty() ) {
        s = medium.cddbInfo().get( field ).toString();
    }
    if ( s.isEmpty() ) {
        switch( field ) {
        case KCDDB::Title:
            s = medium.cdText().title();
            break;
        case KCDDB::Artist:
            s = medium.cdText().performer();
            break;
        case KCDDB::Comment:
            s = medium.cdText().message();
            break;
        default:
            break;
        }
    }

    return s;
}


QString K3b::AudioTrackModel::Private::getField( KCDDB::Type field, int trackIndex ) const
{
    QString s = cddbCache.track( trackIndex ).get( field ).toString();
    if ( s.isEmpty() ) {
        s = medium.cddbInfo().track( trackIndex ).get( field ).toString();
    }
    if ( s.isEmpty() ) {
        switch( field ) {
        case KCDDB::Title:
            s = medium.cdText().track( trackIndex ).title();
            break;
        case KCDDB::Artist:
            s = medium.cdText().track( trackIndex ).performer();
            break;
        case KCDDB::Comment:
            s = medium.cdText().track( trackIndex ).message();
            break;
        default:
            break;
        }
    }

    return s;
}



K3b::AudioTrackModel::AudioTrackModel( QObject* parent )
    : QAbstractItemModel( parent ),
      d( new Private() )
{
}


K3b::AudioTrackModel::~AudioTrackModel()
{
    delete d;
}


void K3b::AudioTrackModel::setMedium( const K3b::Medium& medium )
{
    beginResetModel();
    d->medium = medium;
    d->itemCheckedList.resize( d->medium.toc().count() );
    for ( int i = 0; i < d->medium.toc().count(); ++i ) {
        if( d->medium.toc()[i].type() == K3b::Device::Track::TYPE_AUDIO )
            d->itemCheckedList[i] = true;
        else
            d->itemCheckedList[i] = false;
    }
    endResetModel();
}


void K3b::AudioTrackModel::setCddbInfo( const KCDDB::CDInfo& data )
{
    beginResetModel();
    d->cddbCache = data;
    endResetModel();
}


KCDDB::CDInfo K3b::AudioTrackModel::cddbInfo() const
{
    return d->cddbCache;
}


K3b::Medium K3b::AudioTrackModel::medium() const
{
    return d->medium;
}


int K3b::AudioTrackModel::columnCount( const QModelIndex& parent ) const
{
    Q_UNUSED( parent );
    return NumColumns;
}


QVariant K3b::AudioTrackModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    Q_UNUSED( orientation );

    if ( role == Qt::DisplayRole ) {
        switch( section ) {
        case TrackNumberColumn:
            return i18nc( "audio track number", "No." );
        case ArtistColumn:
            return i18n( "Artist" );
        case TitleColumn:
            return i18nc( "audio track title", "Title" );
        case LengthColumn:
            return i18n( "Length" );
        }
    }

    return QVariant();
}


QVariant K3b::AudioTrackModel::data( const QModelIndex& index, int role ) const
{
//    qDebug() << index << role;

    // FIXME: add a cache for all the values that can be changed (maybe a local KCDDB::CDInfo)
    // which will then be the first choice for all values

    int trackIndex = index.row();

    switch( role ) {
    case MediumRole:
        return qVariantFromValue( d->medium );

    case TrackNumberRole:
        return trackIndex+1;

    case ArtistRole:
        return d->getField( KCDDB::Artist, trackIndex );

    case TitleRole:
        return d->getField( KCDDB::Title, trackIndex );

    case CommentRole:
        return d->getField( KCDDB::Comment, trackIndex );

    case LengthRole:
        return qVariantFromValue( d->medium.toc()[trackIndex].length() );

    case Qt::DisplayRole:
    case Qt::EditRole:
        switch( index.column() ) {
        case TrackNumberColumn:
            return trackIndex+1;
        case ArtistColumn:
            return d->getField( KCDDB::Artist, trackIndex );
        case TitleColumn:
        {
            if( d->medium.toc()[trackIndex].type() == Device::Track::TYPE_DATA )
                return i18n( "Data Track" );
            else
                return d->getField( KCDDB::Title, trackIndex );
        }
        case LengthColumn:
            return d->medium.toc()[trackIndex].length().toString();
        }
        break;

    case Qt::CheckStateRole:
        if ( index.column() == TrackNumberColumn ) {
            return trackChecked( trackIndex ) ? Qt::Checked : Qt::Unchecked;
        }
        break;

    case Qt::TextAlignmentRole:
        if ( index.column() == LengthColumn ) {
            return Qt::AlignHCenter;
        }
        break;
        
    case Qt::FontRole:
        if( d->medium.toc()[trackIndex].type() == Device::Track::TYPE_DATA )
        {
            QFont font;
            font.setItalic( true );
            return font;
        }
    }

    return QVariant();
}


Qt::ItemFlags K3b::AudioTrackModel::flags( const QModelIndex& index ) const
{
    Qt::ItemFlags f = 0;

    if ( index.isValid() && index.row() >= 0 && index.row() < d->medium.toc().count() &&
         d->medium.toc()[index.row()].type() == K3b::Device::Track::TYPE_AUDIO ) {

        f |= Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;

        switch( index.column() ) {
        case ArtistColumn:
        case TitleColumn:
            f |= Qt::ItemIsEditable;
            break;
        }
    }

    return f;
}


QModelIndex K3b::AudioTrackModel::index( int row, int column, const QModelIndex& parent ) const
{
    if ( !parent.isValid() &&
         row < d->medium.toc().count() &&
         column < NumColumns ) {
        return createIndex( row, column );
    }
    else {
        return QModelIndex();
    }
}


QModelIndex K3b::AudioTrackModel::parent( const QModelIndex& ) const
{
    return QModelIndex();
}


int K3b::AudioTrackModel::rowCount( const QModelIndex& parent ) const
{
    if ( !parent.isValid() ) {
        // FIXME: only use audio tracks
        return d->medium.toc().count();
    }
    else {
        return 0;
    }
}


void K3b::AudioTrackModel::setTrackChecked( int track, bool checked )
{
    if ( track >= 0 && track < d->itemCheckedList.count() ) {
        d->itemCheckedList[track] = checked;
        emit dataChanged( index( track, TrackNumberColumn ), index( track, TrackNumberColumn ) );
    }
}


bool K3b::AudioTrackModel::trackChecked( int trackIndex ) const
{
    if ( trackIndex >= 0 &&
         trackIndex < d->medium.toc().count() ) {
        return d->itemCheckedList[trackIndex];
    }
    else {
        return false;
    }
}


QList<int> K3b::AudioTrackModel::checkedTrackIndices() const
{
    QList<int> l;
    for ( int i = 0; i < d->itemCheckedList.count(); ++i ) {
        if ( d->itemCheckedList[i] ) {
            l.append( i );
        }
    }
    return l;
}


bool K3b::AudioTrackModel::setData( const QModelIndex& index, const QVariant& value, int role )
{
    if ( index.isValid() ) {
        switch ( role ) {
        case Qt::EditRole:
            switch( index.column() ) {
            case ArtistColumn:
                d->cddbCache.track( index.row() ).set( KCDDB::Artist, value );
                emit dataChanged( index, index );
                return true;

            case TitleColumn:
                d->cddbCache.track( index.row() ).set( KCDDB::Title, value );
                emit dataChanged( index, index );
                return true;
            }

        case Qt::CheckStateRole:
            setTrackChecked( index.row(), value.toInt() == Qt::Checked );
            return true;

        case ArtistRole:
            d->cddbCache.track( index.row() ).set( KCDDB::Artist, value );
            emit dataChanged( index, index );
            return true;

        case TitleRole:
            d->cddbCache.track( index.row() ).set( KCDDB::Title, value );
            emit dataChanged( index, index );
            return true;

        case CommentRole:
            d->cddbCache.track( index.row() ).set( KCDDB::Comment, value );
            emit dataChanged( index, index );
            return true;
        }
    }

    return false;
}


QStringList K3b::AudioTrackModel::mimeTypes() const
{
    return AudioCdTrackDrag::mimeDataTypes();
}


QMimeData* K3b::AudioTrackModel::mimeData( const QModelIndexList& indexes ) const
{
    // FIXME: Add QDataStream operators to K3b::Medium and encode a complete K3b::Medium in
    // the mimedata including the modified cddb. This way, ejecting the medium during the
    // d'n'd is not a problem
    QList<int> trackNumbers;
    foreach( const QModelIndex& index, indexes ) {
        if ( index.column() == TrackNumberColumn )
            trackNumbers << index.data( TrackNumberColumn ).toInt();
    }
    AudioCdTrackDrag drag( d->medium.toc(), trackNumbers, d->cddbCache, d->medium.device() );
    QMimeData* mime = new QMimeData();
    drag.populateMimeData( mime );
    return mime;
}


void K3b::AudioTrackModel::checkAll()
{
    for ( int i = 0; i < d->medium.toc().count(); ++i ) {
        d->itemCheckedList[i] = true;
    }
    emit dataChanged( index( 0, TrackNumberColumn ), index( d->itemCheckedList.count(), TrackNumberColumn ) );
}


void K3b::AudioTrackModel::uncheckAll()
{
    for ( int i = 0; i < d->medium.toc().count(); ++i ) {
        d->itemCheckedList[i] = false;
    }
    emit dataChanged( index( 0, TrackNumberColumn ), index( d->itemCheckedList.count(), TrackNumberColumn ) );
}


