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


#ifndef K3BDIRVIEW_H
#define K3BDIRVIEW_H

#include <qvbox.h>
#include <qstring.h>


class QSplitter;
class KURL;
class K3bAudioCdView;
class K3bVideoCdView;
class K3bFileView;
class K3bMovieView;
class KComboBox;
class K3bFileTreeView;
class QWidgetStack;
class K3bDiskInfoView;
class KActionCollection;
class KActionMenu;
class QScrollView;
class QLabel;
class KConfig;
class K3bDeviceBranch;

namespace K3bDevice {
  class Device;
  class DiskInfo;
  class DiskInfoDetector;
}

namespace KIO {
  class Job;
}


/**
  *@author Sebastian Trueg
  */
class K3bDirView : public QVBox
{
  Q_OBJECT

 public:
  K3bDirView(K3bFileTreeView* tree, QWidget *parent=0, const char *name=0);
  ~K3bDirView();

  //  K3bCdView* getCdView() { return m_cdView; }

 public slots:
  void saveConfig( KConfig* c );
  void showUrl( const KURL& );
  void showDevice( K3bDevice::Device* );
  void showDiskInfo( K3bDevice::Device* dev );
  
 protected slots:
  void slotDirActivated( const KURL& );
  void slotDirActivated( const QString& );
  void slotMountDevice( K3bDevice::Device* dev );
  void slotMountFinished( K3bDeviceBranch*, const QString& );
  void slotDiskInfoReady( K3bDevice::DiskInfoDetector* );
  void slotDetectDiskInfo( K3bDevice::Device* dev );
  void reload();
  void home();
  void slotShowDiskInfo();
  void slotUnlockDevice();
  void slotLockDevice();
  void slotUnmountDisk();
  void slotUnmountFinished( K3bDeviceBranch*, bool );
  void slotEjectDisk();
  void slotLoadDisk();
  void slotSetReadSpeed();
  //  void slotEjectFinished();
  void slotFileTreeContextMenu( K3bDevice::Device* dev, const QPoint& p );

 signals:
  void urlEntered( const KURL& );

 private:
  QWidgetStack* m_viewStack;
  QScrollView* m_scroll;

  K3bAudioCdView*   m_cdView;
  K3bVideoCdView*   m_videoView;
  K3bMovieView* m_movieView;
  K3bFileView* m_fileView;
  K3bDiskInfoView* m_infoView;

  // these are used to display a message while retrieving disk information
  class NoViewView;
  NoViewView* m_noViewView;
  QLabel* m_noViewLabel;

  KComboBox* m_urlCombo;
  QSplitter* m_mainSplitter;
  K3bFileTreeView* m_fileTreeView;

  KActionCollection* m_actionCollection;

  K3bDevice::DiskInfoDetector* m_diskInfoDetector;
  bool m_bViewDiskInfo;
  KActionMenu* m_devicePopupMenu;
  K3bDevice::Device* m_lastDevice;

  class Private;
  Private* d;
};

#endif
