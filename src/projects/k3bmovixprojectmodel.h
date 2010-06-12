/*
 * Copyright (C) 2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009 Arthur Renato Mello <arthur@mandriva.com>
 * Copyright (C) 2009 Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
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
            QModelIndex indexForSubtitle( MovixSubtitleItem* item ) const;

            virtual QModelIndex index( int row, int column, const QModelIndex& parent = QModelIndex() ) const;
            virtual QModelIndex parent( const QModelIndex& index ) const;
            virtual int rowCount( const QModelIndex& parent = QModelIndex() ) const;
            virtual int columnCount( const QModelIndex& parent = QModelIndex() ) const;
            virtual QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const;
            virtual bool setData( const QModelIndex& index, const QVariant& value, int role = Qt::EditRole );
            virtual QVariant headerData( int section, Qt::Orientation orientation, int role ) const;
            virtual Qt::ItemFlags flags( const QModelIndex& index ) const;
            virtual Qt::DropActions supportedDropActions() const;
            virtual QMimeData* mimeData( const QModelIndexList& indexes ) const;
            virtual QStringList mimeTypes() const;
            virtual bool dropMimeData( const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent );
            virtual bool removeRows( int row, int count, const QModelIndex& parent = QModelIndex() );

    private:
        class Private;
        Private* const d;

        Q_PRIVATE_SLOT( d, void _k_itemsAboutToBeInserted(int, int) )
        Q_PRIVATE_SLOT( d, void _k_itemsInserted() )
        Q_PRIVATE_SLOT( d, void _k_itemsAboutToBeRemoved(int, int) )
        Q_PRIVATE_SLOT( d, void _k_itemsRemoved() )
        Q_PRIVATE_SLOT( d, void _k_subTitleAboutToBeInserted(K3b::MovixFileItem*) )
        Q_PRIVATE_SLOT( d, void _k_subTitleInserted() )
        Q_PRIVATE_SLOT( d, void _k_subTitleAboutToBeRemoved(K3b::MovixFileItem*) )
        Q_PRIVATE_SLOT( d, void _k_subTitleRemoved() )
    };
}

#endif
