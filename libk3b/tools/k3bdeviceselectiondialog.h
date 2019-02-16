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
