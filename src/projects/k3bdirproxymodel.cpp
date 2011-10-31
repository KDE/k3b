/*
 *
 * Copyright (C) 2009 Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
 * Copyright (C) 2010-2011 Michal Malek <michalm@jabster.pl>
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

#include "k3bdirproxymodel.h"
#include "k3bdataprojectmodel.h"


K3b::DirProxyModel::DirProxyModel(QObject *parent)
: QSortFilterProxyModel(parent)
{
    setDynamicSortFilter( true );
    sort( DataProjectModel::FilenameColumn, Qt::AscendingOrder );
}

K3b::DirProxyModel::~DirProxyModel()
{
}


bool K3b::DirProxyModel::filterAcceptsRow( int source_row, const QModelIndex & source_parent ) const
{
    QAbstractItemModel *model = sourceModel();
    QModelIndex index = model->index(source_row, 0, source_parent);

    QVariant data = index.data( DataProjectModel::ItemTypeRole );
    DataProjectModel::ItemType type = DataProjectModel::FileItemType;

    if (data.isValid())
        type = (DataProjectModel::ItemType) index.data( DataProjectModel::ItemTypeRole ).toInt();

    return (type == DataProjectModel::DirItemType);
}

bool K3b::DirProxyModel::lessThan( const QModelIndex& left, const QModelIndex& right ) const
{
    if( !left.parent().isValid() && !right.parent().isValid() ) {
        return left.row() < right.row();
    } else {
        return QSortFilterProxyModel::lessThan( left, right );
    }
}
