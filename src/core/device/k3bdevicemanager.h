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

class KProcess;
class KConfig;
class K3bExternalBinManager;
class K3bExternalBin;


namespace K3bCdDevice {

  class CdDevice;

  class DeviceManager : public QObject
    {
      Q_OBJECT

    public:
      DeviceManager( K3bExternalBinManager*, QObject* parent = 0, const char* name = 0 );
      ~DeviceManager();

      CdDevice* deviceByName( const QString& );

      CdDevice* findDevice( int bus, int id, int lun );
      CdDevice* findDevice( const QString& devicename );

      /**
       * Before getting the devices do a @ref scanbus().
       * @return List of all cd writer devices.
       * @deprecated use cdWriter
       */
      QPtrList<CdDevice>& burningDevices();

      /**
       * Note that all burning devices can also be used as
       * reading device and are not present in this list.
       * Before getting the devices do a @ref scanbus().
       * @return List of all reader devices without writer devices.
       * @deprecated use cdReader
       **/
      QPtrList<CdDevice>& readingDevices();

      QPtrList<CdDevice>& allDevices();

      QPtrList<CdDevice>& cdWriter();
      QPtrList<CdDevice>& cdReader();
      QPtrList<CdDevice>& dvdWriter();
      QPtrList<CdDevice>& dvdReader();

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
      CdDevice* addDevice( const QString& );

    private slots:
      void slotCollectStdout( KProcess*, char* data, int len );

    private:
      bool testForCdrom( const QString& );
      bool determineBusIdLun( const QString &dev, int& bus, int& id, int& lun );
      void determineCapabilities(CdDevice *dev);
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
  QString externalBinDeviceParameter( CdDevice* dev, const K3bExternalBin* );
}

typedef K3bCdDevice::DeviceManager K3bDeviceManager;
#endif
