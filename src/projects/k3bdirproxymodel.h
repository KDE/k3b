/* 
    SPDX-FileCopyrightText: 2009 Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
    SPDX-FileCopyrightText: 2011 Michal Malek <michalm@jabster.pl>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
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
