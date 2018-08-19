/*
 *
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2010 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bvcdtrackkeysmodel.h"
#include "k3bvcdtrack.h"

#include <KLocalizedString>

#include <QIcon>

Q_DECLARE_METATYPE( K3b::VcdTrack* )

namespace K3b {


class VcdTrackKeysModel::Private
{
public:
    Private( int kc ) : keyCount( kc ) {}

    Key2Track keys;
    int keyCount;
};


VcdTrackKeysModel::VcdTrackKeysModel( int keyCount, QObject* parent )
    : QAbstractTableModel( parent ),
      d( new Private( keyCount ) )
{
}


VcdTrackKeysModel::~VcdTrackKeysModel()
{
    delete d;
}


int VcdTrackKeysModel::keyCount() const
{
    return d->keyCount;
}


void VcdTrackKeysModel::setKeys( const Key2Track& keys )
{
    beginResetModel();
    d->keys = keys;
    endResetModel();
}


const VcdTrackKeysModel::Key2Track& VcdTrackKeysModel::keys() const
{
    return d->keys;
}


Qt::ItemFlags VcdTrackKeysModel::flags( const QModelIndex& index ) const
{
    if( index.isValid() && index.column() == PlayingColumn )
        return QAbstractTableModel::flags( index ) | Qt::ItemIsEditable;
    else
        return QAbstractTableModel::flags( index );
}


QVariant VcdTrackKeysModel::data( const QModelIndex& index, int role ) const
{
    if( index.isValid() ) {
        if( role == Qt::DisplayRole || role == Qt::EditRole ) {
            if( index.column() == KeyColumn ) {
                return index.row() + 1;
            }
            else if( index.column() == PlayingColumn ) {
                Key2Track::const_iterator it = d->keys.constFind( index.row()+1 );
                if( it != d->keys.constEnd() ) {
                    return trackName( it.value() );
                }
            }
        }
        else if( role == Qt::DecorationRole ) {
            if( index.column() == PlayingColumn ) {
                Key2Track::const_iterator it = d->keys.constFind( index.row()+1 );
                if( it != d->keys.constEnd() ) {
                    return trackIcon( it.value() );
                }
            }
        }
        else if( role == TrackRole ) {
            Key2Track::const_iterator it = d->keys.constFind( index.row()+1 );
            if( it != d->keys.constEnd() )
                return QVariant::fromValue<VcdTrack*>( it.value() );
            else
                return QVariant();
        }
    }
    return QVariant();;
}


bool VcdTrackKeysModel::setData( const QModelIndex& index, const QVariant& value, int role )
{
    if( index.isValid() && index.column() == PlayingColumn && role == Qt::EditRole ) {
        if( value.isNull() ) {
            d->keys.remove( index.row()+1 );
        }
        else {
            VcdTrack* track = value.value<VcdTrack*>();
            d->keys[ index.row()+1 ] = track;
        }
        emit dataChanged( index, index );
        return true;
    }
    return false;
}


int VcdTrackKeysModel::columnCount( const QModelIndex& /*parent*/ ) const
{
    return ColumnCount;
}


int VcdTrackKeysModel::rowCount( const QModelIndex& parent ) const
{
    if( parent.isValid() )
        return 0;
    else
        return d->keyCount;
}


QVariant VcdTrackKeysModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole ) {
        switch( section ) {
            case KeyColumn:     return i18n( "Key" );
            case PlayingColumn: return i18n( "Playing" );
            default:            return QVariant();
        }
    }
    else {
        return QVariant();
    }
}


QModelIndex VcdTrackKeysModel::buddy( const QModelIndex& index ) const
{
    if( index.isValid() && index.column() == KeyColumn )
        return QAbstractTableModel::index( index.row(), PlayingColumn );
    else
        return index;
}


QString VcdTrackKeysModel::trackName( VcdTrack* track )
{
    if( track == 0 )
        return i18n( "Video CD END" );
    else if( track->isSegment() )
        return i18n( "Segment-%1 - %2" , QString::number( track->index() + 1 ).rightJustified( 3, '0' ) , track->title() );
    else
        return i18n( "Sequence-%1 - %2" , QString::number( track->index() + 1 ).rightJustified( 3, '0' ) , track->title() );
}


QIcon VcdTrackKeysModel::trackIcon( VcdTrack* track )
{
    if( track == 0 )
        return QIcon::fromTheme( "media-optical-video" );
    else if( track->isSegment() )
        return QIcon::fromTheme( "image-x-generic" );
    else
        return QIcon::fromTheme( "video-x-generic" );
}

} // namespace K3b
