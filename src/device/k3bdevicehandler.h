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


#ifndef _K3B_DEVICE_HANDLER_H_
#define _K3B_DEVICE_HANDLER_H_

#include <k3bthreadjob.h>
#include <device/k3bdiskinfo.h>
#include <device/k3bmsf.h>


class QCustomEvent;


namespace K3bCdDevice
{
  class CdDevice;


  /**
   * The K3bDevicehandler is a threaded wrapper around K3bDevice.
   * It allows async access to the time comsuming blocking K3bDevice methods.
   * Since it's a K3bJob it is very easy to handle. Just use one of the methods and
   * connect to the finished signal.
   * Be aware that all methods only return valid values if the corresponding info has
   * been successfully requested.
   */
  class DeviceHandler : public K3bThreadJob
    {
      Q_OBJECT

     public:
      DeviceHandler( CdDevice*, QObject* parent = 0, const char* name = 0 );
      DeviceHandler( QObject* parent = 0, const char* name = 0 );

      /**
       * This constructor is used by the global "quick" methods and should not be used
       * otherwise except for the same usage.
       */
      DeviceHandler( int command, CdDevice*, const char* name );

      ~DeviceHandler();

      const DiskInfo& diskInfo() const;
      const Toc& toc() const;
      const K3b::Msf& diskSize() const;
      const K3b::Msf& remainingSize() const;
      int tocType() const;
      int numSessions() const;

      bool success() const;

      /**
       * Use this when the command
       * returnes some error code.
       */
      int errorCode() const;

      enum Command {
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

    signals:
      void finished( K3bCdDevice::DeviceHandler* );

     public slots:
      void setDevice( CdDevice* );
      void sendCommand( int command );

      void getToc();
      void getDiskInfo();
      void getDiskSize();
      void getRemainingSize();
      void getTocType();
      void getNumSessions();
      void block( bool );
      void eject();
      void mount();
      void unmount();

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
   *  connect( K3bCdDevice::sendCommand( K3bCdDevice::DeviceHandler::MOUNT, dev ), SIGNAL(finished(DeviceHandler*)),
   *           this, SLOT(someSlot(DeviceHandler*)) );
   *
   *  void someSlot( DeviceHandler* dh ) {
   *     if( dh->success() ) {
   * </pre>
   * Be aware that the DeviceHandler will get destroyed once the signal has been 
   * emited.
   */
  DeviceHandler* sendCommand( int command, CdDevice* );
};

#endif
