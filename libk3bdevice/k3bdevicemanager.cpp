/*
    SPDX-FileCopyrightText: 2003-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3bdevicemanager.h"
#include "k3bdevice.h"
#include "k3bdeviceglobals.h"
#include "k3bscsicommand.h"
#include "k3bmmc.h"

#include <config-k3b.h>

#include <KConfig>
#include <KConfigGroup>

#include <Solid/DeviceNotifier>
#include <Solid/DeviceInterface>
#include <Solid/OpticalDrive>
#include <Solid/Block>
#include <Solid/Device>
#ifdef Q_OS_NETBSD
#include <Solid/GenericInterface>
#endif

#include <QDebug>
#include <QString>
#include <QStringList>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryFile>

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



class K3b::Device::DeviceManager::Private
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



K3b::Device::DeviceManager::DeviceManager( QObject* parent )
    : QObject( parent ),
      d( new Private() )
{
    connect( Solid::DeviceNotifier::instance(), SIGNAL(deviceAdded(QString)),
             this, SLOT(slotSolidDeviceAdded(QString)) );
    connect( Solid::DeviceNotifier::instance(), SIGNAL(deviceRemoved(QString)),
             this, SLOT(slotSolidDeviceRemoved(QString)) );
}


K3b::Device::DeviceManager::~DeviceManager()
{
    qDeleteAll( d->allDevices );
    delete d;
}


void K3b::Device::DeviceManager::setCheckWritingModes( bool b )
{
    d->checkWritingModes = b;
}


K3b::Device::Device* K3b::Device::DeviceManager::deviceByName( const QString& name )
{
    return findDevice( name );
}


K3b::Device::Device* K3b::Device::DeviceManager::findDevice( const QString& devicename )
{
    if( devicename.isEmpty() ) {
        qDebug() << "(K3b::Device::DeviceManager) request for empty device!";
        return nullptr;
    }

    foreach( Device* dev, d->allDevices ) {
        if( dev->blockDeviceName() == devicename )
            return dev;
    }

    return nullptr;
}


K3b::Device::Device* K3b::Device::DeviceManager::findDeviceByUdi( const QString& udi )
{
    foreach( Device* dev, d->allDevices ) {
        if ( dev->solidDevice().udi() == udi )
            return dev;
    }
    return nullptr;
}


QList<K3b::Device::Device*> K3b::Device::DeviceManager::cdWriter() const
{
    return d->cdWriter;
}

QList<K3b::Device::Device*> K3b::Device::DeviceManager::cdReader() const
{
    return d->cdReader;
}

QList<K3b::Device::Device*> K3b::Device::DeviceManager::dvdWriter() const
{
    return d->dvdWriter;
}

QList<K3b::Device::Device*> K3b::Device::DeviceManager::dvdReader() const
{
    return d->dvdReader;
}

QList<K3b::Device::Device*> K3b::Device::DeviceManager::blueRayReader() const
{
    return d->bdReader;
}

QList<K3b::Device::Device*> K3b::Device::DeviceManager::blueRayWriters() const
{
    return d->bdWriter;
}

QList<K3b::Device::Device*> K3b::Device::DeviceManager::burningDevices() const
{
    return cdWriter();
}


QList<K3b::Device::Device*> K3b::Device::DeviceManager::readingDevices() const
{
    return cdReader();
}


QList<K3b::Device::Device*> K3b::Device::DeviceManager::allDevices() const
{
    return d->allDevices;
}


int K3b::Device::DeviceManager::scanBus()
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


K3b::Device::Device* K3b::Device::DeviceManager::checkDevice( const Solid::Device& dev )
{
    if ( dev.is<Solid::OpticalDrive>() ) {
        return addDevice( dev );
    }
    else {
        return nullptr;
    }
}


void K3b::Device::DeviceManager::printDevices()
{
    qDebug() << "Devices:" << Qt::endl
             << "------------------------------" << Qt::endl;
    Q_FOREACH( Device* dev, d->allDevices ) {
        qDebug() << "Blockdevice:    " << dev->blockDeviceName() << Qt::endl
                 << "Vendor:         " << dev->vendor() << Qt::endl
                 << "Description:    " << dev->description() << Qt::endl
                 << "Version:        " << dev->version() << Qt::endl
                 << "Write speed:    " << dev->maxWriteSpeed() << Qt::endl
                 << "Profiles:       " << mediaTypeString( dev->supportedProfiles() ) << Qt::endl
                 << "Read Cap:       " << mediaTypeString( dev->readCapabilities() ) << Qt::endl
                 << "Write Cap:      " << mediaTypeString( dev->writeCapabilities() ) << Qt::endl
                 << "Writing modes:  " << writingModeString( dev->writingModes() ) << Qt::endl
                 << "------------------------------" << Qt::endl;
    }
}


void K3b::Device::DeviceManager::clear()
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
    QList<Device*> devicesToDelete( d->allDevices );
    d->allDevices.clear();

    emit changed( this );
    emit changed();

    qDeleteAll( devicesToDelete );
}


bool K3b::Device::DeviceManager::readConfig( const KConfigGroup& c )
{
    //
    // New configuration format since K3b 0.11.94
    // for details see saveConfig()
    //

    //
    // Iterate over all devices and check if we have a config entry
    //
    for( QList<K3b::Device::Device*>::iterator it = d->allDevices.begin(); it != d->allDevices.end(); ++it ) {
        K3b::Device::Device* dev = *it;

        QString configEntryName = dev->vendor() + ' ' + dev->description();
        QStringList list = c.readEntry( configEntryName, QStringList() );
        if( !list.isEmpty() ) {
            qDebug() << "(K3b::Device::DeviceManager) found config entry for devicetype: " << configEntryName;

            dev->setMaxReadSpeed( list[0].toInt() );
            if( list.count() > 1 )
                dev->setMaxWriteSpeed( list[1].toInt() );
        }
    }

    return true;
}


bool K3b::Device::DeviceManager::saveConfig( KConfigGroup c )
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
        QString configEntryName = dev->vendor() + ' ' + dev->description();
        QStringList list;
        list << QString::number(dev->maxReadSpeed())
             << QString::number(dev->maxWriteSpeed());

        c.writeEntry( configEntryName, list );
    }

    return true;
}


K3b::Device::Device* K3b::Device::DeviceManager::addDevice( const Solid::Device& solidDevice )
{
    if( const Solid::Block* blockDevice = solidDevice.as<Solid::Block>() ) {
#ifndef Q_OS_NETBSD
        if( !findDevice( blockDevice->device() ) )
#else
        if( !findDevice( solidDevice.as<Solid::GenericInterface>()->propertyExists("block.netbsd.raw_device") ? solidDevice.as<Solid::GenericInterface>()->property("block.netbsd.raw_device").toString() : blockDevice->device() ) )
#endif
            return addDevice( new K3b::Device::Device( solidDevice ) );
        else
            qDebug() << "(K3b::Device::DeviceManager) dev " << blockDevice->device()  << " already found";
    }
    return nullptr;
}


K3b::Device::Device* K3b::Device::DeviceManager::addDevice( K3b::Device::Device* device )
{
    const QString devicename = device->blockDeviceName();

    if( !device->init() ) {
        qDebug() << "Could not initialize device " << devicename;
        delete device;
        return nullptr;
    }

    if( device ) {
        d->allDevices.append( device );

        // not every drive is able to read CDs
        // there are some 1st generation DVD writer that cannot
        if( device->type() & K3b::Device::DEVICE_CD_ROM )
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
            qDebug() << "(K3b::Device::DeviceManager) setting current write speed of device "
                     << device->blockDeviceName()
                     << " to " << device->maxWriteSpeed();
            device->setCurrentWriteSpeed( device->maxWriteSpeed() );
        }

        emit changed( this );
        emit changed();
    }

    return device;
}


void K3b::Device::DeviceManager::removeDevice( const Solid::Device& dev )
{
    if( const Solid::Block* blockDevice = dev.as<Solid::Block>() ) {
        if( Device* device = findDevice( blockDevice->device() ) ) {
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
}


void K3b::Device::DeviceManager::slotSolidDeviceAdded( const QString& udi )
{
    qDebug() << udi;
    checkDevice( Solid::Device( udi ) );
}


void K3b::Device::DeviceManager::slotSolidDeviceRemoved( const QString& udi )
{
    qDebug() << udi;
    Solid::Device solidDev( udi );
    if ( solidDev.isDeviceInterface( Solid::DeviceInterface::OpticalDrive ) ) {
        if ( solidDev.is<Solid::OpticalDrive>() ) {
            removeDevice( solidDev );
        }
    }
}

#include "moc_k3bdevicemanager.cpp"
