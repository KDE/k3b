/*
    SPDX-FileCopyrightText: 2005-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3bhalconnection.h"
#include "k3bdevice.h"
#include "k3bdevice_i18n.h"

#include <Solid/Device>

#include <QDebug>
#include <QDBusConnection>
#include <QDBusInterface>


Q_GLOBAL_STATIC( K3b::Device::HalConnection, s_instance )


namespace {
    
    K3b::Device::HalConnection::ErrorCode toErrorCode( const QString& errorName )
    {
        if( errorName == QLatin1String( "org.freedesktop.Hal.NoSuchDevice" ) ) {
            return K3b::Device::HalConnection::org_freedesktop_Hal_NoSuchDevice;
        }
        else if( errorName == QLatin1String( "org.freedesktop.Hal.DeviceAlreadyLocked" ) ) {
            return K3b::Device::HalConnection::org_freedesktop_Hal_DeviceAlreadyLocked;
        }
        else if( errorName == QLatin1String( "org.freedesktop.Hal.DeviceNotLocked" ) ) {
            return K3b::Device::HalConnection::org_freedesktop_Hal_DeviceNotLocked;
        }
        else if( errorName == QLatin1String( "org.freedesktop.Hal.Device.InterfaceAlreadyLocked" ) ) {
            return K3b::Device::HalConnection::org_freedesktop_Hal_Device_InterfaceAlreadyLocked;
        }
        else if( errorName == QLatin1String( "org.freedesktop.Hal.Device.InterfaceNotLocked" ) ) {
            return K3b::Device::HalConnection::org_freedesktop_Hal_Device_InterfaceNotLocked;
        }
        else if( errorName == QLatin1String( "org.freedesktop.Hal.PermissionDenied" ) ) {
            return K3b::Device::HalConnection::org_freedesktop_Hal_PermissionDenied;
        }
        else {
            qDebug() << "Unknown HAL error:" << errorName;
            return K3b::Device::HalConnection::org_freedesktop_Hal_Unknown;
        }
    }
    
} // namespace


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


K3b::Device::HalConnection::ErrorCode K3b::Device::HalConnection::lock( Device* dev )
{
    qDebug() << dev->blockDeviceName();

    QDBusInterface halIface( "org.freedesktop.Hal",
                             dev->solidDevice().udi(),
                             "org.freedesktop.Hal.Device",
                             QDBusConnection::systemBus() );

    if ( halIface.isValid() ) {
        
        QDBusMessage msg = halIface.call( QLatin1String( "Lock" ), QLatin1String( "Locked by the K3b libraries" ) );
        if ( msg.type() == QDBusMessage::ErrorMessage ) {
            qDebug() << "Failed to lock device through HAL:" << msg.errorMessage();
            return toErrorCode( msg.errorName() );
        }
        
        bool exclusive = true;
        msg = halIface.call( QLatin1String( "AcquireInterfaceLock" ), QLatin1String( "org.freedesktop.Hal.Device.Storage" ), exclusive );
        if ( msg.type() == QDBusMessage::ErrorMessage ) {
            qDebug() << "Failed to acquire storage interface lock through HAL:" << msg.errorMessage();
            return toErrorCode( msg.errorName() );
        }
        
        return org_freedesktop_Hal_Success;
    }
    else {
        qDebug() << "Could not connect to device object:" << halIface.path();
        return org_freedesktop_Hal_CommunicationError;
    }
}


K3b::Device::HalConnection::ErrorCode K3b::Device::HalConnection::unlock( Device* dev )
{
    qDebug() << dev->blockDeviceName();

    QDBusInterface halIface( "org.freedesktop.Hal",
                             dev->solidDevice().udi(),
                             "org.freedesktop.Hal.Device",
                             QDBusConnection::systemBus() );

    if ( halIface.isValid() ) {
        
        QDBusMessage msg = halIface.call( QLatin1String( "ReleaseInterfaceLock" ), QLatin1String( "org.freedesktop.Hal.Device.Storage" ) );
        if ( msg.type() == QDBusMessage::ErrorMessage ) {
            qDebug() << "Failed to release storage interface lock through HAL:" << msg.errorMessage();
            return toErrorCode( msg.errorName() );
        }
        
        msg = halIface.call( QLatin1String( "Unlock" ) );
        if ( msg.type() == QDBusMessage::ErrorMessage ) {
            qDebug() << "Failed to unlock device through HAL:" << msg.errorMessage();
            return toErrorCode( msg.errorName() );
        }
        
        return org_freedesktop_Hal_Success;
    }
    else {
        qDebug() << "Could not connect to device object:" << halIface.path();
        return org_freedesktop_Hal_CommunicationError;
    }
}

#include "moc_k3bhalconnection.cpp"
