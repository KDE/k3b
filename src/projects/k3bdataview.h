/*
    SPDX-FileCopyrightText: 2009-2010 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/


#ifndef K3BDATAVIEW_H
#define K3BDATAVIEW_H

#include "k3bview.h"

class QModelIndex;
class QTreeView;

namespace K3b {
    class DataDoc;
    class DataProjectModel;
    class DataViewImpl;
    class DirProxyModel;

    class DataView : public View
    {
        Q_OBJECT

    public:
        explicit DataView( DataDoc* doc, QWidget* parent = 0 );
        ~DataView() override;

    public Q_SLOTS:
        void slotBurn() override;
        void addUrls( const QList<QUrl>& urls ) override;

    private Q_SLOTS:
        void slotParentDir();
        void slotCurrentDirChanged();
        void slotSetCurrentRoot( const QModelIndex& index );

    protected:
        ProjectBurnDialog* newBurnDialog( QWidget* parent = 0 ) override;

    private:
        DataDoc* m_doc;
        DataViewImpl* m_dataViewImpl;
        QTreeView* m_dirView;
        DirProxyModel* m_dirProxy;
    };
}

#endif
