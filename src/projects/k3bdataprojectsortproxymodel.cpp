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

#include "k3bdataprojectsortproxymodel.h"
#include "k3bdataprojectmodel.h"

namespace K3b {

DataProjectSortProxyModel::DataProjectSortProxyModel( QObject* parent )
:
    QSortFilterProxyModel( parent )
{
    setDynamicSortFilter( true );
    setSortLocaleAware( true );
    setSortCaseSensitivity( Qt::CaseInsensitive );
    setSortRole( DataProjectModel::SortRole );
}


bool DataProjectSortProxyModel::lessThan( const QModelIndex& left, const QModelIndex& right ) const
{
    const int leftType = left.data( DataProjectModel::ItemTypeRole ).toInt();
    const int rightType = right.data( DataProjectModel::ItemTypeRole ).toInt();
    if( leftType == DataProjectModel::DirItemType && rightType == DataProjectModel::FileItemType )
        return sortOrder() == Qt::AscendingOrder;
    else if( leftType == DataProjectModel::FileItemType && rightType == DataProjectModel::DirItemType )
        return sortOrder() == Qt::DescendingOrder;
    else
        return QSortFilterProxyModel::lessThan( left, right );
}

} // namespace K3b
