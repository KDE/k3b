/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3bdevicehandler.h"

#include <k3bthread.h>
#include <device/k3bdevice.h>

class K3bCdDevice::DeviceHandler::DeviceHandlerThread : public K3bThread
{
public:
  DeviceHandlerThread() 
    : K3bThread(),
      dev(0) {
  }


  void run() {
    bool success = true;

    if( dev ) {
      switch( request ) {
      case DISKINFO:
	info = dev->diskInfo();
	success = info.valid;
	break;
      case TOC:
	info.toc = dev->readToc();
	break;
      case DISKSIZE:
	info.size = dev->discSize();
	success = (info.size != 0);
	break;
      case REMAININGSIZE:
	info.remaining = dev->remainingSize();
	success = (info.remaining != 0);
	break;
      case TOCTYPE:
	info.tocType = dev->diskType();
	success = ( info.tocType != DiskInfo::UNKNOWN );
	break;
      case NUMSESSIONS:
	info.sessions = dev->numSessions();
	break;
      case BLOCK:
	success = dev->block( true );
	break;
      case UNBLOCK:
	success = dev->block( false );
	break;
      case EJECT:
        dev->eject();
	success = true;
	break;
      case MOUNT:
        success = ( dev->mount() >= 0 );
	break;
      case UNMOUNT:
        success = ( dev->unmount() >= 0 );
        break;
      default:
	success = false;
      }
    }
    emitFinished(success);
  }

  enum Request {
    DISKINFO,
    TOC,
    DISKSIZE,
    REMAININGSIZE,
    TOCTYPE,
    NUMSESSIONS,
    BLOCK,
    UNBLOCK,
    EJECT,
    MOUNT,
    UNMOUNT
  };

  int request;
  DiskInfo info;
  CdDevice* dev;
};


K3bCdDevice::DeviceHandler::DeviceHandler( CdDevice* dev, QObject* parent, const char* name )
  : K3bThreadJob( parent, name )
{
  m_thread = new DeviceHandlerThread();
  m_thread->dev = dev;
  setThread( m_thread );
}


K3bCdDevice::DeviceHandler::DeviceHandler( QObject* parent, const char* name )
  : K3bThreadJob( parent, name )
{
  m_thread = new DeviceHandlerThread();
  setThread( m_thread );
}

K3bCdDevice::DeviceHandler::~DeviceHandler()
{
  delete m_thread;
}

const K3bCdDevice::DiskInfo& K3bCdDevice::DeviceHandler::diskInfo() const
{
  return m_thread->info;
}

const K3bCdDevice::Toc& K3bCdDevice::DeviceHandler::toc() const
{
  return m_thread->info.toc;
}

const K3b::Msf& K3bCdDevice::DeviceHandler::diskSize() const
{
  return m_thread->info.size;
}

const K3b::Msf& K3bCdDevice::DeviceHandler::remainingSize() const
{
  return m_thread->info.remaining;
}

int K3bCdDevice::DeviceHandler::tocType() const
{
  return m_thread->info.tocType;
}

int K3bCdDevice::DeviceHandler::numSessions() const
{
  return m_thread->info.sessions;
}

void K3bCdDevice::DeviceHandler::setDevice( CdDevice* dev )
{
  m_thread->dev = dev;
}

void K3bCdDevice::DeviceHandler::getToc()
{
  m_thread->request = DeviceHandlerThread::TOC;
  start();
}

void K3bCdDevice::DeviceHandler::getDiskInfo()
{
  m_thread->request = DeviceHandlerThread::DISKINFO;
  start();
}

void K3bCdDevice::DeviceHandler::getDiskSize()
{
  m_thread->request = DeviceHandlerThread::DISKSIZE;
  start();
}

void K3bCdDevice::DeviceHandler::getRemainingSize()
{
  m_thread->request = DeviceHandlerThread::REMAININGSIZE;
  start();
}

void K3bCdDevice::DeviceHandler::getTocType()
{
  m_thread->request = DeviceHandlerThread::TOCTYPE;
  start();
}

void K3bCdDevice::DeviceHandler::getNumSessions()
{
  m_thread->request = DeviceHandlerThread::NUMSESSIONS;
  start();
}

void K3bCdDevice::DeviceHandler::block( bool b )
{
  m_thread->request = ( b ? DeviceHandlerThread::BLOCK : DeviceHandlerThread::UNBLOCK );
  start();
}

void K3bCdDevice::DeviceHandler::eject()
{
  m_thread->request = DeviceHandlerThread::EJECT;
  start();
}

void K3bCdDevice::DeviceHandler::mount()
{
  m_thread->request = DeviceHandlerThread::MOUNT;
  start();
}

void K3bCdDevice::DeviceHandler::unmount()
{
  m_thread->request = DeviceHandlerThread::UNMOUNT;
  start();
}

#include "k3bdevicehandler.moc"
