/* 
 * Copyright (C) 2009      Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
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
    DirProxyModel( QObject *parent = 0 );
    ~DirProxyModel();

protected:
    /**
     * This function (reimplemented from QSortFilterProxyModel) allows to decide
     * which rows to show. In this specific model we only allow directories to
     * be shown.
     */
    bool filterAcceptsRow( int source_row, const QModelIndex & source_parent ) const;

};
}

#endif
