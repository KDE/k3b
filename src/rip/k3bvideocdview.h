/*
    SPDX-FileCopyrightText: 2003 Christian Kvasny <chris@k3b.org>
    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/


#ifndef _K3B_VIDEOCDVIEW_H_
#define _K3B_VIDEOCDVIEW_H_

#include "k3bmediacontentsview.h"

class KActionCollection;
class QTreeWidgetItem;
class QDomElement;

namespace K3b {
class VideoCdView : public MediaContentsView
{
        Q_OBJECT

    public:
        explicit VideoCdView( QWidget* parent = 0 );
        ~VideoCdView() override;

        KActionCollection* actionCollection() const;

    private Q_SLOTS:
        void slotContextMenu( const QPoint& pos );
        void slotTrackSelectionChanged( QTreeWidgetItem* current, QTreeWidgetItem* previous );
        void slotStateChanged( QTreeWidgetItem* item, int column );
        void slotVideoCdInfoFinished( bool );

        void startRip();
        void slotCheckAll();
        void slotUncheckAll();
        void slotCheck();
        void slotUncheck();
        void slotViewFiles();

    private:
        void reloadMedium() override;
        void initActions();
        void updateDisplay();
        void enableInteraction( bool ) override;
        void buildTree( QTreeWidgetItem* parentItem, const QDomElement& parentElement, const QString& pname = QString() );
        
        class Private;
        Private* d;
};
}

#endif
