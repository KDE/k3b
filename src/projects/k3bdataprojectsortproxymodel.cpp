/*
    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2010 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
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
