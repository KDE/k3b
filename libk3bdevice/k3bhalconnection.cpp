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


Q_GLOBAL_STATIC( K3b::Device::HalConnection, s_instance )


class K3b::Device::HalConnection::Private
{
public:
};


K3b::Device::HalConnection* K3b::Device::HalConnection::instance()
{
    return s_instance();
}


K3b::Device::HalConnection::HalConnection( QObject* parent )
    : QObject( parent )
{
    d = new Private();
}


K3b::Device::HalConnection::~HalConnection()
{
    delete d;
}


int K3b::Device::HalConnection::lock( Device* dev )
{
    kDebug() << dev->blockDeviceName();

    QDBusInterface halIface( "org.freedesktop.Hal",
                             dev->solidDevice().udi(),
                             "org.freedesktop.Hal.Device",
                             QDBusConnection::systemBus() );

    if ( halIface.isValid() ) {
        QDBusMessage msg = halIface.call( QLatin1String( "Lock" ), QLatin1String( "Locked by the K3b libraries" ) );

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
    else {
        kDebug() << "Could not connect to device object:" << halIface.path();
        return org_freedesktop_Hal_CommunicationError;
    }
}


int K3b::Device::HalConnection::unlock( Device* dev )
{
    kDebug() << dev->blockDeviceName();

    QDBusInterface halIface( "org.freedesktop.Hal",
                             dev->solidDevice().udi(),
                             "org.freedesktop.Hal.Device",
                             QDBusConnection::systemBus() );

    if ( halIface.isValid() ) {
        QDBusMessage msg = halIface.call( QLatin1String( "Unlock" ) );

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
    else {
        kDebug() << "Could not connect to device object:" << halIface.path();
        return org_freedesktop_Hal_CommunicationError;
    }
}

#include "k3bhalconnection.moc"
