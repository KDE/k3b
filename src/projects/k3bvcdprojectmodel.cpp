/*
 * Copyright (C) 2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009 Arthur Mello <arthur@mandriva.com>
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bvcdprojectmodel.h"

#include "k3bvcddoc.h"
#include "k3bvcdtrack.h"

#include <KUrlMimeData>
#include <KLocalizedString>

#include <QDataStream>
#include <QMimeData>
#include <QUrl>

namespace K3b {

class VcdProjectModel::Private
{
    public:
        Private( VcdProjectModel* parent )
            : doc( 0 ),
            q( parent ) { }

        VcdDoc* doc;

        void _k_aboutToAddRows(int pos, int count)
        {
            q->beginInsertRows(QModelIndex(), pos, pos + count - 1);
        }

        void _k_addedRows()
        {
            q->endInsertRows();
        }

        void _k_aboutToRemoveRows(int pos, int count)
        {
            q->beginRemoveRows(QModelIndex(), pos, pos + count - 1);
        }

        void _k_removedRows()
        {
            q->endRemoveRows();
        }

    private:
        VcdProjectModel* q;
};


VcdProjectModel::VcdProjectModel( VcdDoc* doc, QObject* parent )
    : QAbstractTableModel( parent ),
    d( new Private(this) )
{
    d->doc = doc;

    connect( doc, SIGNAL(aboutToAddVCDTracks(int,int)),
             this, SLOT(_k_aboutToAddRows(int,int)), Qt::DirectConnection );
    connect( doc, SIGNAL(addedVCDTracks()),
             this, SLOT(_k_addedRows()), Qt::DirectConnection );
    connect( doc, SIGNAL(aboutToRemoveVCDTracks(int,int)),
             this, SLOT(_k_aboutToRemoveRows(int,int)), Qt::DirectConnection );
    connect( doc, SIGNAL(removedVCDTracks()),
             this, SLOT(_k_removedRows()), Qt::DirectConnection );
}


VcdProjectModel::~VcdProjectModel()
{
    delete d;
}


VcdDoc* VcdProjectModel::doc() const
{
    return d->doc;
}


VcdTrack* VcdProjectModel::trackForIndex( const QModelIndex& index ) const
{
    if( index.isValid() && index.row() >= 0 && index.row() < d->doc->numOfTracks() )
        return d->doc->at( index.row() );
    else
        return 0;
}


QModelIndex VcdProjectModel::indexForTrack( VcdTrack* track, int column ) const
{
    if( track != 0 && column >= 0 && column < NumColumns )
        return createIndex( track->index(), column, track );
    else
        return QModelIndex();
}


int VcdProjectModel::rowCount( const QModelIndex& parent) const
{
    if( parent.isValid() )
        return 0;
    else
        return d->doc->numOfTracks();
}


int VcdProjectModel::columnCount( const QModelIndex& /*parent*/) const
{
    return NumColumns;
}


bool VcdProjectModel::setData( const QModelIndex& index, const QVariant& value, int role )
{
    if ( index.isValid() )
    {
        VcdTrack* track = trackForIndex( index );
        if ( role == Qt::EditRole )
        {
            if ( index.column() == TitleColumn )
            {
                track->setTitle( value.toString() );
                return true;
            }
        }
    }

    return false;
}


QVariant VcdProjectModel::data( const QModelIndex& index, int role ) const
{
    if ( index.isValid() ) {
        VcdTrack* track = trackForIndex( index );

        switch( index.column() ) {
            case NoColumn:
                if( role == Qt::DisplayRole ||
                    role == Qt::EditRole )
                {
                    return track->index() + 1;
                }
                break;
            case TitleColumn:
                if( role == Qt::DisplayRole ||
                    role == Qt::EditRole )
                {
                    return track->title();
                }
                break;
            case TypeColumn:
                if( role == Qt::DisplayRole ||
                    role == Qt::EditRole )
                {
                    return track->mpegTypeS();
                }
                break;
            case ResolutionColumn:
                if( role == Qt::DisplayRole ||
                    role == Qt::EditRole )
                {
                    return track->resolution();
                }
                break;
            case HighResolutionColumn:
                if( role == Qt::DisplayRole ||
                    role == Qt::EditRole )
                {
                    return track->highresolution();
                }
                break;
            case FrameRateColumn:
                if( role == Qt::DisplayRole ||
                    role == Qt::EditRole )
                {
                    return track->video_frate();
                }
                break;
            case MuxRateColumn:
                if( role == Qt::DisplayRole ||
                    role == Qt::EditRole )
                {
                    return QString::number( track->muxrate() );
                }
                break;
            case DurationColumn:
                if( role == Qt::DisplayRole ||
                    role == Qt::EditRole )
                {
                    return track->duration();
                }
                break;
            case SizeColumn:
                if( role == Qt::DisplayRole ||
                    role == Qt::EditRole )
                {
                    return KIO::convertSize( track->size() );
                }
                break;
            case FilenameColumn:
                if( role == Qt::DisplayRole ||
                    role == Qt::EditRole )
                {
                    return track->fileName();
                }
                break;
        }
    }

    return QVariant();
}


QVariant VcdProjectModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole ) {
        switch( section ) {
            case NoColumn:
                return i18nc( "Video CD Track Number", "No." );
            case TitleColumn:
                return i18nc( "Video CD Track Title", "Title" );
            case TypeColumn:
                return i18nc( "Video CD Track Type (ie. MPEG1)", "Type" );
            case ResolutionColumn:
                return i18nc( "Video CD Track Resolution", "Resolution" );
            case HighResolutionColumn:
                return i18nc(   "Video CD Track High Resolution",
                                "High Resolution" );
            case FrameRateColumn:
                return i18nc( "Video CD Track Framerate", "Framerate" );
            case MuxRateColumn:
                return i18nc( "Video CD Track Muxrate", "Muxrate" );
            case DurationColumn:
                return i18nc( "Video CD Track Duration", "Duration" );
            case SizeColumn:
                return i18nc( "Video CD Track File Size", "File Size" );
            case FilenameColumn:
                return i18nc( "Video CD Track Filename", "Filename" );
            default:
                return QVariant();
        }
    }
    else {
        return QVariant();
    }
}


Qt::ItemFlags VcdProjectModel::flags( const QModelIndex& index ) const
{
    if( index.isValid() )
    {
        Qt::ItemFlags f = Qt::ItemIsSelectable |
                          Qt::ItemIsEnabled |
                          Qt::ItemIsDropEnabled |
                          Qt::ItemIsDragEnabled;

        if( index.column() == TitleColumn )
        {
            f |= Qt::ItemIsEditable;
        }

        return f;
    }
    else
    {
        return QAbstractItemModel::flags( index )|Qt::ItemIsDropEnabled;
    }
}


Qt::DropActions VcdProjectModel::supportedDragActions() const
{
    return Qt::MoveAction;
}


Qt::DropActions VcdProjectModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}


QMimeData* VcdProjectModel::mimeData( const QModelIndexList& indexes ) const
{
    QMimeData* mime = new QMimeData();

    QList<VcdTrack*> tracks;
    QList<QUrl> urls;

    foreach( const QModelIndex& index, indexes ) {
        VcdTrack* track = trackForIndex( index );
        tracks << track;

        if( !urls.contains( QUrl::fromLocalFile( track->absolutePath() ) ) )
        {
            urls << QUrl::fromLocalFile( track->absolutePath() );
        }
    }
    mime->setUrls(urls);

    // the easy road: encode the pointers
    QByteArray trackData;
    QDataStream trackDataStream( &trackData, QIODevice::WriteOnly );

    foreach( VcdTrack* track, tracks ) {
        trackDataStream << ( qint64 )track;
    }

    mime->setData( "application/x-k3bvcdtrack", trackData );

    return mime;
}


QStringList VcdProjectModel::mimeTypes() const
{
    QStringList s = KUrlMimeData::mimeDataTypes();

    s += QString::fromLatin1( "application/x-k3bvcdtrack" );

    return s;
}


bool VcdProjectModel::dropMimeData( const QMimeData* data, Qt::DropAction action, int row, int /*column*/, const QModelIndex& parent )
{
    if( action == Qt::IgnoreAction ) {
        return true;
    }
    else if( data->hasFormat( "application/x-k3bvcdtrack" ) ) {
        VcdTrack* before;
        if( parent.isValid() )
            row = parent.row();

        if( row >= 0 && row < d->doc->numOfTracks() )
            before = d->doc->at( row );
        else
            before = 0;

        QByteArray trackData = data->data( "application/x-k3bvcdtrack" );
        QDataStream trackDataStream( trackData );
        while ( !trackDataStream.atEnd() )
        {
            qint64 p;
            trackDataStream >> p;
            VcdTrack* track = reinterpret_cast< VcdTrack* >( p );
            d->doc->moveTrack( track, before );
        }

        return true;
    }
    else if( data->hasUrls() ) {
        int pos;
        if( parent.isValid() )
            row = parent.row();

        if( row >= 0 )
            pos = row;
        else
            pos = d->doc->numOfTracks();

        QList<QUrl> urls = KUrlMimeData::urlsFromMimeData( data );
        d->doc->addTracks( urls, pos );

        return true;
    }
    else {
        return false;
    }
}


bool VcdProjectModel::removeRows( int row, int count, const QModelIndex& parent )
{
    // remove the indexes from the project
    while (count > 0)
    {
        QModelIndex i = index( row, 0, parent );
        d->doc->removeTrack( trackForIndex(i) );

        row++;
        count--;
    }

    return true;
}

} // namespace K3b

#include "moc_k3bvcdprojectmodel.cpp"
