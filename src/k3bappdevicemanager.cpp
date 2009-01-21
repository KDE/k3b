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
#include <KMenu>
#include <KMessageBox>

#include <Solid/Block>
#include <Solid/Device>
#include <Solid/StorageAccess>


K3bAppDeviceManager::K3bAppDeviceManager( QObject* parent )
    : K3bDevice::DeviceManager( parent ),
      m_currentDevice(0),
      m_ejectRequested(false)
{
    // setup actions
    m_actionDiskInfo = K3b::createAction(this, i18n("Media &Info"), "document-properties", 0, this, SLOT(diskInfo()),
                                         actionCollection(), "device_diskinfo");
    // FIXME (jpetso): combine "Unmount" and "Mount" into one toggling entry
    m_actionUnmount = K3b::createAction(this,  i18n("&Unmount"), "media-optical", 0, this, SLOT(unmountDisk()),
                                        actionCollection(), "device_unmount");
    m_actionMount = K3b::createAction(this, i18n("&Mount"), "media-optical", 0, this, SLOT(mountDisk()),
                                      actionCollection(), "device_mount");
    m_actionEject = K3b::createAction(this, i18n("&Eject"), 0, 0, this, SLOT(ejectDisk()),
                                      actionCollection(), "device_eject");
    m_actionLoad = K3b::createAction(this, i18n("L&oad"), 0, 0, this, SLOT(loadDisk()),
                                     actionCollection(), "device_load");
//   KAction* actionUnlock = new KAction( i18n("Un&lock"), "", 0, this, SLOT(unlockDevice()),
// 				       actionCollection(), "device_unlock" );
//   KAction* actionlock = new KAction( i18n("Loc&k"), "", 0, this, SLOT(lockDevice()),
// 				     actionCollection(), "device_lock" );
    m_actionSetReadSpeed = K3b::createAction(this, i18n("Set Read Speed..."), 0, 0, this, SLOT(setReadSpeed()),
                                             actionCollection(), "device_set_read_speed" );

    m_actionDiskInfo->setToolTip( i18n("Display generic medium information") );
    m_actionUnmount->setToolTip( i18n("Unmount the medium") );
    m_actionMount->setToolTip( i18n("Mount the medium") );
    m_actionEject->setToolTip( i18n("Eject the medium") );
    m_actionLoad->setToolTip( i18n("(Re)Load the medium") );
    m_actionSetReadSpeed->setToolTip( i18n("Force the drive's read speed") );

    setXMLFile( "k3bdeviceui.rc", true );

    slotMediumChanged( 0 );
}


void K3bAppDeviceManager::setMediaCache( K3bMediaCache* c )
{
    connect( c, SIGNAL(mediumChanged(K3bDevice::Device*)),
             this, SLOT(slotMediumChanged(K3bDevice::Device*)) );
}


K3bDevice::Device* K3bAppDeviceManager::currentDevice() const
{
    return m_currentDevice;
}


void K3bAppDeviceManager::clear()
{
    // make sure we do not use a deleted device
    setCurrentDevice( 0 );
    K3bDevice::DeviceManager::clear();
}


void K3bAppDeviceManager::removeDevice( const Solid::Device& dev )
{
    if( findDevice( dev.as<Solid::Block>()->device() ) == currentDevice() )
        setCurrentDevice( 0 );
    
    K3bDevice::DeviceManager::removeDevice( dev );

    if( currentDevice() == 0 && !allDevices().isEmpty() )
        setCurrentDevice( allDevices().first() );
}


K3bAppDeviceManager::~K3bAppDeviceManager()
{
}


void K3bAppDeviceManager::slotMediumChanged( K3bDevice::Device* dev )
{
    m_actionDiskInfo->setEnabled( dev != 0 );
    m_actionUnmount->setEnabled( dev != 0 );
    m_actionMount->setEnabled( dev != 0 );
    m_actionEject->setEnabled( dev != 0 );
    m_actionLoad->setEnabled( dev != 0 );
    m_actionSetReadSpeed->setEnabled( dev != 0 );

    if( dev && dev == currentDevice() ) {
        bool mediumMountable = k3bappcore->mediaCache()->medium( dev ).content() & K3bMedium::CONTENT_DATA;
        m_actionMount->setEnabled( mediumMountable );
        m_actionUnmount->setEnabled( mediumMountable );

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
            m_actionMount->setVisible( !storage->isAccessible() );
            m_actionUnmount->setVisible( storage->isAccessible() );
        }
    }
}


void K3bAppDeviceManager::slotMountChanged( bool accessible, const QString& )
{
    m_actionMount->setVisible( !accessible );
    m_actionUnmount->setVisible( accessible );
}


void K3bAppDeviceManager::slotMountFinished( Solid::ErrorType error, QVariant, const QString& )
{
    if( currentDevice() != 0 ) {
        Solid::StorageAccess* storage = currentDevice()->solidStorage();
        if( error == Solid::NoError && storage != 0 ) {
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
        m_currentDevice = dev;

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
