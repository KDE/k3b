/* 
 * Copyright (C) 2009 Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
 * Copyright (C) 2011 Michal Malek <michalm@jabster.pl>
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

#ifndef K3BDIRPROXYMODEL_H
#define K3BDIRPROXYMODEL_H

#include <QSortFilterProxyModel>

/**
 * This class is used to show only directories from a specific model.
 * It is used in the dir panel (of StandardView) to show a directory
 * tree.
 *
 * @author Gustavo Pichorim Boiko
 */
namespace K3b {
class DirProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit DirProxyModel( QObject *parent = 0 );
    ~DirProxyModel() override;

protected:
    /**
     * This function (reimplemented from QSortFilterProxyModel) allows one to decide
     * which rows to show. In this specific model we only allow directories to
     * be shown.
     */
    bool filterAcceptsRow( int source_row, const QModelIndex & source_parent ) const override;

    /**
     * Reimplemented method from QSortFilterProxyModel. Prevents from sorting
     * top-level elements. Sorting top-level elements is not desirable in MixedView as
     * we need fixed order of items there (first audio part, second data part)
     */
    bool lessThan( const QModelIndex& left, const QModelIndex& right ) const override;
};
}

#endif
