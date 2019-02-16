/*
 *
 * Copyright (C) 2005-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
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

#include <KLocalizedString>
#include <KIO/Job>
#include <KMessageBox>
#include <KActionCollection>
#include <Solid/Block>
#include <Solid/Device>
#include <Solid/StorageAccess>

#include <QAction>
#include <QInputDialog>

class K3b::AppDeviceManager::Private
{
public:
    QAction* actionDiskInfo;
    QAction* actionUnmount;
    QAction* actionMount;
    QAction* actionEject;
    QAction* actionLoad;
    QAction* actionSetReadSpeed;

    K3b::Device::Device* currentDevice;
};

K3b::AppDeviceManager::AppDeviceManager( QObject* parent )
    : K3b::Device::DeviceManager( parent ),
      d( new Private )
{
    // setup actions
    d->actionDiskInfo = K3b::createAction(this, i18n("Media &Info"), "document-properties", 0, this, SLOT(diskInfo()),
                                         actionCollection(), "device_diskinfo");
    d->actionUnmount = K3b::createAction(this,  i18n("&Unmount"), "media-optical", 0, this, SLOT(unmountDisk()),
                                        actionCollection(), "device_unmount");
    d->actionMount = K3b::createAction(this, i18n("&Mount"), "media-optical", 0, this, SLOT(mountDisk()),
                                      actionCollection(), "device_mount");
    d->actionEject = K3b::createAction(this, i18n("&Eject"), "media-eject", 0, this, SLOT(ejectDisk()),
                                      actionCollection(), "device_eject");
    d->actionLoad = K3b::createAction(this, i18n("L&oad"), 0, 0, this, SLOT(loadDisk()),
                                     actionCollection(), "device_load");
//   QAction* actionUnlock = new QAction( i18n("Un&lock"), "", 0, this, SLOT(unlockDevice()),
// 				       actionCollection(), "device_unlock" );
//   QAction* actionlock = new QAction( i18n("Loc&k"), "", 0, this, SLOT(lockDevice()),
// 				     actionCollection(), "device_lock" );
    d->actionSetReadSpeed = K3b::createAction(this, i18n("Set Read Speed..."), 0, 0, this, SLOT(setReadSpeed()),
                                             actionCollection(), "device_set_read_speed" );

    d->actionDiskInfo->setToolTip( i18n("Display generic medium information") );
    d->actionDiskInfo->setStatusTip( d->actionDiskInfo->toolTip() );
    d->actionUnmount->setToolTip( i18n("Unmount the medium") );
    d->actionUnmount->setStatusTip( d->actionUnmount->toolTip() );
    d->actionMount->setToolTip( i18n("Mount the medium") );
    d->actionMount->setStatusTip( d->actionMount->toolTip() );
    d->actionEject->setToolTip( i18n("Eject the medium") );
    d->actionEject->setStatusTip( d->actionEject->toolTip() );
    d->actionLoad->setToolTip( i18n("(Re)Load the medium") );
    d->actionLoad->setStatusTip( d->actionLoad->toolTip() );
    d->actionSetReadSpeed->setToolTip( i18n("Force the drive's read speed") );
    d->actionSetReadSpeed->setStatusTip( d->actionSetReadSpeed->toolTip() );

    setXMLFile( "k3bdeviceui.rc", true );

    d->currentDevice = 0;
    slotMediumChanged( 0 );
}


K3b::AppDeviceManager::~AppDeviceManager()
{
    delete d;
}


void K3b::AppDeviceManager::setMediaCache( K3b::MediaCache* c )
{
    connect( c, SIGNAL(mediumChanged(K3b::Device::Device*)),
             this, SLOT(slotMediumChanged(K3b::Device::Device*)) );
}


K3b::Device::Device* K3b::AppDeviceManager::currentDevice() const
{
    return d->currentDevice;
}


void K3b::AppDeviceManager::clear()
{
    // make sure we do not use a deleted device
    setCurrentDevice( 0 );
    K3b::Device::DeviceManager::clear();
}

K3b::Device::Device* K3b::AppDeviceManager::addDevice( const Solid::Device& solidDev )
{
    K3b::Device::Device* dev = K3b::Device::DeviceManager::addDevice( solidDev );
    if( dev && currentDevice() == 0 ) {
        setCurrentDevice( dev );
    }
    return dev;
}

void K3b::AppDeviceManager::removeDevice( const Solid::Device& solidDev )
{
    if( const Solid::Block* blockDevice = solidDev.as<Solid::Block>() ) {
        if( findDevice( blockDevice->device() ) == currentDevice() )
            setCurrentDevice( 0 );

        K3b::Device::DeviceManager::removeDevice( solidDev );

        if( currentDevice() == 0 && !allDevices().isEmpty() )
            setCurrentDevice( allDevices().first() );
    }
}


void K3b::AppDeviceManager::slotMediumChanged( K3b::Device::Device* dev )
{
    if( dev == currentDevice() ) {

        d->actionDiskInfo->setEnabled( dev != 0 );
        d->actionEject->setEnabled( dev != 0 );
        d->actionLoad->setEnabled( dev != 0 );
        d->actionSetReadSpeed->setEnabled( dev != 0 );

        if( dev ) {
            bool mediumMountable = k3bappcore->mediaCache()->medium( dev ).content() & K3b::Medium::ContentData;
            d->actionMount->setEnabled( mediumMountable );
            d->actionUnmount->setEnabled( mediumMountable );

            disconnect( this, SLOT(slotMountChanged(bool,QString)) );
            disconnect( this, SLOT(slotMountFinished(Solid::ErrorType,QVariant,QString)) );
            disconnect( this, SLOT(slotUnmountFinished(Solid::ErrorType,QVariant,QString)) );

            Solid::StorageAccess* storage = dev->solidStorage();
            if( storage != 0 ) {
                connect( storage, SIGNAL(accessibilityChanged(bool,QString)),
                        this, SLOT(slotMountChanged(bool,QString)) );
                connect( storage, SIGNAL(setupDone(Solid::ErrorType,QVariant,QString)),
                        this, SLOT(slotMountFinished(Solid::ErrorType,QVariant,QString)) );
                connect( storage, SIGNAL(teardownDone(Solid::ErrorType,QVariant,QString)),
                        this, SLOT(slotUnmountFinished(Solid::ErrorType,QVariant,QString)) );
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


void K3b::AppDeviceManager::slotMountChanged( bool accessible, const QString& )
{
    d->actionMount->setVisible( !accessible );
    d->actionUnmount->setVisible( accessible );
}


void K3b::AppDeviceManager::slotMountFinished( Solid::ErrorType error, QVariant, const QString& )
{
    if( currentDevice() != 0 ) {
        Solid::StorageAccess* storage = currentDevice()->solidStorage();
        if( error == Solid::NoError && storage != 0 ) {
            qDebug() << "Device mounted at " << storage->filePath();
            emit mountFinished( storage->filePath() );
        }
    }
}

void K3b::AppDeviceManager::slotUnmountFinished( Solid::ErrorType error, QVariant, const QString& )
{
    emit unmountFinished( error == Solid::NoError );
}


void K3b::AppDeviceManager::setCurrentDevice( K3b::Device::Device* dev )
{
    if( dev != currentDevice() ) {
        d->currentDevice = dev;

        emit currentDeviceChanged( currentDevice() );
        slotMediumChanged( currentDevice() );
    }
}


void K3b::AppDeviceManager::diskInfo()
{
    if( currentDevice() ) {
        emit detectingDiskInfo( currentDevice() );
    }
}


void K3b::AppDeviceManager::unlockDevice()
{
    if( currentDevice() )
        K3b::Device::unblock( currentDevice() );
}


void K3b::AppDeviceManager::lockDevice()
{
    if( currentDevice() )
        K3b::Device::block( currentDevice() );
}


void K3b::AppDeviceManager::mountDisk()
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


void K3b::AppDeviceManager::unmountDisk()
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


void K3b::AppDeviceManager::ejectDisk()
{
    if ( currentDevice() )
        K3b::Device::eject( currentDevice() );
}


void K3b::AppDeviceManager::loadDisk()
{
    if( currentDevice() )
        K3b::Device::reload( currentDevice() );
}


void K3b::AppDeviceManager::setReadSpeed()
{
    if( currentDevice() ) {
        bool ok = false;
        int s = QInputDialog::getInt( nullptr,
                                      i18n("CD Read Speed"),
                                      i18n("<p>Please enter the preferred read speed for <b>%1</b>. "
                                           "This speed will be used for the currently mounted "
                                           "medium."
                                           "<p>This is especially useful to slow down the drive when "
                                           "watching movies which are read directly from the drive "
                                           "and the spinning noise is intrusive."
                                           "<p>Be aware that this has no influence on K3b since it will "
                                           "change the reading speed again when copying CDs or DVDs."
                                           ,currentDevice()->vendor() + ' ' + currentDevice()->description()),
                                      12,
                                      1,
                                      currentDevice()->maxReadSpeed(),
                                      1,
                                      &ok );
        if( ok ) {
            if( !currentDevice()->setSpeed( s*175, 0xFFFF ) )
                KMessageBox::error( 0, i18n("Setting the read speed failed.") );
        }
    }
}


void K3b::AppDeviceManager::diskInfo( K3b::Device::Device* dev )
{
    setCurrentDevice( dev );
    diskInfo();
}


void K3b::AppDeviceManager::unlockDevice( K3b::Device::Device* dev )
{
    setCurrentDevice( dev );
    unlockDevice();
}


void K3b::AppDeviceManager::lockDevice( K3b::Device::Device* dev )
{
    setCurrentDevice( dev );
    lockDevice();
}


void K3b::AppDeviceManager::mountDisk( K3b::Device::Device* dev )
{
    setCurrentDevice( dev );
    mountDisk();
}


void K3b::AppDeviceManager::unmountDisk( K3b::Device::Device* dev )
{
    setCurrentDevice( dev );
    unmountDisk();
}


void K3b::AppDeviceManager::ejectDisk( K3b::Device::Device* dev )
{
    setCurrentDevice( dev );
    ejectDisk();
}


void K3b::AppDeviceManager::loadDisk( K3b::Device::Device* dev )
{
    setCurrentDevice( dev );
    loadDisk();
}


void K3b::AppDeviceManager::setReadSpeed( K3b::Device::Device* dev )
{
    setCurrentDevice( dev );
    setReadSpeed();
}


