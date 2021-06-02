/*
    SPDX-FileCopyrightText: 2008 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2009 Arthur Mello <arthur@mandriva.com>
    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_VCD_PROJECT_MODEL_H_
#define _K3B_VCD_PROJECT_MODEL_H_

#include <QAbstractTableModel>

namespace K3b {
    class VcdDoc;
    class VcdTrack;
    
    class VcdProjectModel : public QAbstractTableModel
    {
        Q_OBJECT

    public:
        explicit VcdProjectModel( VcdDoc* doc, QObject* parent = 0 );
        ~VcdProjectModel() override;

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

        VcdDoc* doc() const;

        VcdTrack* trackForIndex( const QModelIndex& index ) const;
        QModelIndex indexForTrack( VcdTrack* track, int column = NoColumn ) const;

        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        bool setData( const QModelIndex& index, const QVariant& value, int role = Qt::EditRole ) override;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
        Qt::ItemFlags flags( const QModelIndex& index ) const override;
        Qt::DropActions supportedDragActions() const override;
        Qt::DropActions supportedDropActions() const override;
        QMimeData* mimeData( const QModelIndexList& indexes ) const override;
        QStringList mimeTypes() const override;
        bool dropMimeData( const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent ) override;
        bool removeRows( int row, int count, const QModelIndex& parent = QModelIndex() ) override;

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
