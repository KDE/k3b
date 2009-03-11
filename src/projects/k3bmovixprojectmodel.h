/*
 * Copyright (C) 2008 Sebastian Trueg <trueg@k3b.org>
 *           (C) 2009 Arthur Renato Mello <arthur@mandriva.com>
 *           (C) 2009 Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
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

#ifndef _K3B_MOVIX_PROJECT_MODEL_H_
#define _K3B_MOVIX_PROJECT_MODEL_H_

#include <QtCore/QAbstractItemModel>

namespace K3b {
    class MovixDoc;
    class MovixFileItem;
    class MovixSubtitleItem;
}

namespace K3b {
    class MovixProjectModel : public QAbstractItemModel
    {
        Q_OBJECT

        public:
            MovixProjectModel( MovixDoc* doc, QObject* parent );
            ~MovixProjectModel();

            enum Columns {
                NoColumn = 0,
                TitleColumn,
                TypeColumn,
                SizeColumn,
                LocalPathColumn,
                LinkColumn,
                NumColumns
            };

            MovixDoc* project() const;

            MovixFileItem* itemForIndex( const QModelIndex& index ) const;
            QModelIndex indexForItem( MovixFileItem* track ) const;

            MovixSubtitleItem* subtitleForIndex( const QModelIndex& index ) const;
            QModelIndex indexForSubtitle( MovixSubtitleItem* track ) const;

            QModelIndex index(int row, int column,
                const QModelIndex& parent = QModelIndex()) const;
            QModelIndex parent(const QModelIndex& index) const;
            int rowCount(const QModelIndex& parent = QModelIndex()) const;
            int columnCount(const QModelIndex& parent = QModelIndex()) const;
            QVariant data(const QModelIndex& index,
                int role = Qt::DisplayRole) const;
            bool setData( const QModelIndex& index, const QVariant& value,
                int role = Qt::EditRole );
            QVariant headerData(int section, Qt::Orientation orientation,
                int role) const;
            Qt::ItemFlags flags( const QModelIndex& index ) const;

            Qt::DropActions supportedDropActions() const;

            QMimeData* mimeData( const QModelIndexList& indexes ) const;
            QStringList mimeTypes() const;
            bool dropMimeData( const QMimeData* data,
                Qt::DropAction action, int row, int column,
                const QModelIndex& parent );

            virtual bool removeRows( int row, int count, const QModelIndex& parent = QModelIndex() );

    private:
        class Private;
        Private* const d;

        Q_PRIVATE_SLOT( d, void _k_aboutToAddRows(int, int))
        Q_PRIVATE_SLOT( d, void _k_addedRows())
        Q_PRIVATE_SLOT( d, void _k_aboutToRemoveRows(int, int))
        Q_PRIVATE_SLOT( d, void _k_removedRows())
        Q_PRIVATE_SLOT( d, void _k_subTitleAdded(K3b::MovixFileItem*))
        Q_PRIVATE_SLOT( d, void _k_subTitleRemoved(K3b::MovixFileItem*))

    };
}

#endif
