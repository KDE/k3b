/*
 *
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2010 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef K3B_EXTERNALBINMODEL_H
#define K3B_EXTERNALBINMODEL_H

#include <QAbstractItemModel>

namespace K3b {
    
    class DataDoc;
    class ExternalBin;
    class ExternalBinManager;
    class ExternalProgram;

    class ExternalBinModel : public QAbstractItemModel
    {
        Q_OBJECT
        
    public:
        enum Columns {
            PathColumn,
            VersionColumn,
            FeaturesColumn,
            NumColumns
        };
        
    public:
        ExternalBinModel( ExternalBinManager* manager, QObject* parent = 0 );
        ~ExternalBinModel();
        
        /**
         * Reloads programs from ExternalBinManager and updates the model
         */
        void reload();
        
        /**
         * Saves all changes made in model to ExternalBinManager
         */
        void save();
        
        ExternalProgram* programForIndex( const QModelIndex& index ) const;
        QModelIndex indexForProgram( ExternalProgram* program, int column = PathColumn ) const;
        
        const ExternalBin* binForIndex( const QModelIndex& index ) const;
        QModelIndex indexForBin( const ExternalBin* bin, int column = PathColumn ) const;
        
        virtual QModelIndex index( int row, int column, const QModelIndex& parent = QModelIndex() ) const;
        virtual QModelIndex parent( const QModelIndex& index ) const;
        virtual Qt::ItemFlags flags( const QModelIndex& index ) const;
        virtual QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const;
        virtual bool setData( const QModelIndex& index, const QVariant& value, int role );
        virtual int columnCount( const QModelIndex& parent = QModelIndex() ) const;
        virtual int rowCount( const QModelIndex& parent = QModelIndex() ) const;
        virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
        
    private:
        class Private;
        Private* d;
    };

}

#endif // K3B_EXTERNALBINMODEL_H
