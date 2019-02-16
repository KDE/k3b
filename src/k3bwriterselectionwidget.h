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


#ifndef K3BWRITERSELECTIONWIDGET_H
#define K3BWRITERSELECTIONWIDGET_H

#include "k3bglobals.h"

#include <KSharedConfig>

#include <QWidget>
#include <QLabel>

class QLabel;

namespace K3b {

    class IntMapComboBox;
    class MediaSelectionComboBox;
    class Msf;

    namespace Device {
        class Device;
    }

    class WriterSelectionWidget : public QWidget
    {
        Q_OBJECT

    public:
        /**
         * Creates a writerselectionwidget
         */
        explicit WriterSelectionWidget( QWidget* parent = 0 );
        ~WriterSelectionWidget() override;

        int writerSpeed() const;
        Device::Device* writerDevice() const;

        QList<Device::Device*> allDevices() const;

        /**
         * returns K3b::WritingApp
         */
        K3b::WritingApp writingApp() const;

        Device::MediaTypes wantedMediumType() const;
        Device::MediaStates wantedMediumState() const;
        K3b::Msf wantedMediumSize() const;

        void loadConfig( const KConfigGroup& );
        void saveConfig( KConfigGroup );

    public Q_SLOTS:
        void setWriterDevice( K3b::Device::Device* );
        void setSpeed( int );
        void setWritingApp( K3b::WritingApp app );

        /**
         * K3b::WritingApp or'ed together
         *
         * Defaults to cdrecord and cdrdao for CD and growisofs for DVD
         */
        void setSupportedWritingApps( K3b::WritingApps apps );

        /**
         * A simple hack to disable the speed selection for DVD formatting
         */
        void setForceAutoSpeed( bool );

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
         * Set the wanted medium size. Defaults to 0 which means
         * that the size should be irgnored.
         */
        void setWantedMediumSize( const K3b::Msf& minSize );

        /**
         * This is a hack to allow the copy dialogs to use the same device for reading
         * and writing without having the user to choose the same medium.
         *
         * \param overrideString A string which will be shown in place of the medium string.
         *                       For example: "Burn to the same device". Set it to 0 in order
         *                       to disable the feature.
         */
        void setOverrideDevice( K3b::Device::Device* dev, const QString& overrideString = QString(), const QString& tooltip = QString() );

        /**
         * Compare MediaSelectionComboBox::setIgnoreDevice
         */
        void setIgnoreDevice( K3b::Device::Device* dev );

    Q_SIGNALS:
        void writerChanged();
        void writerChanged( K3b::Device::Device* );
        void writingAppChanged( K3b::WritingApp app );

        /**
         * \see MediaSelectionComboBox
         */
        void newMedia();
        void newMedium( K3b::Device::Device* dev );

    private Q_SLOTS:
        void slotRefreshWriterSpeeds();
        void slotRefreshWritingApps();
        void slotWritingAppSelected( int id );
        void slotConfigChanged( KSharedConfig::Ptr c );
        void slotSpeedChanged( int index );
        void slotWriterChanged();
        void slotNewBurnMedium( K3b::Device::Device* dev );
        void slotManualSpeed();

    private:
        void clearSpeedCombo();
        void insertSpeedItem( int );
        K3b::WritingApp selectedWritingApp() const;

        class MediaSelectionComboBox;

        IntMapComboBox* m_comboSpeed;
        MediaSelectionComboBox* m_comboMedium;
        IntMapComboBox* m_comboWritingApp;
        QLabel* m_writingAppLabel;

        class Private;
        Private* d;
    };
}

#endif
