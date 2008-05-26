/*
 *
 * Copyright (C) 2007-2008 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bdevicemodel.h"
#include "k3bmedium.h"
#include "k3bmediacache.h"
#include "k3bcore.h"

#include <k3bdevice.h>

#include <KLocale>


class K3bDeviceModel::Private
{
public:
    QList<K3bDevice::Device*> devices;
    QHash<K3bDevice::Device*, bool> devicesValid;
};


K3bDeviceModel::K3bDeviceModel( QObject* parent )
    : QAbstractItemModel( parent ),
      d( new Private() )
{
    connect( k3bcore->mediaCache(), SIGNAL( mediumChanged( K3bDevice::Device* ) ),
             this, SLOT( slotMediumChanged( K3bDevice::Device* ) ) );
    connect( k3bcore->mediaCache(), SIGNAL( checkingMedium( K3bDevice::Device*, QString ) ),
             this, SLOT( slotCheckingMedium( K3bDevice::Device*, QString ) ) );
}


K3bDeviceModel::~K3bDeviceModel()
{
    delete d;
}


void K3bDeviceModel::setDevices( const QList<K3bDevice::Device*>& devices )
{
    d->devices = devices;
    foreach( K3bDevice::Device* dev, devices ) {
        d->devicesValid[dev] = true;
    }
    reset();
}


void K3bDeviceModel::addDevice( K3bDevice::Device* dev )
{
    if ( !d->devices.contains( dev ) ) {
        d->devices.append( dev );
        reset(); // hardcore reset since entries might change
    }
}


void K3bDeviceModel::removeDevice( K3bDevice::Device* dev )
{
    if ( d->devices.contains( dev ) ) {
        d->devices.removeOne( dev );
            reset();
    }
}


void K3bDeviceModel::addDevices( const QList<K3bDevice::Device*>& devs )
{
    Q_FOREACH( K3bDevice::Device* dev, devs ) {
        if ( !d->devices.contains( dev ) ) {
            d->devices.append( dev );
        }
    }
    reset();
}


void K3bDeviceModel::clear()
{
    d->devices.clear();
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
    for ( int i = 0; i < d->devices.count(); ++i ) {
        if ( d->devices[i] == dev ) {
            return createIndex( i, 0, dev );
        }
    }
    return QModelIndex();
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
    K3bMedium medium = k3bcore->mediaCache()->medium( dev );

    switch( role ) {
    case Qt::DisplayRole:
        if ( d->devicesValid[dev] ) {
            return medium.shortString();
        }
        else {
            return i18n( "Analyzing medium..." );
        }

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

    case Valid:
        return d->devicesValid[dev];

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


void K3bDeviceModel::slotMediumChanged( K3bDevice::Device* dev )
{
    QModelIndex index = indexForDevice( dev );
    if ( index.isValid() ) {
        d->devicesValid[dev] = true;
        emit dataChanged( index, index );
    }
}


void K3bDeviceModel::slotCheckingMedium( K3bDevice::Device* dev, const QString& /*message*/ )
{
    QModelIndex index = indexForDevice( dev );
    if ( index.isValid() ) {
        d->devicesValid[dev] = false;
        emit dataChanged( index, index );
    }
}

#include "k3bdevicemodel.moc"
