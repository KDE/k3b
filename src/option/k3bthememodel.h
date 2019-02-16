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

#ifndef K3B_THEMEMODEL_H
#define K3B_THEMEMODEL_H

#include <QAbstractItemModel>


namespace K3b {
    
    class DataDoc;
    class Theme;
    class ThemeManager;

    class ThemeModel : public QAbstractTableModel
    {
        Q_OBJECT
        
    public:
        enum Columns {
            ThemeColumn,
            AuthorColumn,
            VersionColumn,
            CommentColumn,
            NumColumns
        };
        
    public:
        explicit ThemeModel( ThemeManager* themeManager, QObject* parent = 0 );
        ~ThemeModel() override;
        
        /**
         * Re-reads themes on disk and updates the model
         */
        void reload();
        
        Theme* themeForIndex( const QModelIndex& index ) const;
        QModelIndex indexForTheme( Theme* theme, int column = ThemeColumn ) const;
        
        QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const override;
        int columnCount( const QModelIndex& parent = QModelIndex() ) const override;
        int rowCount( const QModelIndex& parent = QModelIndex() ) const override;
        QVariant headerData( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const override;
        bool removeRows( int row, int count, const QModelIndex& parent = QModelIndex() ) override;
        
    private:
        ThemeManager* m_themeManager;
    };

}

#endif // K3B_THEMEMODEL_H
