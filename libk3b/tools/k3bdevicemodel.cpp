/*
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3bdevicemodel.h"
#include "k3bmedium.h"
#include "k3bmediacache.h"
#include "k3bcore.h"
#include "k3bdevice.h"
#include "k3b_i18n.h"


class K3b::DeviceModel::Private
{
public:
    QList<K3b::Device::Device*> devices;
    QHash<K3b::Device::Device*, bool> devicesValid;
};


K3b::DeviceModel::DeviceModel( QObject* parent )
    : QAbstractItemModel( parent ),
      d( new Private() )
{
    connect( k3bcore->mediaCache(), SIGNAL(mediumChanged(K3b::Device::Device*)),
             this, SLOT(slotMediumChanged(K3b::Device::Device*)) );
    connect( k3bcore->mediaCache(), SIGNAL(checkingMedium(K3b::Device::Device*,QString)),
             this, SLOT(slotCheckingMedium(K3b::Device::Device*,QString)) );
}


K3b::DeviceModel::~DeviceModel()
{
    delete d;
}


void K3b::DeviceModel::setDevices( const QList<K3b::Device::Device*>& devices )
{
    beginResetModel();
    d->devices = devices;
    foreach( K3b::Device::Device* dev, devices ) {
        d->devicesValid[dev] = true;
    }
    endResetModel();
}


void K3b::DeviceModel::addDevice( K3b::Device::Device* dev )
{
    if ( !d->devices.contains( dev ) ) {
        beginResetModel();
        d->devices.append( dev );
        endResetModel(); // hardcore reset since entries might change
    }
}


void K3b::DeviceModel::removeDevice( K3b::Device::Device* dev )
{
    if ( d->devices.contains( dev ) ) {
        beginResetModel();
        d->devices.removeOne( dev );
        endResetModel();
    }
}


void K3b::DeviceModel::addDevices( const QList<K3b::Device::Device*>& devs )
{
    beginResetModel();
    Q_FOREACH( K3b::Device::Device* dev, devs ) {
        if ( !d->devices.contains( dev ) ) {
            d->devices.append( dev );
        }
    }
    endResetModel();
}


void K3b::DeviceModel::clear()
{
    beginResetModel();
    d->devices.clear();
    endResetModel();
}


QList<K3b::Device::Device*> K3b::DeviceModel::devices() const
{
    return d->devices;
}


K3b::Device::Device* K3b::DeviceModel::deviceForIndex( const QModelIndex& index ) const
{
    return static_cast<K3b::Device::Device*>( index.internalPointer() );
}


QModelIndex K3b::DeviceModel::indexForDevice( K3b::Device::Device* dev ) const
{
    for ( int i = 0; i < d->devices.count(); ++i ) {
        if ( d->devices[i] == dev ) {
            return createIndex( i, 0, dev );
        }
    }
    return QModelIndex();
}


int K3b::DeviceModel::columnCount( const QModelIndex& parent ) const
{
    Q_UNUSED( parent );
    // TODO: allow multiple columns at some point (so far we do not need it)
    return 1;
}


QVariant K3b::DeviceModel::data( const QModelIndex& index, int role ) const
{
    K3b::Device::Device* dev = deviceForIndex( index );
    K3b::Medium medium = k3bcore->mediaCache()->medium( dev );

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


QModelIndex K3b::DeviceModel::index( int row, int column, const QModelIndex& parent ) const
{
    Q_UNUSED( parent );
    return row < d->devices.count() ? createIndex( row, column, d->devices[row] ) : QModelIndex();
}


QModelIndex K3b::DeviceModel::parent( const QModelIndex& index ) const
{
    Q_UNUSED( index );
    return QModelIndex();
}


int K3b::DeviceModel::rowCount( const QModelIndex& parent ) const
{
    if ( !parent.isValid() ) {
        return d->devices.count();
    }
    else {
        return 0;
    }
}


void K3b::DeviceModel::slotMediumChanged( K3b::Device::Device* dev )
{
    QModelIndex index = indexForDevice( dev );
    if ( index.isValid() ) {
        d->devicesValid[dev] = true;
        emit dataChanged( index, index );
    }
}


void K3b::DeviceModel::slotCheckingMedium( K3b::Device::Device* dev, const QString& /*message*/ )
{
    QModelIndex index = indexForDevice( dev );
    if ( index.isValid() ) {
        d->devicesValid[dev] = false;
        emit dataChanged( index, index );
    }
}


