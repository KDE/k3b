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
#include <k3bprogressinfoevent.h>
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
    success = true;

    if( dev ) {
      switch( command ) {
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
	success = dev->eject();
	break;
      case MOUNT:
        errorCode = dev->mount();
	success = (errorCode >= 0 );
	break;
      case UNMOUNT:
        errorCode = dev->unmount();
	success = (errorCode >= 0 );
        break;
      case MEDIUM_STATE:
	errorCode = dev->isEmpty();
	success = ( errorCode != K3bCdDevice::CdDevice::NO_INFO );
	break;
      default:
	success = false;
      }
    }
    emitFinished(success);
  }

  bool success;
  int errorCode;
  int command;
  DiskInfo info;
  CdDevice* dev;
};


K3bCdDevice::DeviceHandler::DeviceHandler( CdDevice* dev, QObject* parent, const char* name )
  : K3bThreadJob( parent, name ),
    m_selfDelete(false)
{
  m_thread = new DeviceHandlerThread();
  m_thread->dev = dev;
  setThread( m_thread );
}


K3bCdDevice::DeviceHandler::DeviceHandler( QObject* parent, const char* name )
  : K3bThreadJob( parent, name ),
    m_selfDelete(false)
{
  m_thread = new DeviceHandlerThread();
  setThread( m_thread );
}


K3bCdDevice::DeviceHandler::DeviceHandler( int command, CdDevice* dev, const char* name )
  : K3bThreadJob( 0, name ),
    m_selfDelete(true)
{
  m_thread = new DeviceHandlerThread();
  setThread( m_thread );
  m_thread->dev = dev;
  sendCommand(command);
}

K3bCdDevice::DeviceHandler::~DeviceHandler()
{
  delete m_thread;
}


int K3bCdDevice::DeviceHandler::errorCode() const
{
  return m_thread->errorCode;
}

bool K3bCdDevice::DeviceHandler::success() const
{
  return m_thread->success;
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



void K3bCdDevice::DeviceHandler::sendCommand( int command )
{
  m_thread->command = command;
  start();
}

void K3bCdDevice::DeviceHandler::getToc()
{
  sendCommand(DeviceHandler::TOC);
}

void K3bCdDevice::DeviceHandler::getDiskInfo()
{
  sendCommand(DeviceHandler::DISKINFO);
}

void K3bCdDevice::DeviceHandler::getDiskSize()
{
  sendCommand(DeviceHandler::DISKSIZE);
}

void K3bCdDevice::DeviceHandler::getRemainingSize()
{
  sendCommand(DeviceHandler::REMAININGSIZE);
}

void K3bCdDevice::DeviceHandler::getTocType()
{
  sendCommand(DeviceHandler::TOCTYPE);
}

void K3bCdDevice::DeviceHandler::getNumSessions()
{
  sendCommand(DeviceHandler::NUMSESSIONS);
}


void K3bCdDevice::DeviceHandler::block( bool b )
{
  sendCommand(b ? DeviceHandler::BLOCK : DeviceHandler::UNBLOCK);
}

void K3bCdDevice::DeviceHandler::eject()
{
  sendCommand(DeviceHandler::EJECT);
}

void K3bCdDevice::DeviceHandler::mount()
{
  sendCommand(DeviceHandler::MOUNT);
}

void K3bCdDevice::DeviceHandler::unmount()
{
  sendCommand(DeviceHandler::UNMOUNT);
}


K3bCdDevice::DeviceHandler* K3bCdDevice::sendCommand( int command, CdDevice* dev )
{
  return new DeviceHandler( command, dev, "DeviceHandler" );
}

void K3bCdDevice::DeviceHandler::customEvent( QCustomEvent* e )
{
  K3bThreadJob::customEvent(e);

  if( (int)e->type() == K3bProgressInfoEvent::Finished ) {
    emit finished( this );
    if( m_selfDelete ) {
      kdDebug() << "(K3bCdDevice::DeviceHandler) thread emitted finished. Waiting for thread actually finishing" << endl;
      kdDebug() << "(K3bCdDevice::DeviceHandler) success: " << m_thread->success << endl;
      // wait for the thread to finish
      while( m_thread->running() );
      kdDebug() << "(K3bCdDevice::DeviceHandler) deleting thread." << endl;
      deleteLater();
    }
  }
}


#include "k3bdevicehandler.moc"
