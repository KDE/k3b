/* 
 *
 * $Id: sourceheader,v 1.3 2005/01/19 13:03:46 trueg Exp $
 * Copyright (C) 2005-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_HAL_CONNECTION_H_
#define _K3B_HAL_CONNECTION_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "k3bdevice_export.h"

#include <qobject.h>
#include <qmap.h>
#include <qstringlist.h>

class DBusConnection;


namespace K3bDevice {

  class Device;

  /**
   * This is a simple HAL/DBUS wrapper which creates QT signals whenever a new optical
   * drive is plugged into the system or one is unplugged.
   *
   * The HalConnection class also handles media changes. Whenever a new medium is inserted
   * into a drive or a medium is removed (i.e. ejected) a signal is emitted. This way it
   * is easy to keep track of the inserted media.
   *
   * This class does not deal with K3b devices but with system device names
   * such as /dev/cdrom. These device names can be used in DeviceManager::findDevice().
   */
  class LIBK3BDEVICE_EXPORT HalConnection : public QObject
    {
      Q_OBJECT

    public:
      ~HalConnection();

      /**
       * Creates a new singleton HalConnection object or returns the already existing one.
       * A newly created HalConnection will emit newDevice signals for all devices in the HAL
       * manager. However, since one cannot be sure if this is the first time the HalConnection
       * is created it is recommended to connect to the signals and query the list of current
       * devices.
       *
       * \return An instance of the singleton HalConnection object.
       */
      static HalConnection* instance();

      /**
       * \return true if a connection to the HAL deamon could be established and
       *         communication has been set up.
       */
      bool isConnected() const;

      /**
       * \return a list of optical devices as reported by HAL.
       */
      QStringList devices() const;

      /**
       * \internal
       */
      void addDevice( const char* udi );

      /**
       * \internal
       */
      void removeDevice( const char* udi );

      /**
       * Error codes named as the HAL deamon raises them
       */
      enum ErrorCodes {
	org_freedesktop_Hal_Success = 0, //*< The operation was successful. This code does not match any in HAL
	org_freedesktop_Hal_CommunicationError, //*< DBus communication error. This code does not match any in HAL
	org_freedesktop_Hal_NoSuchDevice,
	org_freedesktop_Hal_DeviceAlreadyLocked,
	org_freedesktop_Hal_PermissionDenied,
	org_freedesktop_Hal_Device_Volume_NoSuchDevice,
	org_freedesktop_Hal_Device_Volume_PermissionDenied,
	org_freedesktop_Hal_Device_Volume_AlreadyMounted,
	org_freedesktop_Hal_Device_Volume_InvalidMountOption,
	org_freedesktop_Hal_Device_Volume_UnknownFilesystemType,
	org_freedesktop_Hal_Device_Volume_InvalidMountpoint,
	org_freedesktop_Hal_Device_Volume_MountPointNotAvailable,
	org_freedesktop_Hal_Device_Volume_PermissionDeniedByPolicy,
	org_freedesktop_Hal_Device_Volume_InvalidUnmountOption,
	org_freedesktop_Hal_Device_Volume_InvalidEjectOption
      };

     public slots:
      /**
       * Lock the device in HAL
       * 
       * Be aware that once the method returns the HAL deamon has not necessarily 
       * finished the procedure yet.
       *
       * \param dev The device to lock
       * \return An error code
       *
       * \see ErrorCode
       */
      int lock( Device* );

      /**
       * Unlock a previously locked device in HAL
       * 
       * Be aware that once the method returns the HAL deamon has not necessarily 
       * finished the procedure yet.
       *
       * \param dev The device to lock
       * \return An error code
       *
       * \see ErrorCode
       */
      int unlock( Device* );

      /**
       * Mounts a device via HAL
       * 
       * Be aware that once the method returns the HAL deamon has not necessarily 
       * finished the procedure yet.
       *
       * \param dev The device to lock
       * \return An error code
       *
       * \see ErrorCode
       */
      int mount( Device*, 
		 const QString& mountPoint = QString::null, 
		 const QString& fstype = QString::null,
		 const QStringList& options = QStringList() );

      /**
       * Unmounts a device via HAL
       * 
       * Be aware that once the method returns the HAL deamon has not necessarily 
       * finished the procedure yet.
       *
       * \param dev The device to lock
       * \return An error code
       *
       * \see ErrorCode
       */
      int unmount( Device*,
		   const QStringList& options = QStringList() );

      /**
       * Unmounts a device via HAL
       * 
       * Be aware that once the method returns the HAL deamon has not necessarily 
       * finished the procedure yet.
       *
       * \param dev The device to lock
       * \return An error code
       *
       * \see ErrorCode
       */
      int eject( Device*,
		 const QStringList& options = QStringList() );

    signals:
      /**
       * This signal gets emitted whenever HAL finds a new optical drive.
       *
       * \param dev The block device name of the new drive.
       */
      void deviceAdded( const QString& dev );

      /**
       * This signal gets emitted whenever HAL detects that an optical drive
       * has been unplugged.
       *
       * \param dev The block device name of the drive.
       */
      void deviceRemoved( const QString& dev );

      /**
       * This signal gets emitted whenever a new medium is inserted into a
       * device or an inserted is removed (i.e. ejected)
       *
       * \param dev The block device name of the drive the medium is or was inserted into.
       */
      void mediumChanged( const QString& dev );

    private:
      /**
       * HalConnection is a signelton class. Use the instance() method to create it.
       */
      HalConnection( QObject* parent = 0, const char* name = 0 );

      /**
       * Tries to open a connection to HAL.
       */
      bool open();
      void close();

      static HalConnection* s_instance;

      class Private;
      Private* d;

      void setupDBusQtConnection( DBusConnection* dbusConnection );
    };
}

#endif
