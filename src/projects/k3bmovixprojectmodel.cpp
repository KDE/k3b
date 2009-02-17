/*
 * Copyright (C) 2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009 Arthur Renato Mello <arthur@mandriva.com>
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

#include "k3bmovixprojectmodel.h"

#include <k3bmovixdoc.h>
#include <k3bmovixfileitem.h>

#include <KLocale>
#include <KUrl>

#include <QtCore/QMimeData>
#include <QtCore/QDataStream>

using namespace K3b;

class MovixProjectModel::Private
{
    public:
        Private( MovixProjectModel* parent )
            : project( 0 ),
            q( parent ) { }

        K3bMovixDoc* project;

        void _k_docChanged()
        {
            q->reset();
        }

    private:
        MovixProjectModel* q;
};

MovixProjectModel::MovixProjectModel( K3bMovixDoc* doc, QObject* parent )
    : QAbstractItemModel( parent ),
    d( new Private(this) )
{
    d->project = doc;

    connect(doc, SIGNAL(changed()), this, SLOT(_k_docChanged()));
}

MovixProjectModel::~MovixProjectModel()
{
    delete d;
}

K3bMovixDoc* MovixProjectModel::project() const
{
    return d->project;
}

K3bMovixFileItem* MovixProjectModel::itemForIndex( const QModelIndex& index ) const
{
#if 0
    if ( index.isValid() )
    {
        Q_ASSERT( index.internalPointer() );
        return static_cast<K3bVcdTrack*>( index.internalPointer() );
    }
#endif

    return 0;
}

QModelIndex MovixProjectModel::indexForItem( K3bMovixFileItem* track ) const
{
#if 0
    return createIndex( track->index(), 0, track );
#endif
    return QModelIndex();
}

QModelIndex MovixProjectModel::index( int row, int column,
    const QModelIndex& parent ) const
{
#if 0
    Q_UNUSED( parent );

    return createIndex( row, column, d->project->at(row) );
#endif
    return QModelIndex();
}

QModelIndex MovixProjectModel::parent( const QModelIndex& index ) const
{
#if 0
    Q_UNUSED( index );
#endif

    return QModelIndex();
}

int MovixProjectModel::rowCount( const QModelIndex& parent) const
{
#if 0
    if ( parent.isValid() )
        return 0;
    else
        return d->project->numOfTracks();
#endif
    return 0;
}

int MovixProjectModel::columnCount( const QModelIndex& parent) const
{
    Q_UNUSED( parent );

    return NumColumns;
}

bool MovixProjectModel::setData( const QModelIndex& index,
    const QVariant& value, int role )
{
#if 0
    if ( index.isValid() )
    {
        K3bVcdTrack* track = trackForIndex( index );
        if ( role == Qt::EditRole )
        {
            if ( index.column() == TitleColumn )
            {
                track->setTitle( value.toString() );
                return true;
            }
        }
    }
#endif
    return false;
}

QVariant MovixProjectModel::data( const QModelIndex& index, int role ) const
{
#if 0
    if ( index.isValid() ) {
        K3bVcdTrack* track = trackForIndex( index );

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
#endif
    return QVariant();
}

QVariant MovixProjectModel::headerData( int section,
    Qt::Orientation orientation, int role ) const
{
#if 0
    Q_UNUSED( orientation );

    if ( role == Qt::DisplayRole ) {
        switch( section ) {
            case NoColumn:
                return i18nc( "VideoCD Track Number", "No." );
            case TitleColumn:
                return i18nc( "VideoCD Track Title", "Title" );
            case TypeColumn:
                return i18nc( "VideoCD Track Type (ie. MPEG1)", "Type" );
            case ResolutionColumn:
                return i18nc( "VideoCD Track Resolution", "Resolution" );
            case HighResolutionColumn:
                return i18nc(   "VideoCD Track High Resolution",
                                "High Resolution" );
            case FrameRateColumn:
                return i18nc( "VideoCD Track Framerate", "Framerate" );
            case MuxRateColumn:
                return i18nc( "VideoCD Track Muxrate", "Muxrate" );
            case DurationColumn:
                return i18nc( "VideoCD Track Duration", "Duration" );
            case SizeColumn:
                return i18nc( "VideoCD Track File Size", "File Size" );
            case FilenameColumn:
                return i18nc( "VideoCD Track Filename", "Filename" );
        }
    }
#endif
    return QVariant();
}

Qt::ItemFlags MovixProjectModel::flags( const QModelIndex& index ) const
{
#if 0
    if ( index.isValid() )
    {
        Qt::ItemFlags f = Qt::ItemIsSelectable |
                          Qt::ItemIsEnabled |
                          Qt::ItemIsDropEnabled |
                          Qt::ItemIsDragEnabled;

        if ( index.column() == TitleColumn )
        {
            f |= Qt::ItemIsEditable;
        }

        return f;
    }
    else
    {
        return QAbstractItemModel::flags( index )|Qt::ItemIsDropEnabled;
    }
#endif
    return QAbstractItemModel::flags( index );
}

Qt::DropActions MovixProjectModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

QMimeData* MovixProjectModel::mimeData( const QModelIndexList& indexes ) const
{
    QMimeData* mime = new QMimeData();
#if 0

    QList<K3bVcdTrack*> tracks;
    KUrl::List urls;

    foreach( const QModelIndex& index, indexes ) {
        K3bVcdTrack* track = trackForIndex( index );
        tracks << track;

        if( !urls.contains( KUrl( track->absolutePath() ) ) )
        {
            urls << KUrl( track->absolutePath() );
        }
    }
    urls.populateMimeData( mime );

    // the easy road: encode the pointers
    QByteArray trackData;
    QDataStream trackDataStream( &trackData, QIODevice::WriteOnly );

    foreach( K3bVcdTrack* track, tracks ) {
        trackDataStream << ( qint64 )track;
    }

    mime->setData( "application/x-k3bvcdtrack", trackData );
#endif

    return mime;
}

QStringList MovixProjectModel::mimeTypes() const
{
    QStringList s = KUrl::List::mimeDataTypes();
#if 0
    s += QString::fromLatin1( "application/x-k3bvcdtrack" );
#endif

    return s;
}

bool MovixProjectModel::dropMimeData( const QMimeData* data,
    Qt::DropAction action, int row, int column, const QModelIndex& parent )
{
#if 0
    Q_UNUSED( column );

    if (action == Qt::IgnoreAction)
        return true;

    QList<K3bVcdTrack*> tracks;
    if ( data->hasFormat( "application/x-k3bvcdtrack" ) )
    {
        QByteArray trackData = data->data( "application/x-k3bvcdtrack" );
        QDataStream trackDataStream( trackData );
        while ( !trackDataStream.atEnd() )
        {
            qint64 p;
            trackDataStream >> p;
            tracks << ( K3bVcdTrack* )p;
        }

        K3bVcdTrack *prev;
        if(parent.isValid())
        {
            int index = trackForIndex(parent)->index();
            if(index == 0)
                prev = 0;
            else
                prev = d->project->at(index - 1);
        }
        else if(row >= 0)
            prev = d->project->at(row - 1);
        else
            prev = d->project->tracks()->last();

        foreach( K3bVcdTrack* track, tracks )
        {
            d->project->moveTrack(track, prev);
            prev = track;
        }

        return true;
    }

    if ( KUrl::List::canDecode( data ) )
    {
        int pos;
        if(parent.isValid())
            pos = trackForIndex(parent)->index();
        else if(row >= 0)
            pos = row;
        else
            pos = d->project->numOfTracks();

        KUrl::List urls = KUrl::List::fromMimeData( data );
        d->project->addTracks( urls, pos );

        return true;
    }
#endif
    return false;
}

#include "k3bmovixprojectmodel.moc"
