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


#ifndef K3BDEVICEMANAGER_H
#define K3BDEVICEMANAGER_H

#include <qobject.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qmemarray.h>
#include <qptrlist.h>
#include "k3bdevice_export.h"
#include <kdebug.h>

class KProcess;
class KConfig;
class K3bExternalBin;


namespace K3bDevice {

  class Device;

  /**
   * \brief Manages all devices.
   *
   * Searches the system for devices and maintains lists of them.
   *
   * <b>Basic usage:</b>
   * \code
   *   K3bDevice::DeviceManager* manager = new K3bDevice::DeviceManager( this );
   *   manager->scanBus();
   *   manager->scanFstab();
   *   K3bDevice::Device* dev = manager->findDevice( "/dev/cdrom" );
   * \endcode
   */
  class LIBK3BDEVICE_EXPORT DeviceManager : public QObject
    {
      Q_OBJECT

    public:
      /**
       * Creates a new DeviceManager
       */
      DeviceManager( QObject* parent = 0, const char* name = 0 );
      virtual ~DeviceManager();

      /**
       * By default the DeviceManager makes the Devices check their writing modes.
       * This includes commands to be sent which require writing permissions on the
       * devices and might take some time.
       *
       * So if you don't need the information about the writing modes use this method
       * to speed up the device detection procedure.
       *
       * Be aware that this only refers to CD writing modes. If you only want to handle
       * DVD devices it's always save to set this to false.
       */
      void setCheckWritingModes( bool b );

      /**
       * \deprecated use findDevice( const QString& )
       */
      Device* deviceByName( const QString& );

      /**
       * Search an SCSI device by SCSI bus, id, and lun.
       *
       * \note This method does not initialize new devices.
       *       Devices cannot be found until they have been added via addDevice(const QString&)
       *       or scanBus().
       *
       * \return The corresponding device or 0 if there is no such device.
       */
      Device* findDevice( int bus, int id, int lun );

      /**
       * Search a device by blockdevice name.
       *
       * \note This method does not initialize new devices.
       *       Devices cannot be found until they have been added via addDevice(const QString&)
       *       or scanBus().
       *
       * \return The corresponding device or 0 if there is no such device.
       */
      Device* findDevice( const QString& devicename );

      /**
       * Before getting the devices do a @ref scanBus().
       * \return List of all cd writer devices.
       * \deprecated use cdWriter()
       */
      QPtrList<Device>& burningDevices() const;

      /**
       * \return List of all reader devices without writer devices.
       * \deprecated use cdReader()
       **/
      QPtrList<Device>& readingDevices() const;

      /**
       * Before getting the devices do a @ref scanBus() or add 
       * devices via addDevice( const QString& ).
       *
       * \return List of all devices.
       */
      QPtrList<Device>& allDevices() const;

      /**
       * Before getting the devices do a @ref scanBus() or add 
       * devices via addDevice( const QString& ).
       *
       * \return List of all cd writer devices.
       */
      QPtrList<Device>& cdWriter() const;

      /**
       * Before getting the devices do a @ref scanBus() or add 
       * devices via addDevice( const QString& ).
       *
       * \return List of all cd reader devices.
       */
      QPtrList<Device>& cdReader() const;

      /**
       * Before getting the devices do a @ref scanBus() or add 
       * devices via addDevice( const QString& ).
       *
       * \return List of all DVD writer devices.
       */
      QPtrList<Device>& dvdWriter() const;

      /**
       * Before getting the devices do a @ref scanBus() or add 
       * devices via addDevice( const QString& ).
       *
       * \return List of all DVD reader devices.
       */
      QPtrList<Device>& dvdReader() const;

      /**
       * Reads the device information from the config file.
       */
      virtual bool readConfig( KConfig* );

      virtual bool saveConfig( KConfig* );


    public slots:
      /**
       * Writes a list of all devices to stderr.
       */
      void printDevices();

      /**
       * Scan the system for devices. Call this to initialize all devices.
       *
       * \return Number of found devices.
       **/
      virtual int scanBus();

      /**
       * Searches for mountpoints of the devices. This method will also add devices
       * that have an entry in the fstab file and have not yet been found.
       */
      void scanFstab();

      /**
       * Clears the writers and readers list of devices.
       */
      virtual void clear();

      /**
       * Add a new device.
       *
       * \param dev Name of a block device or link to a block device. If the 
       *            corresponding device has already been detected it will simply
       *            be returned. Otherwise if a device is found it will be initialized
       *            and added to the internal lists (meaning it can be accessed through
       *            emthods like cdReader()).
       *
       * Called by scanBus()
       *
       * \return The device if it could be found or 0 otherwise.
       */
      virtual Device* addDevice( const QString& dev );

      virtual void removeDevice( const QString& dev );

    signals:
      /**
       * Emitted if the device configuration changed, i.e. a device was added or removed.
       */
      void changed( K3bDevice::DeviceManager* );
      void changed();

    private:
      bool testForCdrom( const QString& );
      bool determineBusIdLun( const QString &dev, int& bus, int& id, int& lun );
      QString resolveSymLink( const QString& path );

      int m_foundDevices;

      class Private;
      Private* d;

      /**
       * Add a device to the managers device lists and initialize the device.
       */
      Device *addDevice( Device* );

      void BSDDeviceScan();
      void LinuxDeviceScan();
    };
}

#endif
