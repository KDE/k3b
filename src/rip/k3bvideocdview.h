/*
 *
 * $Id$
 * Copyright (C) 2003 Christian Kvasny <chris@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef _K3B_VIDEOCDVIEW_H_
#define _K3B_VIDEOCDVIEW_H_

#include <k3bcdcontentsview.h>

#include <device/k3bdiskinfo.h>
#include "k3bvideocdinfo.h"

class KActionCollection;
class KActionMenu;
class KListView;

class QLabel;
class QListViewItem;

class K3bListView;
class K3bToolBox;

namespace K3bCdDevice {
  class DiskInfoDetector;
}


class K3bVideoCdView : public K3bCdContentsView
{
  Q_OBJECT

 public:
  K3bVideoCdView( QWidget* parent = 0, const char * name = 0 );
  ~K3bVideoCdView();

  void setDisk( K3bCdDevice::DiskInfoDetector* );

  const K3bCdDevice::DiskInfo& displayedDisk() const { return m_diskInfo; }

  KActionCollection* actionCollection() const { return m_actionCollection; }

 private slots:
  void slotContextMenu( KListView*, QListViewItem*, const QPoint& );
  void slotTrackSelectionChanged( QListViewItem* );
  void slotVideoCdInfoFinished( bool );

  void startRip();
  void slotSelectAll();
  void slotDeselectAll();
  void slotSelect();
  void slotDeselect();

 private:

  class VideoTrackViewCheckItem;
  class VideoTrackViewItem;

  void initActions();
  void updateDisplay();
  void enableInteraction( bool );


  K3bCdDevice::DiskInfo m_diskInfo;

  KActionCollection* m_actionCollection;
  KActionMenu* m_popupMenu;

  K3bVideoCdInfoResult m_videocdinfoResult;
  K3bVideoCdInfo* m_videocdinfo;

  K3bListView* m_trackView;
  K3bToolBox* m_toolBox;
  QLabel* m_labelLength;

  QValueList<VideoTrackViewCheckItem *> m_contentList;

  long m_videocdsize;

};

#endif
