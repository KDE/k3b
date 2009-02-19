/*
 *
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bdevicemanager.h"
#include "k3bdevice.h"
#include "k3bdeviceglobals.h"
#include "k3bscsicommand.h"
#include "k3bmmc.h"

#include <config-k3b.h>

#include "kdebug.h"

#include <qstring.h>
#include <qstringlist.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qregexp.h>

#include <k3process.h>
#include <kapplication.h>
#include <kconfig.h>
#include <kconfiggroup.h>
#include <ktemporaryfile.h>

#include <Solid/DeviceNotifier>
#include <Solid/DeviceInterface>
#include <Solid/OpticalDrive>
#include <Solid/Block>
#include <Solid/Device>

#include <iostream>
#include <limits.h>
#include <assert.h>

#ifdef Q_OS_FREEBSD
#include <sys/param.h>
#include <sys/ucred.h>
#include <osreldate.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#ifdef HAVE_RESMGR
#include <resmgr.h>
#endif

#ifdef Q_OS_LINUX
#include <sys/utsname.h>
/* Fix definitions for 2.5 kernels */
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,70)
typedef unsigned char u8;
#endif

#undef __STRICT_ANSI__
#include <asm/types.h>
#define __STRICT_ANSI__

#include <scsi/scsi.h>
#include <linux/major.h>


#ifndef SCSI_DISK_MAJOR
#define SCSI_DISK_MAJOR(M) ((M) == SCSI_DISK0_MAJOR ||                  \
                            ((M) >= SCSI_DISK1_MAJOR && (M) <= SCSI_DISK7_MAJOR) || \
                            ((M) >= SCSI_DISK8_MAJOR && (M) <= SCSI_DISK15_MAJOR))
#endif

#ifndef SCSI_BLK_MAJOR
#define SCSI_BLK_MAJOR(M)                       \
    (SCSI_DISK_MAJOR(M)                         \
     || (M) == SCSI_CDROM_MAJOR)
#endif

#ifndef SCSI_GENERIC_MAJOR
#define SCSI_GENERIC_MAJOR 21
#endif

#endif // Q_OS_LINUX


#ifdef Q_OS_FREEBSD
#include <cam/cam.h>
#include <cam/scsi/scsi_pass.h>
#include <camlib.h>
#endif

#ifdef Q_OS_NETBSD
#include <sys/scsiio.h>
#endif



class K3bDevice::DeviceManager::Private
{
public:
    QList<Device*> allDevices;
    QList<Device*> cdReader;
    QList<Device*> cdWriter;
    QList<Device*> dvdReader;
    QList<Device*> dvdWriter;
    QList<Device*> bdReader;
    QList<Device*> bdWriter;

    bool checkWritingModes;
};



K3bDevice::DeviceManager::DeviceManager( QObject* parent )
    : QObject( parent ),
      d( new Private() )
{
    connect( Solid::DeviceNotifier::instance(), SIGNAL( deviceAdded( const QString& ) ),
             this, SLOT( slotSolidDeviceAdded( const QString& ) ) );
    connect( Solid::DeviceNotifier::instance(), SIGNAL( deviceRemoved( const QString& ) ),
             this, SLOT( slotSolidDeviceRemoved( const QString& ) ) );
}


K3bDevice::DeviceManager::~DeviceManager()
{
    qDeleteAll( d->allDevices );
    delete d;
}


void K3bDevice::DeviceManager::setCheckWritingModes( bool b )
{
    d->checkWritingModes = b;
}


K3bDevice::Device* K3bDevice::DeviceManager::deviceByName( const QString& name )
{
    return findDevice( name );
}


K3bDevice::Device* K3bDevice::DeviceManager::findDevice( const QString& devicename )
{
    if( devicename.isEmpty() ) {
        kDebug() << "(K3bDevice::DeviceManager) request for empty device!";
        return 0;
    }

    foreach( Device* dev, d->allDevices ) {
        if( dev->blockDeviceName() == devicename )
            return dev;
    }

    return 0;
}


K3bDevice::Device* K3bDevice::DeviceManager::findDeviceByUdi( const QString& udi )
{
    foreach( Device* dev, d->allDevices ) {
        if ( dev->solidDevice().udi() == udi )
            return dev;
    }
    return 0;
}


QList<K3bDevice::Device*> K3bDevice::DeviceManager::cdWriter() const
{
    return d->cdWriter;
}

QList<K3bDevice::Device*> K3bDevice::DeviceManager::cdReader() const
{
    return d->cdReader;
}

QList<K3bDevice::Device*> K3bDevice::DeviceManager::dvdWriter() const
{
    return d->dvdWriter;
}

QList<K3bDevice::Device*> K3bDevice::DeviceManager::dvdReader() const
{
    return d->dvdReader;
}

QList<K3bDevice::Device*> K3bDevice::DeviceManager::blueRayReader() const
{
    return d->bdReader;
}

QList<K3bDevice::Device*> K3bDevice::DeviceManager::blueRayWriters() const
{
    return d->bdWriter;
}

QList<K3bDevice::Device*> K3bDevice::DeviceManager::burningDevices() const
{
    return cdWriter();
}


QList<K3bDevice::Device*> K3bDevice::DeviceManager::readingDevices() const
{
    return cdReader();
}


QList<K3bDevice::Device*> K3bDevice::DeviceManager::allDevices() const
{
    return d->allDevices;
}


int K3bDevice::DeviceManager::scanBus()
{
    int cnt = 0;

    QList<Solid::Device> dl = Solid::Device::listFromType( Solid::DeviceInterface::OpticalDrive );
    Q_FOREACH( const Solid::Device& solidDev, dl ) {
        if ( checkDevice( solidDev ) ) {
            ++cnt;
        }
    }

    return cnt;
}


K3bDevice::Device* K3bDevice::DeviceManager::checkDevice( const Solid::Device& dev )
{
    if ( dev.is<Solid::OpticalDrive>() ) {
        return addDevice( dev );
    }
    else {
        return 0;
    }
}


void K3bDevice::DeviceManager::printDevices()
{
    kDebug() << "Devices:" << endl
             << "------------------------------" << endl;
    Q_FOREACH( Device* dev, d->allDevices ) {
        kDebug() << "Blockdevice:    " << dev->blockDeviceName() << endl
                 << "Vendor:         " << dev->vendor() << endl
                 << "Description:    " << dev->description() << endl
                 << "Version:        " << dev->version() << endl
                 << "Write speed:    " << dev->maxWriteSpeed() << endl
                 << "Profiles:       " << mediaTypeString( dev->supportedProfiles() ) << endl
                 << "Read Cap:       " << mediaTypeString( dev->readCapabilities() ) << endl
                 << "Write Cap:      " << mediaTypeString( dev->writeCapabilities() ) << endl
                 << "Writing modes:  " << writingModeString( dev->writingModes() ) << endl
                 << "------------------------------" << endl;
    }
}


void K3bDevice::DeviceManager::clear()
{
    // clear current devices
    d->cdReader.clear();
    d->cdWriter.clear();
    d->dvdReader.clear();
    d->dvdWriter.clear();
    d->bdReader.clear();
    d->bdWriter.clear();

    // to make sure no one crashes lets keep the devices around until the changed
    // signals return
    qDeleteAll( d->allDevices );
    d->allDevices.clear();

    emit changed( this );
    emit changed();
}


bool K3bDevice::DeviceManager::readConfig( const KConfigGroup& c )
{
    //
    // New configuration format since K3b 0.11.94
    // for details see saveConfig()
    //

    //
    // Iterate over all devices and check if we have a config entry
    //
    for( QList<K3bDevice::Device*>::iterator it = d->allDevices.begin(); it != d->allDevices.end(); ++it ) {
        K3bDevice::Device* dev = *it;

        QString configEntryName = dev->vendor() + " " + dev->description();
        QStringList list = c.readEntry( configEntryName, QStringList() );
        if( !list.isEmpty() ) {
            kDebug() << "(K3bDevice::DeviceManager) found config entry for devicetype: " << configEntryName;

            dev->setMaxReadSpeed( list[0].toInt() );
            if( list.count() > 1 )
                dev->setMaxWriteSpeed( list[1].toInt() );
        }
    }

    return true;
}


bool K3bDevice::DeviceManager::saveConfig( KConfigGroup c )
{
    //
    // New configuration format since K3b 0.11.94
    //
    // We save a device search path which contains all device nodes
    // where devices could be found including the old search path.
    // This way also for example a manually added USB device will be
    // found between sessions.
    // Then we do not save the device settings (writing speed, cdrdao driver)
    // for every single device but for every device type.
    // This also makes sure device settings are kept between sessions
    //

    Q_FOREACH( Device* dev, d->allDevices ) {

        // save the device type settings
        QString configEntryName = dev->vendor() + " " + dev->description();
        QStringList list;
        list << QString::number(dev->maxReadSpeed())
             << QString::number(dev->maxWriteSpeed());

        c.writeEntry( configEntryName, list );
    }

    return true;
}


K3bDevice::Device* K3bDevice::DeviceManager::addDevice( const Solid::Device& solidDevice )
{
    if ( findDevice( solidDevice.as<Solid::Block>()->device() ) ) {
        kDebug() << "(K3bDevice::DeviceManager) dev " << solidDevice.as<Solid::Block>()->device()  << " already found";
        return 0;
    }

    return addDevice( new K3bDevice::Device( solidDevice ) );
}


K3bDevice::Device* K3bDevice::DeviceManager::addDevice( K3bDevice::Device* device )
{
    const QString devicename = device->blockDeviceName();

    if( !device->init() ) {
        kDebug() << "Could not initialize device " << devicename;
        delete device;
        return 0;
    }

    if( device ) {
        d->allDevices.append( device );

        // not every drive is able to read CDs
        // there are some 1st generation DVD writer that cannot
        if( device->type() & K3bDevice::DEVICE_CD_ROM )
            d->cdReader.append( device );
        if( device->readsDvd() )
            d->dvdReader.append( device );
        if( device->writesCd() )
            d->cdWriter.append( device );
        if( device->writesDvd() )
            d->dvdWriter.append( device );
        if( device->readCapabilities() & MEDIA_BD_ALL )
            d->bdReader.append( device );
        if( device->writeCapabilities() & MEDIA_BD_ALL )
            d->bdWriter.append( device );

        if( device->writesCd() ) {
            // default to max write speed
            kDebug() << "(K3bDevice::DeviceManager) setting current write speed of device "
                     << device->blockDeviceName()
                     << " to " << device->maxWriteSpeed() << endl;
            device->setCurrentWriteSpeed( device->maxWriteSpeed() );
        }

        emit changed( this );
        emit changed();
    }

    return device;
}


void K3bDevice::DeviceManager::removeDevice( const Solid::Device& dev )
{
    if( Device* device = findDevice( dev.as<Solid::Block>()->device() ) ) {
        d->cdReader.removeAll( device );
        d->dvdReader.removeAll( device );
        d->bdReader.removeAll( device );
        d->cdWriter.removeAll( device );
        d->dvdWriter.removeAll( device );
        d->bdWriter.removeAll( device );
        d->allDevices.removeAll( device );

        emit changed( this );
        emit changed();

        delete device;
    }
}


void K3bDevice::DeviceManager::slotSolidDeviceAdded( const QString& udi )
{
    kDebug() << udi;
    checkDevice( Solid::Device( udi ) );
}


void K3bDevice::DeviceManager::slotSolidDeviceRemoved( const QString& udi )
{
    kDebug() << udi;
    Solid::Device solidDev( udi );
    if ( solidDev.isDeviceInterface( Solid::DeviceInterface::OpticalDrive ) ) {
        if ( solidDev.is<Solid::OpticalDrive>() ) {
            removeDevice( solidDev );
        }
    }
}

#include "k3bdevicemanager.moc"
