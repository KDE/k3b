/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
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
#include <k3bdevice.h>
#include <k3bcdtext.h>


// TODO : allow a bitwise or of the command types

class K3bCdDevice::DeviceHandler::DeviceHandlerThread : public K3bThread
{
public:
  DeviceHandlerThread() 
    : K3bThread(),
      dev(0) {
  }


  void run() {
    success = false;

    if( dev ) {
      success = (dev->open() != -1);
      if( command & DISKINFO ) {
	ngInfo = dev->diskInfo();
	if( !ngInfo.empty() ) {
	  toc = dev->readToc();
	  if( toc.contentType() == AUDIO ||
	      toc.contentType() == MIXED )
	    cdText = dev->readCdText();
	}
      }

      if( command & (NG_DISKINFO|
		     DISKSIZE|
		     REMAININGSIZE|
		     NUMSESSIONS) ) {
	ngInfo = dev->diskInfo();
      }

      if( command & (TOC|TOCTYPE) ) {
	toc = dev->readToc();
      }

      if( command & CD_TEXT ) {
	cdText = dev->readCdText();
	success = (success && !cdText.isEmpty());
      }

      if( command & CD_TEXT_RAW ) {
	unsigned char* data = 0;
	int dataLen = 0;
	if( dev->readTocPmaAtip( &data, dataLen, 5, false, 0 ) ) {
	  // we need more than the header and a multible of 18 bytes to have valid CD-TEXT
	  if( dataLen > 4 && dataLen%sizeof(cdtext_pack) == 4 ) {
	    cdTextRaw.assign( reinterpret_cast<char*>(data), dataLen );
	  }
	  else {
	    kdDebug() << "(K3bCdDevice::DeviceHandler) invalid CD-TEXT length: " << dataLen << endl;
	    delete [] data;
	    success = false;
	  }
	}
	else
	  success = false;
      }

      if( command & BLOCK )
	success = (success && dev->block( true ));

      if( command & UNBLOCK )
	success = (success && dev->block( false ));

      //
      // It is important that eject is performed before load
      // since the RELOAD command is a combination of both
      //

      if( command & EJECT )
	success = (success && dev->eject());

      if( command & LOAD )
	success = (success && dev->load());

      dev->close();
    }
    emitFinished(success);
  }

  bool success;
  int errorCode;
  int command;
  DiskInfo ngInfo;
  Toc toc;
  CdText cdText;
  QByteArray cdTextRaw;
  CdDevice* dev;
};


K3bCdDevice::DeviceHandler::DeviceHandler( CdDevice* dev, QObject* parent, const char* name )
  : K3bThreadJob( 0, parent, name ),
    m_selfDelete(false)
{
  m_thread = new DeviceHandlerThread();
  m_thread->dev = dev;
  setThread( m_thread );
}


K3bCdDevice::DeviceHandler::DeviceHandler( QObject* parent, const char* name )
  : K3bThreadJob( 0, parent, name ),
    m_selfDelete(false)
{
  m_thread = new DeviceHandlerThread();
  setThread( m_thread );
}


K3bCdDevice::DeviceHandler::DeviceHandler( int command, CdDevice* dev, const char* name )
  : K3bThreadJob( 0, 0, name ),
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
  return m_thread->ngInfo;
}


const K3bCdDevice::Toc& K3bCdDevice::DeviceHandler::toc() const
{
  return m_thread->toc;
}

const K3bCdDevice::CdText& K3bCdDevice::DeviceHandler::cdText() const
{
  return m_thread->cdText;
}


const QByteArray& K3bCdDevice::DeviceHandler::cdTextRaw() const
{
  return m_thread->cdTextRaw;
}


K3b::Msf K3bCdDevice::DeviceHandler::diskSize() const
{
  return m_thread->ngInfo.capacity();
}

K3b::Msf K3bCdDevice::DeviceHandler::remainingSize() const
{
  return m_thread->ngInfo.remainingSize();
}

int K3bCdDevice::DeviceHandler::tocType() const
{
  return m_thread->toc.contentType();
}

int K3bCdDevice::DeviceHandler::numSessions() const
{
  return m_thread->ngInfo.numSessions();
}

void K3bCdDevice::DeviceHandler::setDevice( CdDevice* dev )
{
  m_thread->dev = dev;
}



void K3bCdDevice::DeviceHandler::sendCommand( int command )
{
  kdDebug() << "(K3bCdDevice::DeviceHandler) starting command: " << command << endl;

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
      m_thread->wait();
      kdDebug() << "(K3bCdDevice::DeviceHandler) deleting thread." << endl;
      deleteLater();
    }
  }
}


#include "k3bdevicehandler.moc"
