/*
 *
 * $Id: k3bdevicecombobox.cpp 735766 2007-11-12 15:25:22Z trueg $
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bdevicelistmodel.h"

#include <k3bdevicemanager.h>
#include <k3bdevice.h>
#include <k3bcore.h>

#include <QtCore/QList>


class DeviceEntry {
public:
    DeviceEntry( K3bDevice::Device* dev )
        : device( dev ),
          showBlockDevice( false ) {
    }

    K3bDevice::Device* device;
    bool showBlockDevice;
};


class K3bDeviceListModel::Private
{
public:
    QList<DeviceEntry*> devices;

    QString deviceText( K3bDevice::Device* dev ) const {
        return dev->vendor() + ' ' + dev->description();
    }

    void update() {
        QMap<QString, DeviceEntry*> el;
        Q_FOREACH( DeviceEntry* dev, devices ) {
            dev->showBlockDevice = false;
            if ( el.contains( deviceText( dev->device ) ) ) {
                el[deviceText( dev->device )]->showBlockDevice = true;
                dev->showBlockDevice = true;
            }
            el[deviceText( dev->device )] = dev;
        }
    }
};


K3bDeviceListModel::K3bDeviceListModel( QObject* parent )
    : QAbstractItemModel( parent ),
      d( new Private() )
{
    connect( k3bcore->deviceManager(), SIGNAL(changed(K3bDevice::DeviceManager*)),
             this, SLOT(slotDeviceManagerChanged(K3bDevice::DeviceManager*)) );
}


K3bDeviceListModel::~K3bDeviceListModel()
{
    delete d;
}


void K3bDeviceListModel::addDevice( K3bDevice::Device* dev )
{
    Q_FOREACH( DeviceEntry* de, d->devices ) {
        if ( de->device == dev ) {
            return;
        }
    }

    d->devices.append( new DeviceEntry( dev ) );
    d->update();
    reset(); // hardcore reset since entries might change
}


void K3bDeviceListModel::removeDevice( K3bDevice::Device* dev )
{
    Q_FOREACH( DeviceEntry* de, d->devices ) {
        if ( de->device == dev ) {
            d->devices.removeAll( de );
            delete de;
            d->update();
            reset();
        }
    }
}


void K3bDeviceListModel::addDevices( const QList<K3bDevice::Device*>& devs )
{
    Q_FOREACH( K3bDevice::Device* dev, devs ) {
        addDevice( dev );
    }
}


void K3bDeviceListModel::setDevices( const QList<K3bDevice::Device*>& devs )
{
    // FIXME: remember selection
    clear();
    addDevices( devs );
}


void K3bDeviceListModel::clear()
{
    qDeleteAll( d->devices );
    d->devices.clear();
    reset();
}



int K3bDeviceListModel::columnCount( const QModelIndex& parent ) const
{
    Q_UNUSED( parent );
    return 1;
}


QVariant K3bDeviceListModel::data( const QModelIndex& index, int role ) const
{
    DeviceEntry* dev = static_cast<DeviceEntry*>( index.internalPointer() );

    switch( role ) {
    case Qt::DisplayRole:
        if ( dev->showBlockDevice ) {
            return d->deviceText( dev->device ) + " (" + dev->device->blockDeviceName() + ")";
        }
        else {
            return d->deviceText( dev->device );
        }

    case Qt::ToolTipRole:
        // FIXME: add a nice tooltip
        return QVariant();
    }

    return QVariant();
}


QModelIndex K3bDeviceListModel::index ( int row, int column, const QModelIndex& parent ) const
{
    Q_UNUSED( parent );
    return createIndex( row, column, &d->devices[row] );
}


QModelIndex K3bDeviceListModel::parent ( const QModelIndex& index ) const
{
    Q_UNUSED( index );
    return QModelIndex();
}


int K3bDeviceListModel::rowCount ( const QModelIndex& parent ) const
{
    Q_UNUSED( parent );
    return d->devices.count();
}


void K3bDeviceListModel::slotDeviceManagerChanged( K3bDevice::DeviceManager* dm )
{
    QList<K3bDevice::Device*> allDevices = dm->allDevices();
    int i = 0;
    while( i < d->devices.count() ) {
        if ( !allDevices.contains( d->devices[i]->device ) ) {
            removeDevice( d->devices[i]->device );
            i = 0;
        }
        else {
            ++i;
        }
    }
}


K3bDevice::Device* K3bDeviceListModel::deviceForIndex( const QModelIndex& index ) const
{
    return static_cast<DeviceEntry*>( index.internalPointer() )->device;
}

#include "k3bdevicelistmodel.moc"
