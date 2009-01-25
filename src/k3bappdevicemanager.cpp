/*
 *
 * Copyright (C) 2005-2008 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bappdevicemanager.h"

#include "k3baction.h"
#include "k3bdevice.h"
#include "k3bdevicehandler.h"
#include "k3bglobals.h"
#include "k3bapplication.h"
#include "k3bmediacache.h"

#include <KAction>
#include <KActionCollection>
#include <KInputDialog>
#include <KIO/Job>
#include <KLocale>
#include <KMessageBox>

#include <Solid/Block>
#include <Solid/Device>
#include <Solid/StorageAccess>


class K3bAppDeviceManager::Private
{
public:
    KAction* actionDiskInfo;
    KAction* actionUnmount;
    KAction* actionMount;
    KAction* actionEject;
    KAction* actionLoad;
    KAction* actionSetReadSpeed;

    K3bDevice::Device* currentDevice;
};

K3bAppDeviceManager::K3bAppDeviceManager( QObject* parent )
    : K3bDevice::DeviceManager( parent ),
      d( new Private )
{
    // setup actions
    d->actionDiskInfo = K3b::createAction(this, i18n("Media &Info"), "document-properties", 0, this, SLOT(diskInfo()),
                                         actionCollection(), "device_diskinfo");
    d->actionUnmount = K3b::createAction(this,  i18n("&Unmount"), "media-optical", 0, this, SLOT(unmountDisk()),
                                        actionCollection(), "device_unmount");
    d->actionMount = K3b::createAction(this, i18n("&Mount"), "media-optical", 0, this, SLOT(mountDisk()),
                                      actionCollection(), "device_mount");
    d->actionEject = K3b::createAction(this, i18n("&Eject"), 0, 0, this, SLOT(ejectDisk()),
                                      actionCollection(), "device_eject");
    d->actionLoad = K3b::createAction(this, i18n("L&oad"), 0, 0, this, SLOT(loadDisk()),
                                     actionCollection(), "device_load");
//   KAction* actionUnlock = new KAction( i18n("Un&lock"), "", 0, this, SLOT(unlockDevice()),
// 				       actionCollection(), "device_unlock" );
//   KAction* actionlock = new KAction( i18n("Loc&k"), "", 0, this, SLOT(lockDevice()),
// 				     actionCollection(), "device_lock" );
    d->actionSetReadSpeed = K3b::createAction(this, i18n("Set Read Speed..."), 0, 0, this, SLOT(setReadSpeed()),
                                             actionCollection(), "device_set_read_speed" );

    d->actionDiskInfo->setToolTip( i18n("Display generic medium information") );
    d->actionUnmount->setToolTip( i18n("Unmount the medium") );
    d->actionMount->setToolTip( i18n("Mount the medium") );
    d->actionEject->setToolTip( i18n("Eject the medium") );
    d->actionLoad->setToolTip( i18n("(Re)Load the medium") );
    d->actionSetReadSpeed->setToolTip( i18n("Force the drive's read speed") );

    setXMLFile( "k3bdeviceui.rc", true );

    d->currentDevice = 0;
    slotMediumChanged( 0 );
}


K3bAppDeviceManager::~K3bAppDeviceManager()
{
    delete d;
}


void K3bAppDeviceManager::setMediaCache( K3bMediaCache* c )
{
    connect( c, SIGNAL(mediumChanged(K3bDevice::Device*)),
             this, SLOT(slotMediumChanged(K3bDevice::Device*)) );
}


K3bDevice::Device* K3bAppDeviceManager::currentDevice() const
{
    return d->currentDevice;
}


void K3bAppDeviceManager::clear()
{
    // make sure we do not use a deleted device
    setCurrentDevice( 0 );
    K3bDevice::DeviceManager::clear();
}

K3bDevice::Device* K3bAppDeviceManager::addDevice( const Solid::Device& solidDev )
{
    K3bDevice::Device* dev = K3bDevice::DeviceManager::addDevice( solidDev );
    if( dev && currentDevice() == 0 ) {
        setCurrentDevice( dev );
    }
    return dev;
}

void K3bAppDeviceManager::removeDevice( const Solid::Device& solidDev )
{
    if( findDevice( solidDev.as<Solid::Block>()->device() ) == currentDevice() )
        setCurrentDevice( 0 );
    
    K3bDevice::DeviceManager::removeDevice( solidDev );

    if( currentDevice() == 0 && !allDevices().isEmpty() )
        setCurrentDevice( allDevices().first() );
}


void K3bAppDeviceManager::slotMediumChanged( K3bDevice::Device* dev )
{
    if( dev == currentDevice() ) {
        
        d->actionDiskInfo->setEnabled( dev != 0 );
        d->actionEject->setEnabled( dev != 0 );
        d->actionLoad->setEnabled( dev != 0 );
        d->actionSetReadSpeed->setEnabled( dev != 0 );
        
        if( dev ) {
            bool mediumMountable = k3bappcore->mediaCache()->medium( dev ).content() & K3bMedium::CONTENT_DATA;
            d->actionMount->setEnabled( mediumMountable );
            d->actionUnmount->setEnabled( mediumMountable );
            
            disconnect( this, SLOT(slotMountChanged(bool,const QString&)) );
            disconnect( this, SLOT(slotMountFinished(Solid::ErrorType,QVariant,const QString&)) );
            disconnect( this, SLOT(slotUnmountFinished(Solid::ErrorType,QVariant,const QString&)) );

            Solid::StorageAccess* storage = dev->solidStorage();
            if( storage != 0 ) {
                connect( storage, SIGNAL(accessibilityChanged(bool,const QString&)),
                        this, SLOT(slotMountChanged(bool,const QString&)) );
                connect( storage, SIGNAL(setupDone(Solid::ErrorType,QVariant,const QString&)),
                        this, SLOT(slotMountFinished(Solid::ErrorType,QVariant,const QString&)) );
                connect( storage, SIGNAL(teardownDone(Solid::ErrorType,QVariant,const QString&)),
                        this, SLOT(slotUnmountFinished(Solid::ErrorType,QVariant,const QString&)) );
                d->actionMount->setVisible( !storage->isAccessible() );
                d->actionUnmount->setVisible( storage->isAccessible() );
            }
            else {
                d->actionMount->setVisible( true );
                d->actionUnmount->setVisible( false );
            }
        }
        else {
            d->actionMount->setVisible( true );
            d->actionUnmount->setVisible( false );
            d->actionMount->setEnabled( false );
            d->actionUnmount->setEnabled( false );
        }
    }
}


void K3bAppDeviceManager::slotMountChanged( bool accessible, const QString& )
{
    d->actionMount->setVisible( !accessible );
    d->actionUnmount->setVisible( accessible );
}


void K3bAppDeviceManager::slotMountFinished( Solid::ErrorType error, QVariant, const QString& )
{
    if( currentDevice() != 0 ) {
        Solid::StorageAccess* storage = currentDevice()->solidStorage();
        if( error == Solid::NoError && storage != 0 ) {
            kDebug() << "Device mounted at " << storage->filePath();
            emit mountFinished( storage->filePath() );
        }
    }
}

void K3bAppDeviceManager::slotUnmountFinished( Solid::ErrorType error, QVariant, const QString& )
{
    emit unmountFinished( error == Solid::NoError );
}


void K3bAppDeviceManager::setCurrentDevice( K3bDevice::Device* dev )
{
    if( dev != currentDevice() ) {
        d->currentDevice = dev;

        emit currentDeviceChanged( currentDevice() );
        slotMediumChanged( currentDevice() );
    }
}


void K3bAppDeviceManager::diskInfo()
{
    if( currentDevice() ) {
        emit detectingDiskInfo( currentDevice() );
    }
}


void K3bAppDeviceManager::unlockDevice()
{
    if( currentDevice() )
        K3bDevice::unblock( currentDevice() );
}


void K3bAppDeviceManager::lockDevice()
{
    if( currentDevice() )
        K3bDevice::block( currentDevice() );
}


void K3bAppDeviceManager::mountDisk()
{
    if( currentDevice() ) {
        Solid::StorageAccess* storage = currentDevice()->solidStorage();
        if( storage != 0 ) {
            if( storage->isAccessible() )
                emit mountFinished( storage->filePath() );
            else
                storage->setup();
        }
    }
}


void K3bAppDeviceManager::unmountDisk()
{
    if ( currentDevice() ) {
        Solid::StorageAccess* storage = currentDevice()->solidStorage();
        if( storage != 0 ) {
            if( storage->isAccessible() )
                storage->teardown();
            else
                emit unmountFinished( true );
        }
    }
}


void K3bAppDeviceManager::ejectDisk()
{
    // FIXME: make this non-blocking
    if ( currentDevice() )
        K3b::eject( currentDevice() ); // just ignore errors here
}


void K3bAppDeviceManager::loadDisk()
{
    if( currentDevice() )
        K3bDevice::reload( currentDevice() );
}


void K3bAppDeviceManager::setReadSpeed()
{
    if( currentDevice() ) {
        bool ok = false;
        int s = KInputDialog::getInteger( i18n("CD Read Speed"),
                                          i18n("<p>Please enter the preferred read speed for <b>%1</b>. "
                                               "This speed will be used for the currently mounted "
                                               "medium."
                                               "<p>This is especially useful to slow down the drive when "
                                               "watching movies which are read directly from the drive "
                                               "and the spinning noise is intrusive."
                                               "<p>Be aware that this has no influence on K3b since it will "
                                               "change the reading speed again when copying CDs or DVDs."
                                               ,currentDevice()->vendor() + " " + currentDevice()->description()),
                                          12,
                                          1,
                                          currentDevice()->maxReadSpeed(),
                                          1,
                                          10,
                                          &ok,
                                          0 );
        if( ok ) {
            if( !currentDevice()->setSpeed( s*175, 0xFFFF ) )
                KMessageBox::error( 0, i18n("Setting the read speed failed.") );
        }
    }
}


void K3bAppDeviceManager::diskInfo( K3bDevice::Device* dev )
{
    setCurrentDevice( dev );
    diskInfo();
}


void K3bAppDeviceManager::unlockDevice( K3bDevice::Device* dev )
{
    setCurrentDevice( dev );
    unlockDevice();
}


void K3bAppDeviceManager::lockDevice( K3bDevice::Device* dev )
{
    setCurrentDevice( dev );
    lockDevice();
}


void K3bAppDeviceManager::mountDisk( K3bDevice::Device* dev )
{
    setCurrentDevice( dev );
    mountDisk();
}


void K3bAppDeviceManager::unmountDisk( K3bDevice::Device* dev )
{
    setCurrentDevice( dev );
    unmountDisk();
}


void K3bAppDeviceManager::ejectDisk( K3bDevice::Device* dev )
{
    setCurrentDevice( dev );
    ejectDisk();
}


void K3bAppDeviceManager::loadDisk( K3bDevice::Device* dev )
{
    setCurrentDevice( dev );
    loadDisk();
}


void K3bAppDeviceManager::setReadSpeed( K3bDevice::Device* dev )
{
    setCurrentDevice( dev );
    setReadSpeed();
}

#include "k3bappdevicemanager.moc"
