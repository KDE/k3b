/*
 *
 * Copyright (C) 2007 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_PLACES_MODEL_H_
#define _K3B_PLACES_MODEL_H_

#include <QAbstractItemModel>

#include <KUrl>

class KIcon;
namespace K3bDevice {
    class DeviceManager;
}

class K3bPlacesModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    K3bPlacesModel( QObject* parent = 0 );
    ~K3bPlacesModel();

    int columnCount( const QModelIndex& parent = QModelIndex() ) const;
    QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const;
    QModelIndex index( int row, int column, const QModelIndex& parent = QModelIndex() ) const;
    QModelIndex parent( const QModelIndex& index ) const;
    int rowCount( const QModelIndex& parent = QModelIndex() ) const;

public Q_SLOTS:
    void addPlace( const QString& name, const KIcon& icon, const KUrl& rootUrl );

private Q_SLOTS:
    void slotDevicesChanged( K3bDevice::DeviceManager* dm );

private:
    class Private;
    Private* const d;
};

#endif
