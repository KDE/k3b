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


#ifndef K3BDEVICEMANAGER_H
#define K3BDEVICEMANAGER_H

#include <qobject.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qmemarray.h>
#include <qptrlist.h>

#include <kdebug.h>
#include "k3bdevice.h"

class KProcess;
class KConfig;
class K3bExternalBinManager;
class K3bExternalBin;


namespace K3bCdDevice {

  class DeviceManager : public QObject
    {
      Q_OBJECT

    public:
      DeviceManager( K3bExternalBinManager*, QObject* parent = 0, const char* name = 0 );
      ~DeviceManager();

      K3bDevice* deviceByName( const QString& );

      K3bDevice* findDevice( int bus, int id, int lun );
      K3bDevice* findDevice( const QString& devicename );

      /**
       * Before getting the devices do a @ref scanbus().
       * @return List of all cd writer devices.
       * @deprecated use cdWriter
       */
      QPtrList<K3bDevice>& burningDevices();

      /**
       * Note that all burning devices can also be used as
       * reading device and are not present in this list.
       * Before getting the devices do a @ref scanbus().
       * @return List of all reader devices without writer devices.
       * @deprecated use cdReader
       **/
      QPtrList<K3bDevice>& readingDevices();

      QPtrList<K3bDevice>& allDevices();

      QPtrList<K3bDevice>& cdWriter();
      QPtrList<K3bDevice>& cdReader();
      QPtrList<K3bDevice>& dvdWriter();
      QPtrList<K3bDevice>& dvdReader();

      /** writes to stderr **/
      void printDevices();

      /**
       * Returns number of found devices
       **/
      int scanbus();

      void scanFstab();

      /**
       * Reads the device information from the config file.
       */
      bool readConfig( KConfig* );

      bool saveConfig( KConfig* );

      /**
       * Clears the writers and readers list of devices.
       */
      void clear();

      /**
       * add a new device like "/dev/mebecdrom" to be sensed
       * by the deviceManager.
       */
      K3bDevice* addDevice( const QString& );

    private slots:
      void slotCollectStdout( KProcess*, char* data, int len );

    private:
      bool testForCdrom( const QString& );
      bool determineBusIdLun( const QString &dev, int& bus, int& id, int& lun );
      void determineCapabilities(K3bDevice *dev);
      QString resolveSymLink( const QString& path );

      K3bExternalBinManager* m_externalBinManager;

      int m_foundDevices;

      QString m_processOutput;

      class Private;
      Private* d;
    };

  /**
   * true if the kernel supports ATAPI devices without SCSI emulation.
   * use in combination with the K3bExternalProgram feature "plain-atapi"
   */
  bool plainAtapiSupport();
  
  /**
   * true if the kernel supports ATAPI devices without SCSI emulation
   * via the ATAPI: pseudo stuff
   * use in combination with the K3bExternalProgram feature "hacked-atapi"
   */
  bool hackedAtapiSupport();

  /**
   * Used to create a parameter for cdrecord, cdrdao or readcd.
   * Takes care of SCSI and ATAPI.
   */
  QString externalBinDeviceParameter( K3bDevice* dev, const K3bExternalBin* );
}

typedef K3bCdDevice::DeviceManager K3bDeviceManager;
#endif
