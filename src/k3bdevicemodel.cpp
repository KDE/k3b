/*
 *
 * Copyright (C) 2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bdevicemodel.h"
#include "k3bmedium.h"
#include "k3bmediacache.h"
#include "k3bapplication.h"

#include <k3bdevice.h>

class K3bDeviceModel::Private
{
public:
    QList<K3bDevice::Device*> devices;
};


K3bDeviceModel::K3bDeviceModel( QObject* parent )
    : QAbstractItemModel( parent ),
      d( new Private() )
{
}


K3bDeviceModel::~K3bDeviceModel()
{
    delete d;
}


void K3bDeviceModel::setDevices( const QList<K3bDevice::Device*>& devices )
{
    d->devices = devices;
    reset();
}


QList<K3bDevice::Device*> K3bDeviceModel::devices() const
{
    return d->devices;
}


K3bDevice::Device* K3bDeviceModel::deviceForIndex( const QModelIndex& index ) const
{
    return static_cast<K3bDevice::Device*>( index.internalPointer() );
}


QModelIndex K3bDeviceModel::indexForDevice( K3bDevice::Device* dev ) const
{

}


int K3bDeviceModel::columnCount( const QModelIndex& parent ) const
{
    Q_UNUSED( parent );
    // TODO: allow multiple columns at some point (so far we do not need it)
    return 1;
}


QVariant K3bDeviceModel::data( const QModelIndex& index, int role ) const
{
    K3bDevice::Device* dev = deviceForIndex( index );
    K3bMedium medium = k3bappcore->mediaCache()->medium( dev );

    switch( role ) {
    case Qt::DisplayRole:
        return medium.shortString();

    case Qt::DecorationRole:
        return medium.icon();

    case IsDevice:
        return true;

    case Vendor:
        return dev->vendor();

    case Description:
        return dev->description();

    case BlockDevice:
        return dev->blockDeviceName();

    default:
        return QVariant();
    }
}


QModelIndex K3bDeviceModel::index( int row, int column, const QModelIndex& parent ) const
{
    Q_UNUSED( parent );
    return row < d->devices.count() ? createIndex( row, column, d->devices[row] ) : QModelIndex();
}


QModelIndex K3bDeviceModel::parent( const QModelIndex& index ) const
{
    Q_UNUSED( index );
    return QModelIndex();
}


int K3bDeviceModel::rowCount( const QModelIndex& parent ) const
{
    if ( !parent.isValid() ) {
        return d->devices.count();
    }
    else {
        return 0;
    }
}

#include "k3bdevicemodel.moc"
