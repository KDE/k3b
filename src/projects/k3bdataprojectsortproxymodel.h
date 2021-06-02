/*

    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2010 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
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
