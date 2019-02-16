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


#ifndef _K3B_MEDIA_CACHE_H_
#define _K3B_MEDIA_CACHE_H_

#include "k3b_export.h"

#include "k3bdevice.h"
#include "k3btoc.h"
#include "k3bcdtext.h"
#include "k3bdiskinfo.h"

#include "k3bmedium.h"

#include <QMutex>
#include <QObject>
#include <QThread>

namespace K3b {
    namespace Device {
        class DeviceManager;
    }


    /**
     * The Media Cache does know the status of all devices at all times
     * (except for blocked devices).
     *
     * It should be used to get information about media and device status
     * instead of the libk3bdevice methods for faster access.
     *
     * The Media Cache polls for new information every 2 seconds on all devices
     * (except for blocked ones) and emits signals in case a device status changed
     * (for example a media was inserted or removed).
     *
     * To start the media caching call buildDeviceList().
     */
    class LIBK3B_EXPORT MediaCache : public QObject
    {
        Q_OBJECT

    public:
        explicit MediaCache( QObject* parent = 0 );
        ~MediaCache() override;

        /**
         * block a device so it will not be polled. This is used
         * to disable polling on devices that are currently in use
         * for burning.
         *
         * \return A unique id to be used to unblock the device or -1 if the device
         *         is already blocked.
         */
        int blockDevice( Device::Device* dev );

        /**
         * Unblock a device that has been blocked with block() before.
         *
         * \param id The id returned by the previous call to block(). This makes
         *           sure only the one who did the block may unblock the device.
         *
         * \return true if dev has been blocked with id before. false otherwise.
         */
        bool unblockDevice( Device::Device* dev, int id );

        bool isBlocked( Device::Device* dev );

        /**
         * Read cached medium information.
         */
        Medium medium( Device::Device* dev );

        /**
         * Read cached disk information.
         */
        Device::DiskInfo diskInfo( Device::Device* );

        /**
         * Read cached Table of contents.
         */
        Device::Toc toc( Device::Device* );

        /**
         * Read cached CD text from an Audio CD.
         */
        Device::CdText cdText( Device::Device* );

        /**
         * Read cached supported writing speeds.
         */
        QList<int> writingSpeeds( Device::Device* );

        /**
         * \see Medium::shortString()
         */
        QString mediumString( Device::Device* device, bool useContent = true );

    Q_SIGNALS:
        /**
         * Signal emitted whenever a medium changes. That means when a new medium is inserted
         * or an old one is removed.
         *
         * This signal will also be emitted when a previously blocked device becomes unblocked.
         *
         * Be aware though that the Media Cache will silently ignore removed devices. That means
         * one should also listen to Device::DeviceManager::changed() in case a USB drive or
         * something similar is removed.
         */
        void mediumChanged( K3b::Device::Device* dev );

        /**
         * Emitted when the cache analysis a new medium. This might be emitted multiple times
         * with different messages.
         *
         * \param dev The device being analysed.
         * \param message An optional message to display more details to the user.
         *
         * Analyzation of the medium is finished once mediumChanged has been emitted.
         */
        void checkingMedium( K3b::Device::Device* dev, const QString& message );
        
        /**
         * Emitted whenever medium CDDB information changes.
         */
        void mediumCddbChanged( K3b::Device::Device* dev );

    public Q_SLOTS:
        /**
         * Build the device list and start the polling.
         * It might make sense to connect this to Device::DeviceManager::changed()
         */
        void buildDeviceList( K3b::Device::DeviceManager* );

        /**
         * Clear the device list and stop all the polling.
         * This is also done in the destructor.
         */
        void clearDeviceList();

        /**
         * Perform a new cddb query to update the information. This may be useful
         * to let the user select a different entry in case of a multiple entry result
         * or to re-query after enabling a new cddb source.
         *
         * Will result in a mediumChanged signal for media that have audio content.
         */
        void lookupCddb( K3b::Device::Device* );

        /**
         * Reset a device, i.e. force an update of the device.
         *
         * This is useful to ensure that after an eject the cache is still valid.
         */
        void resetDevice( K3b::Device::Device* );

    private:
        class PollThread;
        class DeviceEntry;

        class Private;
        Private* const d;

        DeviceEntry* findDeviceEntry( Device::Device* );

        Q_PRIVATE_SLOT( d, void _k_mediumChanged( K3b::Device::Device* ) )
        Q_PRIVATE_SLOT( d, void _k_cddbJobFinished( KJob* job ) )
    };
}

#endif
