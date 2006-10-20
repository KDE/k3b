/* 
 *
 * $Id: sourceheader 380067 2005-01-19 13:03:46Z trueg $
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bappdevicemanager.h"

#include <k3bdevice.h>
#include <k3bdevicehandler.h>

#include <kaction.h>
#include <kinputdialog.h>
#include <kmessagebox.h>
#include <kinputdialog.h>
#include <kio/job.h>
#include <klocale.h>
#include <kio/global.h>


K3bAppDeviceManager::K3bAppDeviceManager( QObject* parent, const char* name )
  : K3bDevice::DeviceManager( parent, name ),
    m_currentDevice(0),
    m_ejectRequested(false)
{
  // FIXME: should we pass the mainwindow as watch window here?
  //        Is there a proper way to insert this actioncollection into the mainwindow's?
  m_actionCollection = new KActionCollection( this );

  // setup actions
  KActionMenu* devicePopupMenu = new KActionMenu( m_actionCollection, "device_popup" );
  KAction* actionDiskInfo = new KAction( i18n("Media &Info"), "info", 0, this, SLOT(diskInfo()),
					 m_actionCollection, "device_diskinfo");
  KAction* actionUnmount = new KAction( i18n("&Unmount"), "cdrom_unmount", 0, this, SLOT(unmountDisk()),
					m_actionCollection, "device_unmount");
  KAction* actionMount = new KAction( i18n("&Mount"), "cdrom_mount", 0, this, SLOT(mountDisk()),
				      m_actionCollection, "device_mount");
  KAction* actionEject = new KAction( i18n("&Eject"), "", 0, this, SLOT(ejectDisk()),
					m_actionCollection, "device_eject");
  KAction* actionLoad = new KAction( i18n("L&oad"), "", 0, this, SLOT(loadDisk()),
					m_actionCollection, "device_load");
  KAction* actionUnlock = new KAction( i18n("Un&lock"), "", 0, this, SLOT(unlockDevice()),
				       m_actionCollection, "device_unlock" );
  KAction* actionlock = new KAction( i18n("Loc&k"), "", 0, this, SLOT(lockDevice()),
				     m_actionCollection, "device_lock" );
  KAction* actionSetReadSpeed = new KAction( i18n("Set Read Speed..."), "", 0, this, SLOT(setReadSpeed()),
					     m_actionCollection, "device_set_read_speed" );

  devicePopupMenu->insert( actionDiskInfo );
  devicePopupMenu->insert( new KActionSeparator( this ) );
  devicePopupMenu->insert( actionUnmount );
  devicePopupMenu->insert( actionMount );
  devicePopupMenu->insert( new KActionSeparator( this ) );
  devicePopupMenu->insert( actionEject );
  devicePopupMenu->insert( actionLoad );
//  devicePopupMenu->insert( new KActionSeparator( this ) );
//  devicePopupMenu->insert( actionUnlock );
//  devicePopupMenu->insert( actionlock );
  devicePopupMenu->insert( new KActionSeparator( this ) );
  devicePopupMenu->insert( actionSetReadSpeed );
}


int K3bAppDeviceManager::scanBus()
{
  int c = K3bDevice::DeviceManager::scanBus();

  if( allDevices().getFirst() ) {
    setCurrentDevice( allDevices().getFirst() );
  }

  return c;
}


K3bDevice::Device* K3bAppDeviceManager::currentDevice() const
{
  return m_currentDevice;
}


void K3bAppDeviceManager::clear()
{
  // make sure we do not use a deleted device
  m_currentDevice = 0;
  K3bDevice::DeviceManager::clear();
}


void K3bAppDeviceManager::removeDevice( const QString& dev )
{
  if( m_currentDevice == findDevice(dev) )
    m_currentDevice = 0;

  K3bDevice::DeviceManager::removeDevice( dev );

  if( !m_currentDevice )
    setCurrentDevice( allDevices().getFirst() );
}


K3bAppDeviceManager::~K3bAppDeviceManager()
{
}


void K3bAppDeviceManager::setCurrentDevice( K3bDevice::Device* dev )
{
  if( dev && dev != m_currentDevice ) {
    m_currentDevice = dev;
#warning FIXME
//     actionCollection()->action( "device_mount" )->setEnabled( !dev->automount() );
//     actionCollection()->action( "device_unmount" )->setEnabled( !dev->automount() );
    emit currentDeviceChanged( dev );
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
#warning FIXME
//   if( currentDevice() ) {
//     if( !currentDevice()->mountPoint().isEmpty() ) {
//       QString mp = KIO::findDeviceMountPoint( currentDevice()->mountDevice() );
//       if( mp.isEmpty() )
// 	connect( KIO::mount( true, 0, currentDevice()->mountDevice(), currentDevice()->mountPoint(), false ), 
// 		 SIGNAL(result(KIO::Job*)),
// 		 this, 
// 		 SLOT(slotMountFinished(KIO::Job*)) );
//       else {
// 	emit mountFinished( mp );
//       }
//     }
//     else
//       emit mountFinished( QString::null );
//   }
}


void K3bAppDeviceManager::slotMountFinished( KIO::Job* job )
{
#warning FIXME
//   if( job->error() ) {
//     emit mountFinished( QString::null );
//   }
//   else {
//     emit mountFinished( currentDevice()->mountPoint() );
//   }
}


void K3bAppDeviceManager::unmountDisk()
{
#warning FIXME
//   if( currentDevice() ) {
//     QString mp = KIO::findDeviceMountPoint( currentDevice()->mountDevice() );
//     if( mp.isEmpty() ) {
//       if( m_ejectRequested ) {
// 	m_ejectRequested = false;
// 	K3bDevice::eject( currentDevice() );
//       }
//       else
// 	emit unmountFinished( true );   
//     }
//     else {
//       connect( KIO::unmount( currentDevice()->mountPoint(), false ), SIGNAL(result(KIO::Job*)),
// 	       this, SLOT(slotUnmountFinished(KIO::Job*)) );
//     }
//   }
//   else
//     m_ejectRequested = false;
}


void K3bAppDeviceManager::slotUnmountFinished( KIO::Job* job )
{
  if( m_ejectRequested ) {
    m_ejectRequested = false;
    K3bDevice::eject( currentDevice() ); // just ignore errors here
  }
  else
    emit unmountFinished( job->error() );
}


void K3bAppDeviceManager::ejectDisk()
{
  m_ejectRequested = true;
  unmountDisk();
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
					   "change the reading speed again when copying CDs or DVDs.")
				      .arg(currentDevice()->vendor() + " " + currentDevice()->description()),
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
