/*
 *
 * Copyright (C) 2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_DATA_PROJECT_MODEL_H_
#define _K3B_DATA_PROJECT_MODEL_H_

#include <QtCore/QAbstractItemModel>

class K3bDataDoc;
class K3bDataItem;
class K3bDirItem;

namespace K3b {
    class DataProjectModel : public QAbstractItemModel
    {
        Q_OBJECT

    public:
        DataProjectModel( K3bDataDoc* doc, QObject* parent );
        ~DataProjectModel();

        enum Columns {
            FilenameColumn = 0,
            TypeColumn,
            SizeColumn,
            LocalPathColumn,
            LinkColumn,
            NumColumns
        };

        K3bDataDoc* project() const;

        K3bDataItem* itemForIndex( const QModelIndex& index ) const;
        QModelIndex indexForItem( K3bDataItem* item ) const;

        int columnCount( const QModelIndex& parent = QModelIndex() ) const;
        QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const;
        QVariant headerData ( int section, Qt::Orientation orientation, int role ) const;
        Qt::ItemFlags flags( const QModelIndex& index ) const;
        QModelIndex index( int row, int column, const QModelIndex& parent = QModelIndex() ) const;
        QModelIndex parent( const QModelIndex& index ) const;
        int rowCount( const QModelIndex& parent = QModelIndex() ) const;
        bool setData( const QModelIndex& index, const QVariant& value, int role = Qt::EditRole );
        QMimeData* mimeData( const QModelIndexList& indexes ) const;
        Qt::DropActions supportedDropActions() const;
        QStringList mimeTypes() const;
        bool dropMimeData( const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent );

    private:
        class Private;
        Private* const d;

        Q_PRIVATE_SLOT( d, void _k_aboutToAddItem( K3bDirItem* dir, K3bDataItem* item ) )
        Q_PRIVATE_SLOT( d, void _k_aboutToRemoveItem( K3bDataItem* item ) )
        Q_PRIVATE_SLOT( d, void _k_itemAdded( K3bDataItem* item ) )
        Q_PRIVATE_SLOT( d, void _k_itemRemoved( K3bDataItem* item ) )
    };
}

#endif
