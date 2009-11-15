/*
*
* Copyright (C) 2003-2004 Christian Kvasny <chris@k3b.org>
*           (C) 2009      Arthur Renato Mello <arthur@mandriva.com>
*           (C) 2009      Michal Malek <michalm@jabster.pl>
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

#include "k3bstandardview.h"

class QWidget;
namespace K3b {
    class VcdDoc;
}
namespace K3b {
    class ProjectBurnDialog;
}

namespace K3b {
    class VcdProjectModel;
}

class KAction;
class KMenu;

namespace K3b {
    class VcdView : public StandardView
    {
        Q_OBJECT

        public:
            VcdView( VcdDoc* pDoc, QWidget* parent );
            ~VcdView();

        private Q_SLOTS:
            /**
            * reimplemented from @ref StandardView
            */
            virtual void selectionChanged( const QModelIndexList& indexes );
            virtual void contextMenu( const QPoint& pos );
            void showPropertiesDialog();

        protected:
            ProjectBurnDialog* newBurnDialog( QWidget* parent = 0 );

            void init();

        private:
            VcdDoc* m_doc;
            K3b::VcdProjectModel* m_model;

            KAction* m_actionProperties;
            KAction* m_actionRemove;
            KMenu* m_popupMenu;
    };
}

#endif
