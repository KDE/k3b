/*
 *
 * $Id: k3bdevicecombobox.cpp 735766 2007-11-12 15:25:22Z trueg $
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_DEVICE_LIST_MODEL_H_
#define _K3B_DEVICE_LIST_MODEL_H_

namespace K3bDevice {
    class Device;
    class DeviceManager;
}

#include <QtCore/QAbstractItemModel>


class K3bDeviceListModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    K3bDeviceListModel( QObject* parent );
    ~K3bDeviceListModel();

    void addDevice( K3bDevice::Device* );
    void addDevices( const QList<K3bDevice::Device*>& );
    void setDevices( const QList<K3bDevice::Device*>& );
    void removeDevice( K3bDevice::Device* dev );
    void clear();

    K3bDevice::Device* deviceForIndex( const QModelIndex& index ) const;

    int columnCount( const QModelIndex & parent = QModelIndex() ) const;
    QVariant data( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    QModelIndex index ( int row, int column, const QModelIndex & parent = QModelIndex() ) const;
    QModelIndex parent ( const QModelIndex & index ) const;
    int rowCount ( const QModelIndex & parent = QModelIndex() ) const;

private Q_SLOTS:
    void slotDeviceManagerChanged( K3bDevice::DeviceManager* dm );

private:
    class Private;
    Private* const d;
};

#endif
