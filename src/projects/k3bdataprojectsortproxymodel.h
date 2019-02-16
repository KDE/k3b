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

#ifndef K3BDATAPROJECTSORTPROXYMODEL_H
#define K3BDATAPROJECTSORTPROXYMODEL_H

#include <QSortFilterProxyModel>


namespace K3b {

    /**
     * \class DataProjectSortProxyModel
     * Proxy model used for sorting right part of Data Project view.
     * Folders are always shown above files.
     */
    class DataProjectSortProxyModel : public QSortFilterProxyModel
    {
    public:
        explicit DataProjectSortProxyModel( QObject* parent = 0 );

    protected:
        bool lessThan(const QModelIndex& left, const QModelIndex& right) const override;
    };

}

#endif // K3BDATAPROJECTSORTPROXYMODEL_H
