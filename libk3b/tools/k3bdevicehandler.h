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


#ifndef _K3B_DEVICE_HANDLER_H_
#define _K3B_DEVICE_HANDLER_H_

#include <k3bthreadjob.h>
#include "k3bdevice.h"
#include "k3bdiskinfo.h"
#include "k3bmsf.h"
#include "k3bcdtext.h"

#include <qcstring.h>

class QCustomEvent;


namespace K3bDevice
{
  class Device;


  /**
   * The K3bDevice::Devicehandler is a threaded wrapper around K3bDevice::Device.
   * It allows async access to the time comsuming blocking K3bDevice::Device methods.
   * Since it's a K3bJob it is very easy to handle. Just use one of the methods and
   * connect to the finished signal.
   * Be aware that all methods only return valid values if the corresponding info has
   * been successfully requested.
   */
  class DeviceHandler : public K3bThreadJob
    {
      Q_OBJECT

     public:
      DeviceHandler( Device*, QObject* parent = 0, const char* name = 0 );
      DeviceHandler( QObject* parent = 0, const char* name = 0 );

      /**
       * This constructor is used by the global "quick" methods and should not be used
       * otherwise except for the same usage.
       */
      DeviceHandler( int command, Device*, const char* name = 0 );

      ~DeviceHandler();

      const DiskInfo& diskInfo() const;
      const Toc& toc() const;
      const CdText& cdText() const;
      const QByteArray& cdTextRaw() const;
      K3b::Msf diskSize() const;
      K3b::Msf remainingSize() const;
      int tocType() const;
      int numSessions() const;

      bool success() const;

      /**
       * Use this when the command
       * returnes some error code.
       */
      int errorCode() const;

      enum Command {
	/**
	 * Always successfull, even with an empty or no media at all!
	 */
	NG_DISKINFO = 1, // TODO: rename this into DISKINFO
	/**
	 * Always successfull, even with an empty or no media at all!
	 */
	TOC = 2,
	/**
	 * Successful if the media contains CD-Text.
	 */
	CD_TEXT = 4,
	/**
	 * Successful if the media contains CD-Text.
	 */
	CD_TEXT_RAW = 8,
	/**
	 * Always successfull, even with an empty or no media at all!
	 */
	DISKSIZE = 16,
	/**
	 * Always successfull, even with an empty or no media at all!
	 */
	REMAININGSIZE = 32,
	/**
	 * Always successfull, even with an empty or no media at all!
	 */
	TOCTYPE = 64,
	/**
	 * Always successfull, even with an empty or no media at all!
	 */
	NUMSESSIONS = 128,
	/**
	 * Successful if the drive could be blocked.
	 */
	BLOCK = 256,
	/**
	 * Successful if the drive could be unblocked.
	 */
	UNBLOCK = 512,
	/**
	 * Successful if the media was ejected.
	 */
	EJECT = 1024,
	/**
	 * Successful if the media was loaded
	 */
	LOAD = 2048,
	RELOAD = EJECT|LOAD,
	/**
	 * Retrieves NG_DISKINFO, TOC, and CD-Text in case of an audio or mixed
	 * mode cd.
	 * The only difference to NG_DISKINFO|TOC|CD_TEXT is that no CD-Text is not
	 * considered an error.
	 *
	 * Always successfull, even with an empty or no media at all!
	 */
	DISKINFO = 4096  // TODO: rename this in somthing like: DISKINFO_COMPLETE
      };

    signals:
      void finished( K3bDevice::DeviceHandler* );

     public slots:
      void setDevice( Device* );
      void sendCommand( int command );

      void getToc();
      void getDiskInfo();
      void getDiskSize();
      void getRemainingSize();
      void getTocType();
      void getNumSessions();
      void block( bool );
      void eject();

    protected:
      /**
       * reimplemented from K3bThreadJob for internal reasons
       */
      virtual void customEvent( QCustomEvent* );

     private:
      class DeviceHandlerThread;
      DeviceHandlerThread* m_thread;

      bool m_selfDelete;
    };

  /**
   * Usage: 
   * <pre> 
   *  connect( K3bDevice::sendCommand( K3bDevice::DeviceHandler::MOUNT, dev ), SIGNAL(finished(DeviceHandler*)),
   *           this, SLOT(someSlot(DeviceHandler*)) );
   *
   *  void someSlot( DeviceHandler* dh ) {
   *     if( dh->success() ) {
   * </pre>
   * Be aware that the DeviceHandler will get destroyed once the signal has been 
   * emited.
   */
  DeviceHandler* sendCommand( int command, Device* );

  inline DeviceHandler* diskInfo(Device* dev) {
    return sendCommand(DeviceHandler::DISKINFO,dev);
  }

  inline DeviceHandler* toc(Device* dev) {
    return sendCommand(DeviceHandler::TOC,dev);
  }

  inline DeviceHandler* diskSize(Device* dev) {
    return sendCommand(DeviceHandler::DISKSIZE,dev);
  }

  inline DeviceHandler* remainingSize(Device* dev) {
    return sendCommand(DeviceHandler::REMAININGSIZE,dev);
  }

  inline DeviceHandler* tocType(Device* dev) {
    return sendCommand(DeviceHandler::TOCTYPE,dev);
  }

  inline DeviceHandler* numSessions(Device* dev) {
    return sendCommand(DeviceHandler::NUMSESSIONS,dev);
  }

  inline DeviceHandler* block(Device* dev) {
    return sendCommand(DeviceHandler::BLOCK,dev);
  }

  inline DeviceHandler* unblock(Device* dev) {
    return sendCommand(DeviceHandler::UNBLOCK,dev);
  }

  inline DeviceHandler* eject(Device* dev) {
    return sendCommand(DeviceHandler::EJECT,dev);
  }

  inline DeviceHandler* reload(Device* dev) {
    return sendCommand(DeviceHandler::RELOAD,dev);
  }

  inline DeviceHandler* load(Device* dev) {
    return sendCommand(DeviceHandler::LOAD,dev);
  }
}

#endif
