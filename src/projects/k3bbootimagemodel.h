/*
    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2010 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef K3B_K3BBOOTIMAGEMODEL_H
#define K3B_K3BBOOTIMAGEMODEL_H

#include "k3bbootitem.h"
#include <QAbstractItemModel>


namespace K3b {
    
    class DataDoc;

    class BootImageModel : public QAbstractTableModel
    {
        Q_OBJECT
        
    public:
        enum Columns {
            EmulationTypeColumn,
            SizeColumn,
            LocalPathColumn,
            NumColumns
        };
        
    public:
        explicit BootImageModel( DataDoc* doc, QObject* parent = 0 );
        ~BootImageModel() override;
        
        BootItem* bootItemForIndex( const QModelIndex& index ) const;
        QModelIndex indexForBootItem( BootItem* bootItem, int column = EmulationTypeColumn ) const;
        
        void createBootItem( const QString& file, BootItem::ImageType imageType );
        void setImageType( const QModelIndex& index, BootItem::ImageType imageType );
        
        QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const override;
        int columnCount( const QModelIndex& parent = QModelIndex() ) const override;
        int rowCount( const QModelIndex& parent = QModelIndex() ) const override;
        QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const override;
        bool removeRows( int row, int count, const QModelIndex& parent = QModelIndex() ) override;
        
    private:
        DataDoc* m_doc;
    };

}

#endif // K3B_K3BBOOTIMAGEMODEL_H
