/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef _K3B_AUDIOCD_VIEW_H_
#define _K3B_AUDIOCD_VIEW_H_

#include <k3bcdcontentsview.h>

#include <k3btoc.h>
#include <cddb/k3bcddbresult.h>
#include <k3bcdtext.h>

class K3bListView;
class KListView;
class QListViewItem;
class QPoint;
class KActionCollection;
class KActionMenu;
class K3bCddb;
class QLabel;
class K3bToolBox;

namespace K3bCdDevice {
  class DiskInfoDetector;
  class CdDevice;
}


class K3bAudioCdView : public K3bCdContentsView
{
  Q_OBJECT

 public:
  K3bAudioCdView( QWidget* parent = 0, const char * name = 0 );
  ~K3bAudioCdView();

  void setDisk( K3bCdDevice::DiskInfoDetector* );

  //  const K3bCdDevice::DiskInfo& displayedDisk() const { return m_diskInfo; }

  KActionCollection* actionCollection() const { return m_actionCollection; }

 public slots:
  void reload();
  void queryCddb();

 private slots:
  void slotContextMenu( KListView*, QListViewItem*, const QPoint& );
  void slotItemRenamed( QListViewItem*, const QString&, int );
  void slotCddbQueryFinished( int );
  void slotTrackSelectionChanged( QListViewItem* );
  void slotSaveCddbLocally();

  void slotEditTrackCddb();
  void slotEditAlbumCddb();
  void startRip();
  void slotSelectAll();
  void slotDeselectAll();
  void slotSelect();
  void slotDeselect();

 private:
  void initActions();
  void updateDisplay();
  void enableInteraction( bool );

  K3bCdDevice::Toc m_toc;
  K3bCdDevice::CdDevice* m_device;

  K3bCddbResultEntry m_cddbInfo;

  KActionCollection* m_actionCollection;
  KActionMenu* m_popupMenu;

  K3bListView* m_trackView;
  K3bToolBox* m_toolBox;
  QLabel* m_labelLength;

  class AudioTrackViewItem;

  K3bCddb* m_cddb;

  K3bCdDevice::CdText m_cdText;
};


#endif
