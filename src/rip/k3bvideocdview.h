/*
*
* Copyright (C) 2003 Christian Kvasny <chris@k3b.org>
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

class K3ListView;
class KActionCollection;
class Q3ListViewItem;
class QDomElement;

namespace K3b {
class VideoCdView : public MediaContentsView
{
        Q_OBJECT

    public:
        VideoCdView( QWidget* parent = 0 );
        ~VideoCdView();

        KActionCollection* actionCollection() const;

    private Q_SLOTS:
        void slotContextMenu( K3ListView*, Q3ListViewItem*, const QPoint& );
        void slotTrackSelectionChanged( Q3ListViewItem* );
        void slotStateChanged( Q3ListViewItem* );
        void slotVideoCdInfoFinished( bool );

        void startRip();
        void slotCheckAll();
        void slotUncheckAll();
        void slotCheck();
        void slotUncheck();
        void slotViewFiles();

    private:
        void reloadMedium();
        void initActions();
        void updateDisplay();
        void enableInteraction( bool );
        void buildTree( Q3ListViewItem *parentItem, const QDomElement &parentElement, const QString& pname = QString() );
        
        class Private;
        Private* d;
};
}

#endif
