/*
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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


#ifndef K3BDIRVIEW_H
#define K3BDIRVIEW_H


//Added by qt3to4:
#include <QLabel>

#include <k3bmedium.h>
#include <kvbox.h>

class QSplitter;
class KUrl;
class K3bAudioCdView;
class K3bVideoCdView;
class K3bFileView;
class K3bVideoDVDRippingView;
class KComboBox;
class K3bFileTreeView;
class QStackedWidget;
class K3bDiskInfoView;
class Q3ScrollView;
class QLabel;
class KConfig;
class KConfigGroup;
namespace K3bDevice {
  class Device;
  class DiskInfo;
}


/**
  *@author Sebastian Trueg
  */
class K3bDirView : public KVBox
{
  Q_OBJECT

 public:
  K3bDirView(K3bFileTreeView* tree, QWidget *parent=0);
  ~K3bDirView();

 public slots:
  void saveConfig( KConfigGroup&grp );
  void readConfig( const KConfigGroup & grp );
  void showUrl( const KUrl& );
  void showDevice( K3bDevice::Device* );
  
 protected slots:
  void slotDirActivated( const KUrl& );
  void slotDirActivated( const QString& );
  void slotMountFinished( const QString& );
  void slotUnmountFinished( bool );
  void showMediumInfo( const K3bMedium& );
  void slotDetectingDiskInfo( K3bDevice::Device* dev );
  void home();
  void slotFileTreeContextMenu( K3bDevice::Device* dev, const QPoint& p );

 signals:
  void urlEntered( const KUrl& );
  void deviceSelected( K3bDevice::Device* );

 private:
  QStackedWidget* m_viewStack;
  Q3ScrollView* m_scroll;

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
