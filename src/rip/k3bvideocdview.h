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

#include <qdom.h>
//Added by qt3to4:
#include <QLabel>
#include <Q3ValueList>

#include <k3bmediacontentsview.h>
#include <k3bmedium.h>

#include "k3bvideocdinfo.h"

class KActionCollection;
class KActionMenu;
class K3ListView;

class QLabel;
class Q3ListViewItem;
class KToolBar;
class K3bListView;
class K3bVideoCdRippingOptions;

namespace K3bDevice
{
  class DiskInfoDetector;
  class Toc;
  class Device;
}


class K3bVideoCdView : public K3bMediaContentsView
{
        Q_OBJECT

    public:
        K3bVideoCdView( QWidget* parent = 0, const char * name = 0 );
        ~K3bVideoCdView();

        KActionCollection* actionCollection() const
        {
            return m_actionCollection;
        }

    private slots:
        void slotContextMenu( K3ListView*, Q3ListViewItem*, const QPoint& );
        void slotTrackSelectionChanged( Q3ListViewItem* );
        void slotStateChanged( Q3ListViewItem* );
        void slotVideoCdInfoFinished( bool );

        void startRip();
        void slotSelectAll();
        void slotDeselectAll();
        void slotSelect();
        void slotDeselect();

    private:

        class VideoTrackViewCheckItem;
        class VideoTrackViewItem;

	void reloadMedium();

        void initActions();
        void updateDisplay();
        void enableInteraction( bool );
        void buildTree( Q3ListViewItem *parentItem, const QDomElement &parentElement, const QString& pname = QString::null );

        K3bDevice::Toc m_toc;

        KActionCollection* m_actionCollection;
        KActionMenu* m_popupMenu;

        K3bVideoCdInfoResult m_videocdinfoResult;
        K3bVideoCdInfo* m_videocdinfo;
        K3bVideoCdRippingOptions* m_videooptions;

        K3bListView* m_trackView;
        KToolBar* m_toolBox;
        QLabel* m_labelLength;

        QDomDocument domTree;

        Q3ValueList<VideoTrackViewCheckItem *> m_contentList;

        unsigned long m_videocddatasize;
        unsigned long m_videocdmpegsize;
        
};

#endif
