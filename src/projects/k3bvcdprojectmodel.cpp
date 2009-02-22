/*
 * Copyright (C) 2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009 Arthur Mello <arthur@mandriva.com>
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

#include <k3bvcddoc.h>
#include <k3bvcdtrack.h>

#include <KLocale>
#include <KUrl>

#include <QtCore/QMimeData>
#include <QtCore/QDataStream>

using namespace K3b;

class VcdProjectModel::Private
{
    public:
        Private( VcdProjectModel* parent )
            : project( 0 ),
            q( parent ) { }

        K3b::VcdDoc* project;

        void _k_docChanged()
        {
            q->reset();
        }

    private:
        VcdProjectModel* q;
};

VcdProjectModel::VcdProjectModel( K3b::VcdDoc* doc, QObject* parent )
    : QAbstractItemModel( parent ),
    d( new Private(this) )
{
    d->project = doc;

    connect(doc, SIGNAL(changed()), this, SLOT(_k_docChanged()));
}

VcdProjectModel::~VcdProjectModel()
{
    delete d;
}

K3b::VcdDoc* VcdProjectModel::project() const
{
    return d->project;
}

K3b::VcdTrack* VcdProjectModel::trackForIndex( const QModelIndex& index ) const
{
    if ( index.isValid() )
    {
        Q_ASSERT( index.internalPointer() );
        return static_cast<K3b::VcdTrack*>( index.internalPointer() );
    }

    return 0;
}

QModelIndex VcdProjectModel::indexForTrack( K3b::VcdTrack* track ) const
{
    return createIndex( track->index(), 0, track );
}

QModelIndex VcdProjectModel::index( int row, int column,
    const QModelIndex& parent ) const
{
    Q_UNUSED( parent );

    return createIndex( row, column, d->project->at(row) );
}

QModelIndex VcdProjectModel::parent( const QModelIndex& index ) const
{
    Q_UNUSED( index );

    return QModelIndex();
}

int VcdProjectModel::rowCount( const QModelIndex& parent) const
{
    if ( parent.isValid() )
        return 0;
    else
        return d->project->numOfTracks();
}

int VcdProjectModel::columnCount( const QModelIndex& parent) const
{
    Q_UNUSED( parent );

    return NumColumns;
}

bool VcdProjectModel::setData( const QModelIndex& index,
    const QVariant& value, int role )
{
    if ( index.isValid() )
    {
        K3b::VcdTrack* track = trackForIndex( index );
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
        K3b::VcdTrack* track = trackForIndex( index );

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

QVariant VcdProjectModel::headerData( int section,
    Qt::Orientation orientation, int role ) const
{
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

    return QVariant();
}

Qt::ItemFlags VcdProjectModel::flags( const QModelIndex& index ) const
{
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
}

Qt::DropActions VcdProjectModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

QMimeData* VcdProjectModel::mimeData( const QModelIndexList& indexes ) const
{
    QMimeData* mime = new QMimeData();

    QList<K3b::VcdTrack*> tracks;
    KUrl::List urls;

    foreach( const QModelIndex& index, indexes ) {
        K3b::VcdTrack* track = trackForIndex( index );
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

    foreach( K3b::VcdTrack* track, tracks ) {
        trackDataStream << ( qint64 )track;
    }

    mime->setData( "application/x-k3bvcdtrack", trackData );

    return mime;
}

QStringList VcdProjectModel::mimeTypes() const
{
    QStringList s = KUrl::List::mimeDataTypes();

    s += QString::fromLatin1( "application/x-k3bvcdtrack" );

    return s;
}

bool VcdProjectModel::dropMimeData( const QMimeData* data,
    Qt::DropAction action, int row, int column, const QModelIndex& parent )
{
    Q_UNUSED( column );

    if (action == Qt::IgnoreAction)
        return true;

    QList<K3b::VcdTrack*> tracks;
    if ( data->hasFormat( "application/x-k3bvcdtrack" ) )
    {
        QByteArray trackData = data->data( "application/x-k3bvcdtrack" );
        QDataStream trackDataStream( trackData );
        while ( !trackDataStream.atEnd() )
        {
            qint64 p;
            trackDataStream >> p;
            tracks << ( K3b::VcdTrack* )p;
        }

        K3b::VcdTrack *prev;
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

        foreach( K3b::VcdTrack* track, tracks )
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

    return false;
}

#include "k3bvcdprojectmodel.moc"
