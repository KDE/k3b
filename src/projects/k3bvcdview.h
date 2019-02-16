/*
 *
 * Copyright (C) 2003-2004 Christian Kvasny <chris@k3b.org>
 * Copyright (C) 2009      Arthur Renato Mello <arthur@mandriva.com>
 * Copyright (C) 2009-2010 Michal Malek <michalm@jabster.pl>
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
