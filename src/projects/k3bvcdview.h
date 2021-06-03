/*
    SPDX-FileCopyrightText: 2003-2004 Christian Kvasny <chris@k3b.org>
    SPDX-FileCopyrightText: 2009 Arthur Renato Mello <arthur@mandriva.com>
    SPDX-FileCopyrightText: 2009-2010 Michal Malek <michalm@jabster.pl>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef K3BVCDVIEW_H
#define K3BVCDVIEW_H

#include "k3bview.h"

#include <QModelIndex>

class QAction;
class QTreeView;
class QWidget;

namespace K3b {

    class ProjectBurnDialog;
    class VcdDoc;
    class VcdProjectModel;

    class VcdView : public View
    {
        Q_OBJECT

        public:
            VcdView( VcdDoc* doc, QWidget* parent );
            ~VcdView() override;

        private Q_SLOTS:
            void slotSelectionChanged();
            void slotProperties() override;
            void slotRemove();
            void slotItemActivated( const QModelIndex& index );

        protected:
            ProjectBurnDialog* newBurnDialog( QWidget* parent = 0 ) override;

            void init();

        private:
            VcdDoc* m_doc;
            VcdProjectModel* m_model;
            QTreeView* m_view;

            QAction* m_actionProperties;
            QAction* m_actionRemove;
    };
}

#endif
