/*
    SPDX-FileCopyrightText: 2009 Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
    SPDX-FileCopyrightText: 2010-2011 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
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
