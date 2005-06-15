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



class K3bDevice::DeviceHandler::DeviceHandlerThread : public K3bThread
{
public:
  DeviceHandlerThread() 
    : K3bThread(),
      dev(0) {
  }


  void run() {
    success = false;
    m_bCanceled = false;

    if( dev ) {
      success = dev->open();
      if( !m_bCanceled && command & DISKINFO ) {
	ngInfo = dev->diskInfo();
	if( !m_bCanceled && !ngInfo.empty() ) {
	  toc = dev->readToc();
	  if( toc.contentType() == AUDIO ||
	      toc.contentType() == MIXED )
	    cdText = dev->readCdText();
	}
      }

      if( !m_bCanceled && command & (NG_DISKINFO|
				     DISKSIZE|
				     REMAININGSIZE|
				     NUMSESSIONS) ) {
	ngInfo = dev->diskInfo();
      }

      if( !m_bCanceled && command & (TOC|TOCTYPE) ) {
	toc = dev->readToc();
      }

      if( !m_bCanceled && command & CD_TEXT ) {
	cdText = dev->readCdText();
	success = (success && !cdText.isEmpty());
      }

      if( !m_bCanceled && command & CD_TEXT_RAW ) {
	unsigned char* data = 0;
	int dataLen = 0;
	if( dev->readTocPmaAtip( &data, dataLen, 5, false, 0 ) ) {
	  // we need more than the header and a multible of 18 bytes to have valid CD-TEXT
	  if( dataLen > 4 && dataLen%18 == 4 ) {
	    cdTextRaw.assign( reinterpret_cast<char*>(data), dataLen );
	  }
	  else {
	    kdDebug() << "(K3bDevice::DeviceHandler) invalid CD-TEXT length: " << dataLen << endl;
	    delete [] data;
	    success = false;
	  }
	}
	else
	  success = false;
      }

      if( !m_bCanceled && command & BLOCK )
	success = (success && dev->block( true ));

      if( !m_bCanceled && command & UNBLOCK )
	success = (success && dev->block( false ));

      //
      // It is important that eject is performed before load
      // since the RELOAD command is a combination of both
      //

      if( !m_bCanceled && command & EJECT )
	success = (success && dev->eject());

      if( !m_bCanceled && command & LOAD )
	success = (success && dev->load());

      if( !m_bCanceled && command & BUFFER_CAPACITY )
	success = dev->readBufferCapacity( bufferCapacity, availableBufferCapacity );
	
      dev->close();
    }

    //
    // This thread only gets cancelled if a new request was started.
    // So we don't emit the finished signal for this (old) request.
    //
    if( !m_bCanceled )
      emitFinished(success);
  }

  void cancel() {
    m_bCanceled = true;
  }


  bool success;
  int errorCode;
  int command;
  DiskInfo ngInfo;
  Toc toc;
  CdText cdText;
  QByteArray cdTextRaw;
  long long bufferCapacity;
  long long availableBufferCapacity;
  Device* dev;

private:
  bool m_bCanceled;
};


K3bDevice::DeviceHandler::DeviceHandler( Device* dev, QObject* parent, const char* name )
  : K3bThreadJob( 0, parent, name ),
    m_selfDelete(false)
{
  m_thread = new DeviceHandlerThread();
  m_thread->dev = dev;
  setThread( m_thread );
}


K3bDevice::DeviceHandler::DeviceHandler( QObject* parent, const char* name )
  : K3bThreadJob( 0, parent, name ),
    m_selfDelete(false)
{
  m_thread = new DeviceHandlerThread();
  setThread( m_thread );
}


K3bDevice::DeviceHandler::DeviceHandler( int command, Device* dev, const char* name )
  : K3bThreadJob( 0, 0, name ),
    m_selfDelete(true)
{
  m_thread = new DeviceHandlerThread();
  setThread( m_thread );
  m_thread->dev = dev;
  sendCommand(command);
}

K3bDevice::DeviceHandler::~DeviceHandler()
{
  delete m_thread;
}


int K3bDevice::DeviceHandler::errorCode() const
{
  return m_thread->errorCode;
}

bool K3bDevice::DeviceHandler::success() const
{
  return m_thread->success;
}


const K3bDevice::DiskInfo& K3bDevice::DeviceHandler::diskInfo() const
{
  return m_thread->ngInfo;
}


const K3bDevice::Toc& K3bDevice::DeviceHandler::toc() const
{
  return m_thread->toc;
}

const K3bDevice::CdText& K3bDevice::DeviceHandler::cdText() const
{
  return m_thread->cdText;
}


const QByteArray& K3bDevice::DeviceHandler::cdTextRaw() const
{
  return m_thread->cdTextRaw;
}


K3b::Msf K3bDevice::DeviceHandler::diskSize() const
{
  return m_thread->ngInfo.capacity();
}

K3b::Msf K3bDevice::DeviceHandler::remainingSize() const
{
  return m_thread->ngInfo.remainingSize();
}

int K3bDevice::DeviceHandler::tocType() const
{
  return m_thread->toc.contentType();
}

int K3bDevice::DeviceHandler::numSessions() const
{
  return m_thread->ngInfo.numSessions();
}

long long K3bDevice::DeviceHandler::bufferCapacity() const
{
  return m_thread->bufferCapacity;
}

long long K3bDevice::DeviceHandler::availableBufferCapacity() const
{
  return m_thread->availableBufferCapacity;
}

void K3bDevice::DeviceHandler::setDevice( Device* dev )
{
  m_thread->dev = dev;
}



void K3bDevice::DeviceHandler::sendCommand( int command )
{
  //
  // We do not want the finished signal emitted in case the devicehandler was cancelled. This is a special case.
  // That's why we do not use K3bThreadJob::start() becasue otherwise we would be registered twice.
  //
  if( m_thread->running() ) {
    kdDebug() << "(K3bDevice::DeviceHandler) thread already running. canceling thread..." << endl;
    m_thread->cancel();
    m_thread->wait();
  }
  else
    jobStarted();

  kdDebug() << "(K3bDevice::DeviceHandler) starting command: " << command << endl;

  m_thread->command = command;
  m_thread->start();
}

void K3bDevice::DeviceHandler::getToc()
{
  sendCommand(DeviceHandler::TOC);
}

void K3bDevice::DeviceHandler::getDiskInfo()
{
  sendCommand(DeviceHandler::DISKINFO);
}

void K3bDevice::DeviceHandler::getDiskSize()
{
  sendCommand(DeviceHandler::DISKSIZE);
}

void K3bDevice::DeviceHandler::getRemainingSize()
{
  sendCommand(DeviceHandler::REMAININGSIZE);
}

void K3bDevice::DeviceHandler::getTocType()
{
  sendCommand(DeviceHandler::TOCTYPE);
}

void K3bDevice::DeviceHandler::getNumSessions()
{
  sendCommand(DeviceHandler::NUMSESSIONS);
}


void K3bDevice::DeviceHandler::block( bool b )
{
  sendCommand(b ? DeviceHandler::BLOCK : DeviceHandler::UNBLOCK);
}

void K3bDevice::DeviceHandler::eject()
{
  sendCommand(DeviceHandler::EJECT);
}

K3bDevice::DeviceHandler* K3bDevice::sendCommand( int command, Device* dev )
{
  return new DeviceHandler( command, dev, "DeviceHandler" );
}

void K3bDevice::DeviceHandler::customEvent( QCustomEvent* e )
{
  K3bThreadJob::customEvent(e);

  if( (int)e->type() == K3bProgressInfoEvent::Finished ) {
    emit finished( this );
    if( m_selfDelete ) {
      kdDebug() << "(K3bDevice::DeviceHandler) thread emitted finished. Waiting for thread actually finishing" << endl;
      kdDebug() << "(K3bDevice::DeviceHandler) success: " << m_thread->success << endl;
      // wait for the thread to finish
      m_thread->wait();
      kdDebug() << "(K3bDevice::DeviceHandler) deleting thread." << endl;
      deleteLater();
    }
  }
}


#include "k3bdevicehandler.moc"
