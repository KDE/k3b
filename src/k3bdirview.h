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

#include <k3bmedium.h>

class QSplitter;
class KURL;
class K3bAudioCdView;
class K3bVideoCdView;
class K3bFileView;
class K3bVideoDVDRippingView;
class KComboBox;
class K3bFileTreeView;
class QWidgetStack;
class K3bDiskInfoView;
class QScrollView;
class QLabel;
class KConfig;
class K3bDeviceBranch;

namespace K3bDevice {
  class Device;
  class DiskInfo;
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

 public slots:
  void saveConfig( KConfig* c );
  void readConfig( KConfig* c );
  void showUrl( const KURL& );
  void showDevice( K3bDevice::Device* );
  
 protected slots:
  void slotDirActivated( const KURL& );
  void slotDirActivated( const QString& );
  void slotMountFinished( const QString& );
  void slotUnmountFinished( bool );
  void showMediumInfo( const K3bMedium& );
  void slotDetectingDiskInfo( K3bDevice::Device* dev );
  void home();
  void slotFileTreeContextMenu( K3bDevice::Device* dev, const QPoint& p );

 signals:
  void urlEntered( const KURL& );
  void deviceSelected( K3bDevice::Device* );

 private:
  QWidgetStack* m_viewStack;
  QScrollView* m_scroll;

  K3bAudioCdView*   m_cdView;
  K3bVideoCdView*   m_videoView;
  K3bVideoDVDRippingView* m_movieView;
  K3bFileView* m_fileView;
  K3bDiskInfoView* m_infoView;

  KComboBox* m_urlCombo;
  QSplitter* m_mainSplitter;
  K3bFileTreeView* m_fileTreeView;

  bool m_bViewDiskInfo;

  class Private;
  Private* d;
};

#endif
