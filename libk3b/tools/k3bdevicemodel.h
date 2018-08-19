/*
 *
 * Copyright (C) 2007-2009 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_DEVICE_MODEL_H_
#define _K3B_DEVICE_MODEL_H_

#include "k3b_export.h"

#include <QAbstractItemModel>

namespace K3b {
    namespace Device {
        class Device;
    }

    class LIBK3B_EXPORT DeviceModel : public QAbstractItemModel
    {
        Q_OBJECT

    public:
        DeviceModel( QObject* parent = 0 );
        ~DeviceModel();

        QList<Device::Device*> devices() const;

        Device::Device* deviceForIndex( const QModelIndex& index ) const;
        QModelIndex indexForDevice( Device::Device* dev ) const;

        int columnCount( const QModelIndex& parent = QModelIndex() ) const;
        QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const;
        QModelIndex index( int row, int column, const QModelIndex& parent = QModelIndex() ) const;
        QModelIndex parent( const QModelIndex& index ) const;
        int rowCount( const QModelIndex& parent = QModelIndex() ) const;

        enum DeviceRoles {
            IsDevice = 1000, //**< boolean value only used to check if we have a device item */
            Vendor,
            Description,
            BlockDevice,
            Valid
        };

    public Q_SLOTS:
        void addDevice( K3b::Device::Device* );
        void addDevices( const QList<K3b::Device::Device*>& );
        void setDevices( const QList<K3b::Device::Device*>& devices );
        void removeDevice( K3b::Device::Device* dev );
        void clear();

    private Q_SLOTS:
        void slotMediumChanged( K3b::Device::Device* dev );
        void slotCheckingMedium( K3b::Device::Device* dev, const QString& );

    private:
        class Private;
        Private* const d;
    };
}

#endif
