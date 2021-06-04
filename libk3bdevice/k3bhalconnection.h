/* 

    SPDX-FileCopyrightText: 2005-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_HAL_CONNECTION_H_
#define _K3B_HAL_CONNECTION_H_

#include <config-k3b.h>

#include "k3bdevice_export.h"

#include <QObject>
#include <QMap>
#include <QStringList>


namespace K3b {
    namespace Device {

        class Device;

        /**
         * This is a simple HAL/DBUS wrapper.
         *
         * It only provides methods to lock and unlock a device since Solid does not provide those.
         */
        class LIBK3BDEVICE_EXPORT HalConnection : public QObject
        {
            Q_OBJECT

        public:
            /**
             * Use instance() to get the single global object
             */
            explicit HalConnection( QObject* = 0 );
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
             * Error codes named as the HAL daemon raises them
             */
            enum ErrorCode {
                org_freedesktop_Hal_Success = 0, //*< The operation was successful. This code does not match any in HAL
                org_freedesktop_Hal_CommunicationError, //*< DBus communication error. This code does not match any in HAL
                org_freedesktop_Hal_NoSuchDevice,
                org_freedesktop_Hal_DeviceAlreadyLocked,
                org_freedesktop_Hal_DeviceNotLocked,
                org_freedesktop_Hal_Device_InterfaceAlreadyLocked,
                org_freedesktop_Hal_Device_InterfaceNotLocked,
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
                org_freedesktop_Hal_Device_Volume_InvalidEjectOption,
                org_freedesktop_Hal_Unknown //*< Unknown error. This code does not match any in HAL
            };

        public Q_SLOTS:
            /**
             * Lock the device in HAL
             * 
             * Be aware that once the method returns the HAL daemon has not necessarily 
             * finished the procedure yet.
             *
             * \param dev The device to lock
             * \return An error code
             *
             * \see ErrorCode
             */
            ErrorCode lock( Device* );

            /**
             * Unlock a previously locked device in HAL
             * 
             * Be aware that once the method returns the HAL daemon has not necessarily 
             * finished the procedure yet.
             *
             * \param dev The device to lock
             * \return An error code
             *
             * \see ErrorCode
             */
            ErrorCode unlock( Device* );

        private:
            class Private;
            Private* d;
        };
    }
}

#endif
