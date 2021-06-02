/*

    SPDX-FileCopyrightText: 2007-2009 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
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
        explicit DeviceModel( QObject* parent = 0 );
        ~DeviceModel() override;

        QList<Device::Device*> devices() const;

        Device::Device* deviceForIndex( const QModelIndex& index ) const;
        QModelIndex indexForDevice( Device::Device* dev ) const;

        int columnCount( const QModelIndex& parent = QModelIndex() ) const override;
        QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const override;
        QModelIndex index( int row, int column, const QModelIndex& parent = QModelIndex() ) const override;
        QModelIndex parent( const QModelIndex& index ) const override;
        int rowCount( const QModelIndex& parent = QModelIndex() ) const override;

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
