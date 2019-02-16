/*
 *
 * Copyright (C) 2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010-2011 Michal Malek <michalm@jabster.pl>
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

#include <QAbstractItemModel>
#include <QUrl>

namespace K3b {
    class DataDoc;
    class DataItem;
    class DirItem;

    class DataProjectModel : public QAbstractItemModel
    {
        Q_OBJECT

    public:
        explicit DataProjectModel( DataDoc* doc, QObject* parent = 0 );
        ~DataProjectModel() override;

        enum Columns {
            FilenameColumn = 0,
            TypeColumn,
            SizeColumn,
            NumColumns
        };

        enum AdditionalRoles
        {
            ItemTypeRole = Qt::UserRole,  ///< returns int which is a combination of ItemType
            CustomFlagsRole,              ///< returns int which is a combination of ItemFlags
            SortRole                      ///< returns data most suitable for sorting
        };

        enum ItemType
        {
            DirItemType,
            FileItemType
        };

        enum ItemFlags
        {
            ItemIsRemovable = 1
        };

        DataDoc* project() const;

        DataItem* itemForIndex( const QModelIndex& index ) const;
        QModelIndex indexForItem( DataItem* item ) const;

        int columnCount( const QModelIndex& parent = QModelIndex() ) const override;
        QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const override;
        QVariant headerData ( int section, Qt::Orientation orientation, int role ) const override;
        Qt::ItemFlags flags( const QModelIndex& index ) const override;
        QModelIndex index( int row, int column, const QModelIndex& parent = QModelIndex() ) const override;
        QModelIndex parent( const QModelIndex& index ) const override;
        int rowCount( const QModelIndex& parent = QModelIndex() ) const override;
        bool setData( const QModelIndex& index, const QVariant& value, int role = Qt::EditRole ) override;
        QMimeData* mimeData( const QModelIndexList& indexes ) const override;
        Qt::DropActions supportedDropActions() const override;
        QStringList mimeTypes() const override;
        bool dropMimeData( const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent ) override;
        bool removeRows( int row, int count, const QModelIndex& parent = QModelIndex() ) override;
        QModelIndex buddy( const QModelIndex& index ) const override;

    Q_SIGNALS:
        void addUrlsRequested( QList<QUrl> urls, K3b::DirItem* targetDir );
        void moveItemsRequested( QList<K3b::DataItem*> items, K3b::DirItem* targetDir );

    private:
        class Private;
        Private* const d;

        Q_PRIVATE_SLOT( d, void _k_itemsAboutToBeInserted( K3b::DirItem* parent, int start, int end ) )
        Q_PRIVATE_SLOT( d, void _k_itemsAboutToBeRemoved( K3b::DirItem* parent, int start, int end ) )
        Q_PRIVATE_SLOT( d, void _k_itemsInserted( K3b::DirItem* parent, int start, int end ) )
        Q_PRIVATE_SLOT( d, void _k_itemsRemoved( K3b::DirItem* parent, int start, int end ) )
        Q_PRIVATE_SLOT( d, void _k_volumeIdChanged() )
    };
}

#endif
