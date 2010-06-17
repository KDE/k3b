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

#ifndef K3B_EXTERNALBINPARAMSMODEL_H
#define K3B_EXTERNALBINPARAMSMODEL_H

#include <QAbstractItemModel>

namespace K3b {
    
    class DataDoc;
    class ExternalProgram;
    class ExternalBinManager;

    class ExternalBinParamsModel : public QAbstractTableModel
    {
        Q_OBJECT
        
    public:
        enum Columns {
            ProgramColumn,
            ParametersColumn,
            NumColumns
        };
        
    public:
        ExternalBinParamsModel( ExternalBinManager* manager, QObject* parent = 0 );
        ~ExternalBinParamsModel();
        
        void reload();
        void save();
        
        ExternalProgram* programForIndex( const QModelIndex& index ) const;
        QModelIndex indexForProgram( ExternalProgram* program, int column = ProgramColumn ) const;
        
        virtual Qt::ItemFlags flags( const QModelIndex& index ) const;
        virtual QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const;
        virtual bool setData( const QModelIndex& index, const QVariant& value, int role = Qt::EditRole );
        virtual int columnCount( const QModelIndex& parent = QModelIndex() ) const;
        virtual int rowCount( const QModelIndex& parent = QModelIndex() ) const;
        virtual QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;
        
    private:
        class Private;
        Private* d;
    };

}

#endif // K3B_EXTERNALBINPARAMSMODEL_H
