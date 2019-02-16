/*
*
* Copyright (C) 2003 Christian Kvasny <chris@k3b.org>
* Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
*
* This file is part of the K3b project.
* Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
* See the file "COPYING" for the exact licensing terms.
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
