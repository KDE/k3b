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

#include "k3bhalconnection.h"
#include "k3bdevice.h"

#include <k3bdebug.h>
#include <klocale.h>

#include <qtimer.h>

// We acknowledge the the dbus API is unstable
#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/connection.h>
#include <dbus/dbus.h>
#include <hal/libhal.h>


static char** qstringListToArray( const QStringList& s )
{
  char** a = new char*[s.count()];
  for( unsigned int i = 0; i < s.count(); ++i ) {
    a[i] = new char[s[i].length()+1];
    ::strncpy( a[i], s[i].local8Bit().data(), s[i].length() );
    a[s[i].length()] = '\0';
  }
  return a;
}

static void freeArray( char** a, unsigned int length )
{
  for( unsigned int i = 0; i < length; ++i )
    delete [] a[i];
  delete a;
}


// CALLBACKS
void halDeviceAdded( LibHalContext* ctx, const char* udi )
{
  Q_UNUSED( ctx );
  k3bDebug() << "adding udi   " << udi << endl;
  K3bDevice::HalConnection::instance()->addDevice( udi );
}


void halDeviceRemoved( LibHalContext* ctx, const char* udi )
{
  Q_UNUSED( ctx );
  k3bDebug() << "removing udi " << udi << endl;
  K3bDevice::HalConnection::instance()->removeDevice( udi );
}


K3bDevice::HalConnection* K3bDevice::HalConnection::s_instance = 0;


class K3bDevice::HalConnection::Private
{
public:
  Private()
    : halContext(0),
      dBusQtConnection(0),
      bOpen(false) {
  }

  LibHalContext* halContext;
  DBusConnection* connection;
  DBusQt::Connection* dBusQtConnection;

  bool bOpen;

  QMap<QCString, QString> udiDeviceMap;
  QMap<QString, QCString> deviceUdiMap;

  QMap<QCString, QCString> deviceMediumUdiMap;
};


K3bDevice::HalConnection* K3bDevice::HalConnection::instance()
{
  if( s_instance == 0 )
    s_instance = new HalConnection( 0 );

  if( !s_instance->isConnected() && !s_instance->open() )
      k3bDebug() << "(K3bDevice::HalConnection) failed to open connection to HAL." << endl;

  return s_instance;
}


K3bDevice::HalConnection::HalConnection( QObject* parent, const char* name )
  : QObject( parent, name )
{
  d = new Private();
}


K3bDevice::HalConnection::~HalConnection()
{
  s_instance = 0;
  close();
  delete d;
}


bool K3bDevice::HalConnection::isConnected() const
{
  return d->bOpen;
}


bool K3bDevice::HalConnection::open()
{
  close();

  k3bDebug() << "(K3bDevice::HalConnection) initializing HAL >= 0.5" << endl;

  d->halContext = libhal_ctx_new();
  if( !d->halContext ) {
    k3bDebug() << "(K3bDevice::HalConnection) unable to create HAL context." << endl;
    return false;
  }

  DBusError error;
  dbus_error_init( &error );
  d->connection = dbus_bus_get( DBUS_BUS_SYSTEM, &error );
  if( dbus_error_is_set(&error) ) {
    k3bDebug() << "(K3bDevice::HalConnection) unable to connect to DBUS: " << error.message << endl;
    return false;
  }

  setupDBusQtConnection( d->connection );

  libhal_ctx_set_dbus_connection( d->halContext, d->connection );

  libhal_ctx_set_device_added( d->halContext, halDeviceAdded );
  libhal_ctx_set_device_removed( d->halContext, halDeviceRemoved );
  libhal_ctx_set_device_new_capability( d->halContext, 0 );
  libhal_ctx_set_device_lost_capability( d->halContext, 0 );
  libhal_ctx_set_device_property_modified( d->halContext, 0 );
  libhal_ctx_set_device_condition( d->halContext, 0 );

  if( !libhal_ctx_init( d->halContext, 0 ) ) {
    k3bDebug() << "(K3bDevice::HalConnection) Failed to init HAL context!" << endl;
    return false;
  }

  d->bOpen = true;

  //
  // report all devices
  //
  int numDevices;
  char** halDeviceList = libhal_get_all_devices( d->halContext, &numDevices, 0 );
  for( int i = 0; i < numDevices; ++i )
    addDevice( halDeviceList[i] );

  return true;
}


void K3bDevice::HalConnection::close()
{
  if( d->halContext ) {
    // clear the context
    if( isConnected() )
      libhal_ctx_shutdown( d->halContext, 0 );
    libhal_ctx_free( d->halContext );

    // delete the connection (may be 0 if open() failed)
    delete d->dBusQtConnection;

    d->halContext = 0;
    d->dBusQtConnection = 0;
    d->bOpen = false;
  }
}


QStringList K3bDevice::HalConnection::devices() const
{
  return QStringList( d->udiDeviceMap.values() );
}


void K3bDevice::HalConnection::addDevice( const char* udi )
{
  // ignore devices that have no property "info.capabilities" to suppress error messages
  if( !libhal_device_property_exists( d->halContext, udi, "info.capabilities", 0 ) )
    return;

  if( libhal_device_query_capability( d->halContext, udi, "storage.cdrom", 0 ) ) {
    char* dev = libhal_device_get_property_string( d->halContext, udi, "block.device", 0 );
    if( dev ) {
      QString s( dev );
      libhal_free_string( dev );

      if( !s.isEmpty() ) {
	k3bDebug() << "Mapping udi " << udi << " to device " << s << endl;
	d->udiDeviceMap[udi] = s;
	d->deviceUdiMap[s] = udi;
	emit deviceAdded( s );
      }
    }
  }
  else {
    if( libhal_device_property_exists( d->halContext, udi, "block.storage_device", 0 ) ) {
      char* deviceUdi = libhal_device_get_property_string( d->halContext, udi, "block.storage_device", 0 );
      if( deviceUdi ) {
	QCString du( deviceUdi );
	libhal_free_string( deviceUdi );

	if( d->udiDeviceMap.contains( du ) ) {
	  //
	  // A new medium has been inserted. Save this medium's udi so we can reuse it later
	  // on for the mount/unmount/eject methods
	  //
	  d->deviceMediumUdiMap[du] = QCString( udi );
	  emit mediumChanged( d->udiDeviceMap[du] );
	}
      }
    }
  }
}


void K3bDevice::HalConnection::removeDevice( const char* udi )
{
  QMapIterator<QCString, QString> it = d->udiDeviceMap.find( udi );
  if( it != d->udiDeviceMap.end() ) {
    k3bDebug() << "Unmapping udi " << udi << " from device " << it.data() << endl;
    emit deviceRemoved( it.data() );
    d->udiDeviceMap.erase( it );
    d->deviceUdiMap.erase( it.data() );
  }
  else {
    if( libhal_device_property_exists( d->halContext, udi, "block.storage_device", 0 ) ) {
      char* deviceUdi = libhal_device_get_property_string( d->halContext, udi, "block.storage_device", 0 );
      if( deviceUdi ) {
	QCString du( deviceUdi );
	libhal_free_string( deviceUdi );

	if( d->udiDeviceMap.contains( du ) ) {
	  //
	  // A medium has been removed/ejected.
	  //
	  d->deviceMediumUdiMap[du] = 0;
	  emit mediumChanged( d->udiDeviceMap[du] );
	}
      }
    }
  }
}


int K3bDevice::HalConnection::lock( Device* dev )
{
  //
  // The code below is based on the code from kioslave/media/mediamanager/halbackend.cpp in the kdebase package
  // Copyright (c) 2004-2005 Jérôme Lodewyck <jerome dot lodewyck at normalesup dot org>
  //
  DBusMessage* dmesg = 0;
  DBusMessage* reply = 0;
  DBusError error;

  if( !d->deviceUdiMap.contains( dev->blockDeviceName() ) ) {
    return org_freedesktop_Hal_Device_Volume_NoSuchDevice;
  }

  QCString udi = d->deviceUdiMap[dev->blockDeviceName()];

  if( !( dmesg = dbus_message_new_method_call( "org.freedesktop.Hal", udi.data(),
					       "org.freedesktop.Hal.Device",
					       "Lock" ) ) ) {
    k3bDebug() << "(K3bDevice::HalConnection) lock failed for " << udi << ": could not create dbus message\n";
    return org_freedesktop_Hal_CommunicationError;
  }

  const char* lockComment = "Locked by the K3b libraries";

  if( !dbus_message_append_args( dmesg,
				 DBUS_TYPE_STRING, &lockComment,
				 DBUS_TYPE_INVALID ) ) {
    k3bDebug() << "(K3bDevice::HalConnection) lock failed for " << udi << ": could not append args to dbus message\n";
    dbus_message_unref( dmesg );
    return org_freedesktop_Hal_CommunicationError;
  }

  int ret = org_freedesktop_Hal_Success;

  dbus_error_init( &error );
  reply = dbus_connection_send_with_reply_and_block( d->connection, dmesg, -1, &error );
  if( dbus_error_is_set( &error ) ) {
    kdError() << "(K3bDevice::HalConnection) lock failed for " << udi << ": " << error.name << " - " << error.message << endl;
    if( !strcmp(error.name, "org.freedesktop.Hal.NoSuchDevice" ) )
      ret = org_freedesktop_Hal_NoSuchDevice;
    else if( !strcmp(error.name, "org.freedesktop.Hal.DeviceAlreadyLocked" ) )
      ret = org_freedesktop_Hal_DeviceAlreadyLocked;
    else if( !strcmp(error.name, "org.freedesktop.Hal.PermissionDenied" ) )
      ret = org_freedesktop_Hal_PermissionDenied;

    dbus_error_free( &error );
  }
  else
    k3bDebug() << "(K3bDevice::HalConnection) lock queued for " << udi << endl;

  dbus_message_unref( dmesg );
  if( reply )
    dbus_message_unref( reply );

  return ret;
}


int K3bDevice::HalConnection::unlock( Device* dev )
{
  //
  // The code below is based on the code from kioslave/media/mediamanager/halbackend.cpp in the kdebase package
  // Copyright (c) 2004-2005 Jérôme Lodewyck <jerome dot lodewyck at normalesup dot org>
  //
  DBusMessage* dmesg = 0;
  DBusMessage* reply = 0;
  DBusError error;

  if( !d->deviceUdiMap.contains( dev->blockDeviceName() ) ) {
    return org_freedesktop_Hal_Device_Volume_NoSuchDevice;
  }

  QCString udi = d->deviceUdiMap[dev->blockDeviceName()];

  if( !( dmesg = dbus_message_new_method_call( "org.freedesktop.Hal", udi.data(),
					       "org.freedesktop.Hal.Device",
					       "Unlock" ) ) ) {
    k3bDebug() << "(K3bDevice::HalConnection) unlock failed for " << udi << ": could not create dbus message\n";
    return org_freedesktop_Hal_CommunicationError;
  }

  if( !dbus_message_append_args( dmesg,
				 DBUS_TYPE_INVALID ) ) {
    k3bDebug() << "(K3bDevice::HalConnection) unlock failed for " << udi << ": could not append args to dbus message\n";
    dbus_message_unref( dmesg );
    return org_freedesktop_Hal_CommunicationError;
  }

  int ret = org_freedesktop_Hal_Success;

  dbus_error_init( &error );
  reply = dbus_connection_send_with_reply_and_block( d->connection, dmesg, -1, &error );
  if( dbus_error_is_set( &error ) ) {
    kdError() << "(K3bDevice::HalConnection) unlock failed for " << udi << ": " << error.name << " - " << error.message << endl;
    if( !strcmp(error.name, "org.freedesktop.Hal.NoSuchDevice" ) )
      ret = org_freedesktop_Hal_NoSuchDevice;
    else if( !strcmp(error.name, "org.freedesktop.Hal.DeviceAlreadyLocked" ) )
      ret = org_freedesktop_Hal_DeviceAlreadyLocked;
    else if( !strcmp(error.name, "org.freedesktop.Hal.PermissionDenied" ) )
      ret = org_freedesktop_Hal_PermissionDenied;

    dbus_error_free( &error );
  }
  else
    k3bDebug() << "(K3bDevice::HalConnection) unlock queued for " << udi << endl;

  dbus_message_unref( dmesg );
  if( reply )
    dbus_message_unref( reply );

  return ret;
}


int K3bDevice::HalConnection::mount( K3bDevice::Device* dev,
				     const QString& mountPoint,
				     const QString& fstype,
				     const QStringList& options )
{
  //
  // The code below is based on the code from kioslave/media/mediamanager/halbackend.cpp in the kdebase package
  // Copyright (c) 2004-2005 Jérôme Lodewyck <jerome dot lodewyck at normalesup dot org>
  //
  DBusMessage* dmesg = 0;
  DBusMessage* reply = 0;
  DBusError error;

  if( !d->deviceUdiMap.contains( dev->blockDeviceName() ) )
    return org_freedesktop_Hal_NoSuchDevice;

  if( !d->deviceMediumUdiMap.contains( d->deviceUdiMap[dev->blockDeviceName()] ) )
    return org_freedesktop_Hal_Device_Volume_NoSuchDevice;

  QCString mediumUdi = d->deviceMediumUdiMap[d->deviceUdiMap[dev->blockDeviceName()]];

  if( !( dmesg = dbus_message_new_method_call( "org.freedesktop.Hal", mediumUdi.data(),
					       "org.freedesktop.Hal.Device.Volume",
					       "Mount" ) ) ) {
    k3bDebug() << "(K3bDevice::HalConnection) mount failed for " << mediumUdi << ": could not create dbus message\n";
    return org_freedesktop_Hal_CommunicationError;
  }

  char** poptions = qstringListToArray( options );

  QByteArray strMountPoint = mountPoint.local8Bit();
  QByteArray strFstype = fstype.local8Bit();

  if( !dbus_message_append_args( dmesg,
				 DBUS_TYPE_STRING, strMountPoint.data(),
				 DBUS_TYPE_STRING, strFstype.data(),
				 DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &poptions, options.count(),
				 DBUS_TYPE_INVALID ) ) {
    k3bDebug() << "(K3bDevice::HalConnection) mount failed for " << mediumUdi << ": could not append args to dbus message\n";
    dbus_message_unref( dmesg );
    freeArray( poptions, options.count() );
    return org_freedesktop_Hal_CommunicationError;
  }

  freeArray( poptions, options.count() );

  int ret = org_freedesktop_Hal_Success;

  dbus_error_init( &error );
  reply = dbus_connection_send_with_reply_and_block( d->connection, dmesg, -1, &error );
  if( dbus_error_is_set( &error ) ) {
    kdError() << "(K3bDevice::HalConnection) mount failed for " << mediumUdi << ": " << error.name << " - " << error.message << endl;
    if( !strcmp(error.name, "org.freedesktop.Hal.Device.Volume.NoSuchDevice" ) )
      ret = org_freedesktop_Hal_Device_Volume_NoSuchDevice;
    else if( !strcmp(error.name, "org.freedesktop.Hal.Device.Volume.PermissionDenied" ) )
      ret = org_freedesktop_Hal_Device_Volume_PermissionDenied;
    else if( !strcmp(error.name, "org.freedesktop.Hal.Device.Volume.UnknownFilesystemType" ) )
      ret = org_freedesktop_Hal_Device_Volume_UnknownFilesystemType;
    else if( !strcmp(error.name, "org.freedesktop.Hal.Device.Volume.MountPointNotAvailable" ) )
      ret = org_freedesktop_Hal_Device_Volume_MountPointNotAvailable;
    else if( !strcmp(error.name, "org.freedesktop.Hal.Device.Volume.AlreadyMounted" ) )
      ret = org_freedesktop_Hal_Device_Volume_AlreadyMounted;
    else if( !strcmp(error.name, "org.freedesktop.Hal.Device.Volume.InvalidMountpoint" ) )
      ret = org_freedesktop_Hal_Device_Volume_InvalidMountpoint;
    else if( !strcmp(error.name, "org.freedesktop.Hal.Device.Volume.InvalidMountOption" ) )
      ret = org_freedesktop_Hal_Device_Volume_InvalidMountOption;
    else if( !strcmp(error.name, "org.freedesktop.Hal.Device.Volume.PermissionDeniedByPolicy" ) )
      ret = org_freedesktop_Hal_Device_Volume_PermissionDeniedByPolicy;

    dbus_error_free( &error );
  }
  else
    k3bDebug() << "(K3bDevice::HalConnection) mount queued for " << mediumUdi << endl;

  dbus_message_unref( dmesg );
  if( reply )
    dbus_message_unref( reply );

  return ret;
}


int K3bDevice::HalConnection::unmount( K3bDevice::Device* dev,
				       const QStringList& options )
{
  //
  // The code below is based on the code from kioslave/media/mediamanager/halbackend.cpp in the kdebase package
  // Copyright (c) 2004-2005 Jérôme Lodewyck <jerome dot lodewyck at normalesup dot org>
  //
  DBusMessage* dmesg = 0;
  DBusMessage* reply = 0;
  DBusError error;

  if( !d->deviceUdiMap.contains( dev->blockDeviceName() ) )
    return org_freedesktop_Hal_NoSuchDevice;

  if( !d->deviceMediumUdiMap.contains( d->deviceUdiMap[dev->blockDeviceName()] ) )
    return org_freedesktop_Hal_Device_Volume_NoSuchDevice;

  QCString mediumUdi = d->deviceMediumUdiMap[d->deviceUdiMap[dev->blockDeviceName()]];

  if( !( dmesg = dbus_message_new_method_call( "org.freedesktop.Hal", mediumUdi.data(),
					       "org.freedesktop.Hal.Device.Volume",
					       "Unmount" ) ) ) {
    k3bDebug() << "(K3bDevice::HalConnection) unmount failed for " << mediumUdi << ": could not create dbus message\n";
    return org_freedesktop_Hal_CommunicationError;
  }

  char** poptions = qstringListToArray( options );

  if( !dbus_message_append_args( dmesg,
				 DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &poptions, options.count(),
				 DBUS_TYPE_INVALID ) ) {
    k3bDebug() << "(K3bDevice::HalConnection) unmount failed for " << mediumUdi << ": could not append args to dbus message\n";
    dbus_message_unref( dmesg );
    freeArray( poptions, options.count() );
    return org_freedesktop_Hal_CommunicationError;
  }

  freeArray( poptions, options.count() );

  int ret = org_freedesktop_Hal_Success;

  dbus_error_init( &error );
  reply = dbus_connection_send_with_reply_and_block( d->connection, dmesg, -1, &error );
  if( dbus_error_is_set( &error ) ) {
    kdError() << "(K3bDevice::HalConnection) unmount failed for " << mediumUdi << ": " << error.name << " - " << error.message << endl;
    if( !strcmp(error.name, "org.freedesktop.Hal.Device.Volume.NoSuchDevice" ) )
      ret = org_freedesktop_Hal_Device_Volume_NoSuchDevice;
    else if( !strcmp(error.name, "org.freedesktop.Hal.Device.Volume.PermissionDenied" ) )
      ret = org_freedesktop_Hal_Device_Volume_PermissionDenied;
    else if( !strcmp(error.name, "org.freedesktop.Hal.Device.Volume.MountPointNotAvailable" ) )
      ret = org_freedesktop_Hal_Device_Volume_MountPointNotAvailable;
    else if( !strcmp(error.name, "org.freedesktop.Hal.Device.Volume.InvalidUnmountOption" ) )
      ret = org_freedesktop_Hal_Device_Volume_InvalidUnmountOption;
    else if( !strcmp(error.name, "org.freedesktop.Hal.Device.Volume.InvalidMountpoint" ) )
      ret = org_freedesktop_Hal_Device_Volume_InvalidMountpoint;
    else if( !strcmp(error.name, "org.freedesktop.Hal.Device.Volume.PermissionDeniedByPolicy" ) )
      ret = org_freedesktop_Hal_Device_Volume_PermissionDeniedByPolicy;

    dbus_error_free( &error );
  }
  else
    k3bDebug() << "(K3bDevice::HalConnection) unmount queued for " << mediumUdi << endl;

  dbus_message_unref( dmesg );
  if( reply )
    dbus_message_unref( reply );

  return ret;
}


int K3bDevice::HalConnection::eject( K3bDevice::Device* dev,
				     const QStringList& options )
{
  //
  // The code below is based on the code from kioslave/media/mediamanager/halbackend.cpp in the kdebase package
  // Copyright (c) 2004-2005 Jérôme Lodewyck <jerome dot lodewyck at normalesup dot org>
  //
  DBusMessage* dmesg = 0;
  DBusMessage* reply = 0;
  DBusError error;

  if( !d->deviceUdiMap.contains( dev->blockDeviceName() ) )
    return org_freedesktop_Hal_NoSuchDevice;

  if( !d->deviceMediumUdiMap.contains( d->deviceUdiMap[dev->blockDeviceName()] ) )
    return org_freedesktop_Hal_Device_Volume_NoSuchDevice;

  QCString mediumUdi = d->deviceMediumUdiMap[d->deviceUdiMap[dev->blockDeviceName()]];

  if( !( dmesg = dbus_message_new_method_call( "org.freedesktop.Hal", mediumUdi.data(),
					       "org.freedesktop.Hal.Device.Volume",
					       "Eject" ) ) ) {
    k3bDebug() << "(K3bDevice::HalConnection) eject failed for " << mediumUdi << ": could not create dbus message\n";
    return org_freedesktop_Hal_CommunicationError;
  }

  char** poptions = qstringListToArray( options );

  if( !dbus_message_append_args( dmesg,
				 DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &poptions, options.count(),
				 DBUS_TYPE_INVALID ) ) {
    k3bDebug() << "(K3bDevice::HalConnection) eject failed for " << mediumUdi << ": could not append args to dbus message\n";
    dbus_message_unref( dmesg );
    freeArray( poptions, options.count() );
    return org_freedesktop_Hal_CommunicationError;
  }

  freeArray( poptions, options.count() );

  int ret = org_freedesktop_Hal_Success;

  dbus_error_init( &error );
  reply = dbus_connection_send_with_reply_and_block( d->connection, dmesg, -1, &error );
  if( dbus_error_is_set( &error ) ) {
    kdError() << "(K3bDevice::HalConnection) eject failed for " << mediumUdi << ": " << error.name << " - " << error.message << endl;
    if( !strcmp(error.name, "org.freedesktop.Hal.Device.Volume.NoSuchDevice" ) )
      ret = org_freedesktop_Hal_Device_Volume_NoSuchDevice;
    else if( !strcmp(error.name, "org.freedesktop.Hal.Device.Volume.PermissionDenied" ) )
      ret = org_freedesktop_Hal_Device_Volume_PermissionDenied;
    else if( !strcmp(error.name, "org.freedesktop.Hal.Device.Volume.InvalidEjectOption" ) )
      ret = org_freedesktop_Hal_Device_Volume_InvalidEjectOption;
    else if( !strcmp(error.name, "org.freedesktop.Hal.Device.Volume.PermissionDeniedByPolicy" ) )
      ret = org_freedesktop_Hal_Device_Volume_PermissionDeniedByPolicy;

    dbus_error_free( &error );
  }
  else
    k3bDebug() << "(K3bDevice::HalConnection) eject queued for " << mediumUdi << endl;

  dbus_message_unref( dmesg );
  if( reply )
    dbus_message_unref( reply );

  return ret;
}


void K3bDevice::HalConnection::setupDBusQtConnection( DBusConnection* dbusConnection )
{
  d->dBusQtConnection = new DBusQt::Connection( this );
  d->dBusQtConnection->dbus_connection_setup_with_qt_main( dbusConnection );
}

#include "k3bhalconnection.moc"
