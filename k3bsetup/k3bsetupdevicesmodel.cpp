/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009 Michal Malek <michalm@jabster.pl>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bsetupdevicesmodel.h"
#include <k3bdevicemanager.h>
#include <k3bdevice.h>

#include <KConfig>
#include <KConfigGroup>
#include <KDebug>
#include <KLocale>

#include <QFile>
#include <QFileInfo>
#include <QSet>

#include <sys/stat.h>

namespace K3b {
namespace Setup {

class DevicesModel::Private
{
public:
    Device::DeviceManager* deviceManager;
    QSet<Device::Device*> unselectedDevices;
    QString burningGroup;

    bool needChangePermissions( const K3b::Device::Device* device );
};


bool DevicesModel::Private::needChangePermissions( const K3b::Device::Device* device )
{
    struct stat s;
    if( ::stat( QFile::encodeName( device->blockDeviceName() ), &s ) == 0 ) {

        QFileInfo fi( device->blockDeviceName() );
        int perm = s.st_mode & 0000777;

        if( !burningGroup.isEmpty() ) {
            if( perm != 0000660 || fi.group() != burningGroup )
                return true;
        }
        else if( perm != 0000666 ) {
             return true;
        }
    }
    return false;
}


DevicesModel::DevicesModel( QObject* parent )
:
    QAbstractItemModel( parent ),
    d( new Private )
{
    d->deviceManager = new K3b::Device::DeviceManager( this );
    d->deviceManager->scanBus();
    connect( d->deviceManager, SIGNAL(changed()), this, SLOT(update()) );
}


DevicesModel::~DevicesModel()
{
    delete d;
}


void DevicesModel::load( const KConfig& config )
{
    d->unselectedDevices.clear();
    d->deviceManager->readConfig( config.group( "Devices" ) );
    reset();
}


void DevicesModel::defaults()
{
    d->unselectedDevices.clear();
    reset();
}


void DevicesModel::save( KConfig& config ) const
{
    d->deviceManager->saveConfig( config.group( "Devices" ) );
}


QStringList DevicesModel::selectedDevices() const
{
    QStringList deviceNodes;
    Q_FOREACH( Device::Device* device, d->deviceManager->allDevices() )
    {
        if( !d->unselectedDevices.contains( device ) && d->needChangePermissions( device ) )
            deviceNodes.push_back( device->blockDeviceName() );
    }
    return deviceNodes;
}


bool DevicesModel::changesNeeded() const
{
    return !selectedDevices().isEmpty();
}


QVariant DevicesModel::data( const QModelIndex& index, int role ) const
{
    if( index.isValid() && role == Qt::DisplayRole &&  index.column() >= 0 && index.column() <= 3 ) {
        Device::Device* device = static_cast<Device::Device*>( index.internalPointer() );
        if( index.column() == 0 ) {
            return device->vendor() + " " + device->description();
        }
        else if( index.column() == 1 ) {
            return device->blockDeviceName();
        }
        else {
            struct stat s;
            if( ::stat( QFile::encodeName( device->blockDeviceName() ), &s ) == 0 ) {

                QFileInfo fi( device->blockDeviceName() );
                int perm = s.st_mode & 0000777;

                if( index.column() == 2 ) {
                    return QString::number( perm, 8 ).rightJustified( 3, '0' ) + " " + fi.owner() + "." + fi.group();
                }
                else if( !d->burningGroup.isEmpty() ) {
                    // we ignore the device's owner here
                    if( perm != 0000660 || fi.group() != d->burningGroup )
                        return "660 " + fi.owner() + "." + d->burningGroup;
                    else
                        return i18n("no change");
                }
                else {
                    // we ignore the device's owner and group here
                    if( perm != 0000666 )
                        return "666 " + fi.owner() + "." + fi.group();
                    else
                        return i18n("no change");
                }
            }
            else {
                kDebug() << "(K3bSetup2) unable to stat " << device->blockDeviceName();
                return QVariant();
            }
        }
    }
    else if( index.isValid() && role == Qt::CheckStateRole && index.column() == 0 ) {
        Device::Device* device = static_cast<Device::Device*>( index.internalPointer() );
        return d->unselectedDevices.contains( device ) ? Qt::Unchecked : Qt::Checked;
    }
    else
        return QVariant();
}


bool DevicesModel::setData( const QModelIndex& index, const QVariant& value, int role )
{
    if( index.isValid() && role == Qt::CheckStateRole ) {
        Device::Device* device = static_cast<Device::Device*>( index.internalPointer() );
        if( value.toInt() == Qt::Checked && d->unselectedDevices.contains( device ) ) {
            d->unselectedDevices.remove( device );
            emit dataChanged( index, index );
            return true;
        }
        else if( value.toInt() == Qt::Unchecked && !d->unselectedDevices.contains( device ) ) {
            d->unselectedDevices.insert( device );
            emit dataChanged( index, index );
            return true;
        }
        else
            return false;
    }
    else
        return false;
}


Qt::ItemFlags DevicesModel::flags( const QModelIndex& index ) const
{
    if( index.isValid() && index.column() != 0 )
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    else if( index.isValid() && index.column() == 0 )
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsUserCheckable;
    else
        return 0;
}


QVariant DevicesModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    if( orientation == Qt::Horizontal && role == Qt::DisplayRole ) {
        switch( section )
        {
            case 0: return i18n( "Device" );
            case 1: return i18n( "Devicenode" );
            case 2: return i18n( "Permissions" );
            case 3: return i18n( "New permissions" );
            default: return QVariant();
        }
    }
    else
        return QVariant();
}


QModelIndex DevicesModel::index( int row, int column, const QModelIndex& parent ) const
{
    if( hasIndex(row, column, parent) && !parent.isValid() ) {
        Device::Device* device = d->deviceManager->allDevices().at( row );
        if( device != 0 )
            return createIndex( row, column, device );
        else {
            kDebug() << "device manager returned empty device!";
            return QModelIndex();
        }
    }
    else
        return QModelIndex();
}


QModelIndex DevicesModel::parent( const QModelIndex& index ) const
{
    Q_UNUSED( index );
    return QModelIndex();
}


int DevicesModel::rowCount( const QModelIndex& parent ) const
{
    if( !parent.isValid() )
        return d->deviceManager->allDevices().size();
    else
        return 0;
}


int DevicesModel::columnCount( const QModelIndex& parent ) const
{
    Q_UNUSED( parent );
    return 4;
}


void DevicesModel::setBurningGroup( const QString& burningGroup )
{
    if( burningGroup != d->burningGroup ) {
        d->burningGroup = burningGroup;
        reset();
    }
}


void DevicesModel::update()
{
    // Remove from unselected devices list all devices
    // that are not present anymore in device manager
    QSet<Device::Device*> devices = d->deviceManager->allDevices().toSet();
    d->unselectedDevices.intersect( devices );
    reset();
}

} // namespace Setup
} // namespace K3b

#include "k3bsetupdevicesmodel.moc"
