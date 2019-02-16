/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
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


#ifndef K3BDEVICEMANAGER_H
#define K3BDEVICEMANAGER_H

#include "k3bdevice_export.h"

#include <QDebug>
#include <QObject>
#include <QString>

class KConfigGroup;

namespace Solid {
    class Device;
}

namespace K3b {
    namespace Device {

        class Device;

        /**
         * \brief Manages all devices.
         *
         * Searches the system for devices and maintains lists of them.
         *
         * <b>Basic usage:</b>
         * \code
         *   K3b::Device::DeviceManager* manager = new K3b::Device::DeviceManager( this );
         *   manager->scanBus();
         *   K3b::Device::Device* dev = manager->findDevice( "/dev/cdrom" );
         * \endcode
         */
        class LIBK3BDEVICE_EXPORT DeviceManager : public QObject
        {
            Q_OBJECT

        public:
            /**
             * Creates a new DeviceManager
             */
            explicit DeviceManager( QObject* parent = 0 );
            ~DeviceManager() override;

            /**
             * By default the DeviceManager makes the Devices check their writing modes.
             * This includes commands to be sent which require writing permissions on the
             * devices and might take some time.
             *
             * So if you don't need the information about the writing modes use this method
             * to speed up the device detection procedure.
             *
             * Be aware that this only refers to CD writing modes. If you only want to handle
             * DVD devices it's always save to set this to false.
             */
            void setCheckWritingModes( bool b );

            /**
             * \deprecated use findDevice( const QString& )
             */
            Device* deviceByName( const QString& );

            /**
             * Search a device by blockdevice name.
             *
             * \return The corresponding device or 0 if there is no such device.
             */
            Device* findDevice( const QString& devicename );

            /**
             * Search a device by UDI.
             *
             * \return The corresponding device or 0 if there is no such device.
             */
            Device* findDeviceByUdi( const QString& udi );

            /**
             * Before getting the devices do a @ref scanBus().
             * \return List of all cd writer devices.
             * \deprecated use cdWriter()
             */
            QList<Device*> burningDevices() const;

            /**
             * \return List of all reader devices without writer devices.
             * \deprecated use cdReader()
             **/
            QList<Device*> readingDevices() const;

            /**
             * Before getting the devices do a @ref scanBus() or add 
             * devices via addDevice( const QString& ).
             *
             * \return List of all devices.
             */
            QList<Device*> allDevices() const;

            /**
             * Before getting the devices do a @ref scanBus() or add 
             * devices via addDevice( const QString& ).
             *
             * \return List of all cd writer devices.
             */
            QList<Device*> cdWriter() const;

            /**
             * Before getting the devices do a @ref scanBus() or add 
             * devices via addDevice( const QString& ).
             *
             * \return List of all cd reader devices.
             */
            QList<Device*> cdReader() const;

            /**
             * Before getting the devices do a @ref scanBus() or add 
             * devices via addDevice( const QString& ).
             *
             * \return List of all DVD writer devices.
             */
            QList<Device*> dvdWriter() const;

            /**
             * Before getting the devices do a @ref scanBus() or add 
             * devices via addDevice( const QString& ).
             *
             * \return List of all DVD reader devices.
             */
            QList<Device*> dvdReader() const;

            /**
             * Before getting the devices do a @ref scanBus() or add 
             * devices via addDevice( const QString& ).
             *
             * \return List of all Blue Ray reader devices.
             */
            QList<Device*> blueRayReader() const;

            /**
             * Before getting the devices do a @ref scanBus() or add 
             * devices via addDevice( const QString& ).
             *
             * \return List of all Blue Ray writer devices.
             */
            QList<Device*> blueRayWriters() const;

            /**
             * Reads the device information from the config file.
             */
            virtual bool readConfig( const KConfigGroup& );

            virtual bool saveConfig( KConfigGroup );


        public Q_SLOTS:
            /**
             * Writes a list of all devices to stderr.
             */
            void printDevices();

            /**
             * Scan the system for devices. Call this to initialize all devices.
             *
             * \return Number of found devices.
             **/
            virtual int scanBus();

            /**
             * Clears the writers and readers list of devices.
             */
            virtual void clear();

        Q_SIGNALS:
            /**
             * Emitted if the device configuration changed, i.e. a device was added or removed.
             */
            void changed( K3b::Device::DeviceManager* );
            void changed();

            private Q_SLOTS:
            K3b::Device::Device* checkDevice( const Solid::Device& dev );
            void slotSolidDeviceAdded( const QString& );
            void slotSolidDeviceRemoved( const QString& );

        protected:
            /**
             * Add a new device.
             *
             * Called by scanBus()
             *
             * \return The device if it could be found or 0 otherwise.
             */
            virtual Device* addDevice( const Solid::Device& dev );

            /**
             * Remove a device from the device manager.
             */
            virtual void removeDevice( const Solid::Device& dev );

        private:
            class Private;
            Private* const d;

            /**
             * Add a device to the managers device lists and initialize the device.
             */
            Device *addDevice( Device* );
        };
    }
}

#endif
