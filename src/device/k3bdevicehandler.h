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

namespace K3bCdDevice
{
  class CdDevice;


  /**
   * The K3bDevicehandler is a threaded wrapper around K3bDevice.
   * It allows async access to the time comsuming blocking K3bDevice methods.
   * Since it's a K3bJob it is very easy to handle. Just use one of the methods and
   * connect to the finished signal.
   * Be aware that all methods onyl return valid values if the corresponding info has
   * been successfully requested.
   */
  class DeviceHandler : public K3bThreadJob
    {
      Q_OBJECT

     public:
      DeviceHandler( CdDevice*, QObject* parent = 0, const char* name = 0 );
      DeviceHandler( QObject* parent = 0, const char* name = 0 );
      ~DeviceHandler();

      const DiskInfo& diskInfo() const;
      const Toc& toc() const;
      const K3b::Msf& diskSize() const;
      const K3b::Msf& remainingSize() const;
      int tocType() const;
      int numSessions() const;

     public slots:
      void setDevice( CdDevice* );
      void getToc();
      void getDiskInfo();
      void getDiskSize();
      void getRemainingSize();
      void getTocType();
      void getNumSessions();
      void block( bool );

     private:
      class DeviceHandlerThread;
      DeviceHandlerThread* m_thread;
    };

};

#endif
