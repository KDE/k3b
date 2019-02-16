/*
 *
 * Copyright (C) 2008 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_DEVICE_MENU_H_
#define _K3B_DEVICE_MENU_H_

#include <QMenu>

namespace Device {
    class Device;
}

namespace K3b {
    /**
     * Provides a menu including actions depending on the media
     * type.
     * For example: an audio CD has actions for ripping, copying,
     * or re-querying cddb.
     * Mounted media have an action to unmount while unmounted media
     * have an action to mount.
     */
    class DeviceMenu : public QMenu
    {
        Q_OBJECT

    public:
        explicit DeviceMenu( QWidget* parent = 0 );
        ~DeviceMenu() override;

    private:
        class Private;
        Private* const d;

        Q_PRIVATE_SLOT( d, void _k_aboutToShow() )
        Q_PRIVATE_SLOT( d, void _k_copy() )
        Q_PRIVATE_SLOT( d, void _k_format() )
        Q_PRIVATE_SLOT( d, void _k_ripAudio() )
        Q_PRIVATE_SLOT( d, void _k_ripVcd() )
        Q_PRIVATE_SLOT( d, void _k_ripVideoDVD() )
        Q_PRIVATE_SLOT( d, void _k_continueMultisession() )
    };
}

#endif
