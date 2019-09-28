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

#ifndef _K3B_MEDIA_SELECTION_COMBOBOX_H_
#define _K3B_MEDIA_SELECTION_COMBOBOX_H_

#include "k3bmedium.h"
#include <KComboBox>
#include <QList>

namespace K3b {
    namespace Device {
        class Device;
        class DeviceManager;
    }
    class Msf;

    /**
     * Combo box which allows one to select a media (in comparison
     * to the DeviceComboBox which allows one to select a device.
     *
     * This class uses the MediaCache to update it's status.
     */
    class MediaSelectionComboBox : public KComboBox
    {
        Q_OBJECT

    public:
        explicit MediaSelectionComboBox( QWidget* parent );
        ~MediaSelectionComboBox() override;

        /**
         * Although the widget allows selection of media this
         * results in a device being selected.
         */
        Device::Device* selectedDevice() const;

        QList<Device::Device*> allDevices() const;

        Device::MediaTypes wantedMediumType() const;
        Device::MediaStates wantedMediumState() const;
        Medium::MediumContents wantedMediumContent() const;
        K3b::Msf wantedMediumSize() const;

    Q_SIGNALS:
        /**
         * Be aware that his signal will also be emitted in case
         * no medium is available with a null pointer.
         */
        void selectionChanged( K3b::Device::Device* );

        /**
         * This signal is emitted if the selection of media changed.
         * This includes a change due to changing the wanted medium state.
         */
        void newMedia();

        void newMedium( K3b::Device::Device* dev );

    public Q_SLOTS:
        /**
         * Only works in case the device actually contains a usable medium.
         * Otherwise the currently selected medium stays selected.
         */
        void setSelectedDevice( K3b::Device::Device* );

        /**
         * Set the wanted medium type. Defaults to writable CD.
         *
         * \param type a bitwise combination of the Device::MediaType enum
         */
        void setWantedMediumType( K3b::Device::MediaTypes type );

        /**
         * Set the wanted medium state. Defaults to empty media.
         *
         * \param state a bitwise combination of the Device::State enum
         */
        void setWantedMediumState( K3b::Device::MediaStates state );

        /**
         * Set the wanted medium content type. The default is Medium::ContentAll.
         *
         * \param content A bitwise or of Medium::MediumContent
         */
        void setWantedMediumContent( K3b::Medium::MediumContents content );

        /**
         * Set the wanted medium size. Defaults to 0 which means
         * that the size should be irgnored.
         */
        void setWantedMediumSize( const K3b::Msf& minSize );

        /**
         * Set the device to ignore. This device will not be checked for
         * wanted media. This is many useful for media copy.
         *
         * \param dev The device to ignore or 0 to not ignore any device.
         */
        void setIgnoreDevice( K3b::Device::Device* dev );

    private Q_SLOTS:
        void slotMediumChanged( K3b::Device::Device* );
        void slotDeviceManagerChanged( K3b::Device::DeviceManager* );
        void slotActivated( int i );
        void slotUpdateToolTip( K3b::Device::Device* );

    protected:
        void updateMedia();
        virtual bool showMedium( const Medium& ) const;
        virtual QString mediumString( const Medium& ) const;
        virtual QString mediumToolTip( const Medium& ) const;
        virtual QString noMediumMessage() const;
        QStringList noMediumMessages() const;

    private:
        void updateMedium( Device::Device* );
        void addMedium( Device::Device* );
        void showNoMediumMessage();
        void clear();

        class Private;
        Private* d;
    };
}

#endif
