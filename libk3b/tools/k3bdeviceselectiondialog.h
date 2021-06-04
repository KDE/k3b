/*
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/


#ifndef K3B_DEVICE_SELECTION_DIALOG_H
#define K3B_DEVICE_SELECTION_DIALOG_H


#include "k3b_export.h"
#include <QList>
#include <QDialog>

namespace K3b {
    namespace Device {
        class Device;
    }


    class LIBK3B_EXPORT DeviceSelectionDialog : public QDialog
    {
        Q_OBJECT

    public:
        explicit DeviceSelectionDialog( QWidget* parent = 0,
                               const QString& text = QString() );
        ~DeviceSelectionDialog() override;

        void addDevice( Device::Device* );
        void addDevices( const QList<Device::Device*>& );

        void setSelectedDevice( Device::Device* );

        Device::Device* selectedDevice() const;

        static Device::Device* selectWriter( QWidget* parent,
                                                  const QString& text = QString() );
        static Device::Device* selectDevice( QWidget* parent,
                                                  const QString& text = QString() );
        static Device::Device* selectDevice( QWidget* parent,
                                                  const QList<Device::Device*>& devices,
                                                  const QString& text = QString() );

    private:
        class Private;
        Private* d;
    };
}

#endif
