/* 
 *
 * Copyright (C) 2005-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_APP_DEVICE_MANAGER_H_
#define _K3B_APP_DEVICE_MANAGER_H_

#include <k3bdevicemanager.h>
#include <KXMLGUIClient>
#include <solid/solidnamespace.h>

class K3bMediaCache;

namespace K3bDevice {
    class Device;
}

/**
 * Enhanced device manager which can do some additional actions
 * and maintains a current device
 */
class K3bAppDeviceManager : public K3bDevice::DeviceManager, public KXMLGUIClient
{
    Q_OBJECT

public:
    K3bAppDeviceManager( QObject* parent = 0 );
    ~K3bAppDeviceManager();

    K3bDevice::Device* currentDevice() const;
    void setMediaCache( K3bMediaCache* c );

Q_SIGNALS:
    void currentDeviceChanged( K3bDevice::Device* );

    /**
     * Emitted when starting to detect the diskinfo. This may be used to show some info
     * to the user since deteting the diskinfo might take some time.
     */
    void detectingDiskInfo( K3bDevice::Device* );

    void mountFinished( const QString& mountPoint );
    void unmountFinished( bool success );

public Q_SLOTS:
    /**
     * \reimplemeted for internal reasons. The API is unaffected.
     */
    void clear();

    void setCurrentDevice( K3bDevice::Device* );

    void diskInfo();
    void unlockDevice();
    void lockDevice();
    void mountDisk();
    void unmountDisk();
    void ejectDisk();
    void loadDisk();
    void setReadSpeed();

    void diskInfo( K3bDevice::Device* );
    void unlockDevice( K3bDevice::Device* );
    void lockDevice( K3bDevice::Device* );
    void mountDisk( K3bDevice::Device* );
    void unmountDisk( K3bDevice::Device* );
    void ejectDisk( K3bDevice::Device* );
    void loadDisk( K3bDevice::Device* );
    void setReadSpeed( K3bDevice::Device* );

private Q_SLOTS:
    void slotMediumChanged( K3bDevice::Device* dev );
    void slotMountChanged( bool accessible, const QString& udi );
    void slotMountFinished( Solid::ErrorType error, QVariant errorData, const QString& udi );
    void slotUnmountFinished( Solid::ErrorType error, QVariant errorData, const QString& udi );

private:
    /**
     * \reimplemeted for internal reasons. The API is unaffected.
     */
    virtual K3bDevice::Device* addDevice( const Solid::Device& solidDev );
    virtual void removeDevice( const Solid::Device& solidDev );

    class Private;
    Private* const d;
};

#endif
