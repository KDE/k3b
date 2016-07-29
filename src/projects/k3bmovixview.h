/*
 *
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
 *           (C) 2009      Arthur Renato Mello <arthur@mandriva.com>
 *           (C) 2009      Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
 *           (C) 2009-2010 Michal Malek <michalm@jabster.pl>
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



#ifndef _K3B_MOVIX_VIEW_H_
#define _K3B_MOVIX_VIEW_H_

#include "k3bview.h"

class KAction;
class QTreeView;

namespace K3b {

    class MovixDoc;
    class MovixProjectModel;

    class MovixView : public View
    {
        Q_OBJECT

    public:
        MovixView( MovixDoc* doc, QWidget* parent = 0 );
        virtual ~MovixView();

    private Q_SLOTS:
        void slotRemoveSubTitleItems();
        void showPropertiesDialog();
        void slotAddSubTitleFile();
        void slotSelectionChanged();
        void slotRemove();

    protected:
        virtual ProjectBurnDialog* newBurnDialog( QWidget* parent = 0 );

    private:
        MovixDoc* m_doc;
        MovixProjectModel* m_model;
        QTreeView* m_view;

        KAction* m_actionProperties;
        KAction* m_actionRemove;
        KAction* m_actionRemoveSubTitle;
        KAction* m_actionAddSubTitle;
    };
}

#endif
