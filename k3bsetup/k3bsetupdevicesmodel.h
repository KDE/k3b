/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009 Michal Malek <michalm@jabster.pl>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3BSETUPDEVICESMODEL_H_
#define _K3BSETUPDEVICESMODEL_H_

#include <QAbstractItemModel>
#include <QStringList>

class KConfig;

namespace K3b {
    
namespace Device {
    class Device;
}
    
namespace Setup {

    class DevicesModel : public QAbstractItemModel
    {
    Q_OBJECT

    public:
        DevicesModel( QObject* parent = 0 );
        ~DevicesModel();

        void load( const KConfig& config );
        void save( KConfig& config ) const;
        void defaults();

        QStringList selectedDevices() const;
        bool changesNeeded() const;
        
        Device::Device* deviceForIndex( const QModelIndex& index ) const;
        QModelIndex indexForDevice( Device::Device* device ) const;

        QVariant data( const QModelIndex& index, int role ) const;
        bool setData( const QModelIndex& index, const QVariant& value, int role );
        Qt::ItemFlags flags( const QModelIndex& index ) const;
        QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
        QModelIndex index( int row, int column, const QModelIndex& parent = QModelIndex() ) const;
        QModelIndex parent( const QModelIndex& index ) const;
        int rowCount( const QModelIndex& parent = QModelIndex() ) const;
        int columnCount( const QModelIndex& parent = QModelIndex() ) const;

    public Q_SLOTS:
        void setBurningGroup( const QString& burningGroup );
        void update();

    private:
        class Private;
        Private* d;
    };

} // namespace Setup
} // namespace K3b

#endif
