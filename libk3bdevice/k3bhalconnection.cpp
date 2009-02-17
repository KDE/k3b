/*
 *
 * Copyright (C) 2005-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bhalconnection.h"
#include "k3bdevice.h"

#include <kdebug.h>
#include <klocale.h>

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusInterface>

#include <Solid/Device>


K3bDevice::HalConnection* K3bDevice::HalConnection::s_instance = 0;


class K3bDevice::HalConnection::Private
{
public:
};


K3bDevice::HalConnection* K3bDevice::HalConnection::instance()
{
    if( s_instance == 0 )
        s_instance = new HalConnection( 0 );
    return s_instance;
}


K3bDevice::HalConnection::HalConnection( QObject* parent )
    : QObject( parent )
{
    d = new Private();
}


K3bDevice::HalConnection::~HalConnection()
{
    s_instance = 0;
    delete d;
}


int K3bDevice::HalConnection::lock( Device* dev )
{
    QDBusInterface halIface( "org.freedesktop.Hal",
                             "/org/freedesktop/Hal/devices/" + dev->solidDevice().udi(),
                             "org.freedesktop.Hal.Device",
                             QDBusConnection::systemBus() );

    QDBusMessage msg = halIface.call( "Lock", QVariant( QLatin1String( "Locked by the K3b libraries" ) ) );

    if ( msg.type() == QDBusMessage::ErrorMessage ) {
        kDebug() << "Failed to lock device through HAL:" << msg.errorMessage();
        if( msg.errorName() == QLatin1String( "org.freedesktop.Hal.NoSuchDevice" ) ) {
            return org_freedesktop_Hal_NoSuchDevice;
        }
        else if( msg.errorName() == QLatin1String( "org.freedesktop.Hal.DeviceAlreadyLocked" ) ) {
            return org_freedesktop_Hal_DeviceAlreadyLocked;
        }
        else if( msg.errorName() == QLatin1String( "org.freedesktop.Hal.PermissionDenied" ) ) {
            return org_freedesktop_Hal_PermissionDenied;
        }
        else {
            kDebug() << "Unkown HAL error:" << msg.errorName();
            return -1;
        }
    }
    else {
        return org_freedesktop_Hal_Success;
    }
}


int K3bDevice::HalConnection::unlock( Device* dev )
{
    QDBusInterface halIface( "org.freedesktop.Hal",
                             "/org/freedesktop/Hal/devices/" + dev->solidDevice().udi(),
                             "org.freedesktop.Hal.Device",
                             QDBusConnection::systemBus() );

    QDBusMessage msg = halIface.call( "Unlock" );

    if ( msg.type() == QDBusMessage::ErrorMessage ) {
        kDebug() << "Failed to unlock device through HAL:" << msg.errorMessage();
        if( msg.errorName() == QLatin1String( "org.freedesktop.Hal.NoSuchDevice" ) ) {
            return org_freedesktop_Hal_NoSuchDevice;
        }
        else if( msg.errorName() == QLatin1String( "org.freedesktop.Hal.DeviceAlreadyLocked" ) ) {
            return org_freedesktop_Hal_DeviceAlreadyLocked;
        }
        else if( msg.errorName() == QLatin1String( "org.freedesktop.Hal.PermissionDenied" ) ) {
            return org_freedesktop_Hal_PermissionDenied;
        }
        else {
            kDebug() << "Unkown HAL error:" << msg.errorName();
            return -1;
        }
    }
    else {
        return org_freedesktop_Hal_Success;
    }
}

#include "k3bhalconnection.moc"
