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

#include "k3bhalconnection.h"

#include <kdebug.h>

QMap<LibHalContext*, K3bDevice::HalConnection*> K3bDevice::HalConnection::s_contextMap;
static K3bDevice::HalConnection* s_setupHal = 0;

K3bDevice::HalConnection::HalConnection( QObject* parent, const char* name )
  : QObject( parent, name ),
    m_halContext(0),
    m_dBusQtConnection(0)
{
}


K3bDevice::HalConnection::~HalConnection()
{
  close();
}


bool K3bDevice::HalConnection::open()
{
  close();

#ifdef HAL_0_4
  kdDebug() << "(K3bDevice::HalConnection) initializing HAL 0.4" << endl;

  m_halFunctions.main_loop_integration    = K3bDevice::HalConnection::halMainLoopIntegration;
  m_halFunctions.device_added             = K3bDevice::HalConnection::halDeviceAdded;
  m_halFunctions.device_removed           = K3bDevice::HalConnection::halDeviceRemoved;
  m_halFunctions.device_new_capability    = 0;
  m_halFunctions.device_lost_capability   = 0;
  m_halFunctions.device_property_modified = 0;
  m_halFunctions.device_condition         = 0;

  s_setupHal = this;
  m_halContext = hal_initialize( &m_halFunctions, false );
  s_setupHal = 0;
  if( !m_halContext ) {
    kdDebug() << "(K3bDevice::HalConnection) unable to create HAL context." << endl;
    return false;
  }

  if( libhal_device_property_watch_all( m_halContext, 0 ) ) {
    kdDebug() << "(K3bDevice::HalConnection) Failed to watch HAL properties!" << endl;
    return false;
  }

#else // HAL >= 0.5
  kdDebug() << "(K3bDevice::HalConnection) initializing HAL >= 0.5" << endl;

  m_halContext = libhal_ctx_new();
  if( !m_halContext ) {
    kdDebug() << "(K3bDevice::HalConnection) unable to create HAL context." << endl;
    return false;
  }

  DBusError error;
  dbus_error_init( &error );
  DBusConnection* dbus_connection = dbus_bus_get( DBUS_BUS_SYSTEM, &error );
  if( dbus_error_is_set(&error) ) {
    kdDebug() << "(K3bDevice::HalConnection) unable to connect to DBUS." << endl;
    return false;
  }

  setupDBusQtConnection( dbus_connection );

  libhal_ctx_set_dbus_connection( m_halContext, dbus_connection );
  
  libhal_ctx_set_device_added( m_halContext, K3bDevice::HalConnection::halDeviceAdded );
  libhal_ctx_set_device_removed( m_halContext, K3bDevice::HalConnection::halDeviceRemoved );
  libhal_ctx_set_device_new_capability( m_halContext, 0 );
  libhal_ctx_set_device_lost_capability( m_halContext, 0 );
  libhal_ctx_set_device_property_modified( m_halContext, 0 );
  libhal_ctx_set_device_condition( m_halContext, 0 );
  
  if( !libhal_ctx_init( m_halContext, 0 ) ) {
    kdDebug() << "(K3bDevice::HalConnection) Failed to init HAL context!" << endl;
    return false;
  }
#endif

  // register us so the static hal callbacks can find us
  s_contextMap[m_halContext] = this;

  // report all already detected devices
  QStringList devs = devices();
  for( QStringList::const_iterator it = devs.constBegin(); it != devs.constEnd(); ++it )
    emit deviceAdded( *it );

  return true;
}


void K3bDevice::HalConnection::close()
{
  if( m_halContext ) {
    // remove us from the map
    s_contextMap.remove( m_halContext );

    // clear the context
#ifdef HAL_0_4
    hal_shutdown( m_halContext );
#else
    libhal_ctx_shutdown( m_halContext, 0 );
    libhal_ctx_free( m_halContext );
#endif

    // delete the connection (may be 0 if open() failed)
    delete m_dBusQtConnection;

    m_halContext = 0;
    m_dBusQtConnection = 0;
  }
}


QStringList K3bDevice::HalConnection::devices() const
{
  QStringList devs;
  if( m_halContext ) {
    int numDevices;
    char** halDeviceList = libhal_get_all_devices( m_halContext, &numDevices, 0 );
    for( int i = 0; i < numDevices; ++i ) {
      QString dev = getSystemDeviceForCdrom( halDeviceList[i] );
      if( !dev.isEmpty() )
	devs.append( dev );
    }
  }
  return devs;
}


QString K3bDevice::HalConnection::getSystemDeviceForCdrom( const char* udi ) const
{
  // ignore devices that have no property "info.capabilities" to supress error messages
  if( !libhal_device_property_exists( m_halContext, udi, "info.capabilities", 0 ) )
    return QString::null;

  if( libhal_device_query_capability( m_halContext, udi, "storage.cdrom", 0 ) ) {
    char* dev = libhal_device_get_property_string( m_halContext, udi, "block.device", 0 );
    if( dev ) {
      QString s( dev );
      libhal_free_string( dev );
      return s;
    }
  }

  return QString::null;
}


void K3bDevice::HalConnection::addDevice( const char* udi )
{
  QString s = getSystemDeviceForCdrom( udi );
  if( !s.isEmpty() )
    emit deviceAdded( s );
}


void K3bDevice::HalConnection::removeDevice( const char* udi )
{
  QString s = getSystemDeviceForCdrom( udi );
  if( !s.isEmpty() )
    emit deviceRemoved( s );
}


void K3bDevice::HalConnection::setupDBusQtConnection( DBusConnection* dbusConnection )
{
  m_dBusQtConnection = new DBusQt::Connection( this );
  m_dBusQtConnection->dbus_connection_setup_with_qt_main( dbusConnection );
}


// CALLBACKS
void K3bDevice::HalConnection::halDeviceAdded( LibHalContext* ctx, const char* udi )
{
  //  kdDebug() << k_funcinfo << endl;
  HalConnection* con = s_contextMap[ctx];
  con->addDevice( udi );
}


void K3bDevice::HalConnection::halDeviceRemoved( LibHalContext* ctx, const char* udi )
{
  //  kdDebug() << k_funcinfo << endl;
  HalConnection* con = s_contextMap[ctx];
  con->removeDevice( udi );
}

#ifdef HAL_0_4
void K3bDevice::HalConnection::halMainLoopIntegration( LibHalContext* ctx, DBusConnection* dbus_connection )
{
  Q_UNUSED(ctx)
  // we cannot use the map here since this is used while creating the context
  s_setupHal->setupDBusQtConnection( dbus_connection );
}
#endif

#include "k3bhalconnection.moc"
