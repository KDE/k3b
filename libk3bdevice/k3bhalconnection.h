/* 
 *
 * $Id: sourceheader,v 1.3 2005/01/19 13:03:46 trueg Exp $
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2005 Sebastian Trueg <trueg@k3b.org>
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

#include <qobject.h>
#include <qmap.h>
#include <qstringlist.h>

// We acknowledge the the dbus API is unstable
#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/connection.h>
#include <hal/libhal.h>


// The HAL API changed between 0.4 and 0.5 series.
// These defines enable backward compatibility
#ifdef HAL_0_4
#define libhal_free_string hal_free_string
#define libhal_device_exists(ctx, udi, error) hal_device_exists(ctx, udi)
#define libhal_device_property_watch_all(ctx, error) hal_device_property_watch_all(ctx)
#define libhal_get_all_devices(ctx, num_devices, error) hal_get_all_devices(ctx, num_devices)
#define libhal_device_property_exists(ctx, udi, key, error) hal_device_property_exists(ctx, udi, key)
#define libhal_device_get_property_bool(ctx, udi, key, error) hal_device_get_property_bool(ctx, udi, key)
#define libhal_device_get_property_string(ctx, udi, key, error) hal_device_get_property_string(ctx, udi, key)
#define libhal_device_query_capability(ctx, udi, capability, error) hal_device_query_capability(ctx, udi, capability)
#endif

namespace K3bDevice {

  /**
   * This is a simple HAL/DBUS wrapper which just checks if 
   * CDROM devices have been added or removed.
   *
   * This class does not deal with K3b devices but with system device names
   * such as /dev/cdrom. These device names can be used in DeviceManager::findDevice().
   */
  class HalConnection : public QObject
    {
      Q_OBJECT

    public:
      HalConnection( QObject* parent = 0, const char* name = 0 );
      ~HalConnection();

      /**
       * Tries to open a connection to HAL.
       *
       * \return true if a connection to HAL has been established. In this case
       *         there will also already be deviceAdded signals.
       */
      bool open();
      void close();

      /**
       * \return a list of CDROM devices as reported by HAL.
       */
      QStringList devices() const;

    signals:
      void deviceAdded( const QString& dev );
      void deviceRemoved( const QString& dev );

    private:
      LibHalContext* m_halContext;
      DBusQt::Connection* m_dBusQtConnection;
#ifdef HAL_0_4
      LibHalFunctions m_halFunctions;
#endif

      QMap<QCString, QString> m_udiDeviceMap;

      /**
       * \return the system device for cdrom devices and an empty string for all other devices.
       */
      QString getSystemDeviceForCdrom( const char* udi ) const;

      // used by the static callbacks
      void addDevice( const char* udi );
      void removeDevice( const char* udi );
      void setupDBusQtConnection( DBusConnection* dbusConnection );

      // HAL callback methods
      static void halDeviceAdded( LibHalContext* ctx, const char* udi );
      static void halDeviceRemoved( LibHalContext* ctx, const char* udi );

#ifdef HAL_0_4
      static void halMainLoopIntegration( LibHalContext* ctx, DBusConnection* dbus_connection );
#endif

      // since HalConnection is not a singlelton we need to have a context->HalConnection mapping
      static QMap<LibHalContext*, HalConnection*> s_contextMap;
    };
}

#endif
