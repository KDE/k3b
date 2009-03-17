/*
 * Copyright (C) 2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009 Arthur Mello <arthur@mandriva.com>
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

#ifndef _K3B_VCD_PROJECT_MODEL_H_
#define _K3B_VCD_PROJECT_MODEL_H_

#include <QtCore/QAbstractItemModel>

namespace K3b {
    class VcdDoc;
}
namespace K3b {
    class VcdTrack;
}

namespace K3b {
    class VcdProjectModel : public QAbstractItemModel
    {
        Q_OBJECT

        public:
            VcdProjectModel( VcdDoc* doc, QObject* parent );
            ~VcdProjectModel();

            enum Columns {
                NoColumn = 0,
                TitleColumn,
                TypeColumn,
                ResolutionColumn,
                HighResolutionColumn,
                FrameRateColumn,
                MuxRateColumn,
                DurationColumn,
                SizeColumn,
                FilenameColumn,
                NumColumns
            };

            VcdDoc* project() const;

            VcdTrack* trackForIndex( const QModelIndex& index ) const;
            QModelIndex indexForTrack( VcdTrack* track ) const;

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
    };
}

#endif
