/***************************************************************************
                          k3bdirview.h  -  description
                             -------------------
    begin                : Mon Mar 26 2001
    copyright            : (C) 2001 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef K3BDIRVIEW_H
#define K3BDIRVIEW_H

#include <qvbox.h>
#include <qfile.h>
#include <qstring.h>

#include <klistview.h>
#include <kdiroperator.h>
#include <kfiledetailview.h>

class QSplitter;
class KURL;
class K3bCdView;
class K3bFileView;
class K3bFilmView;
class K3bDeviceManager;
class KComboBox;
class K3bFileTreeView;
class K3bDevice;
class QWidgetStack;
class K3bDiskInfo;
class K3bDiskInfoView;
class KActionCollection;
class K3bDiskInfoDetector;
class KActionMenu;
class QScrollView;

/**
  *@author Sebastian Trueg
  */
class K3bDirView : public QVBox
{
  Q_OBJECT

 public:
  K3bDirView(QWidget *parent=0, const char *name=0);
  ~K3bDirView();
  void setupFinalize( K3bDeviceManager *dm );

 protected slots:
  void slotDirActivated( const KURL& );
  void slotDirActivated( const QString& );
  void slotUpdateURLCombo( const KURL& url );
  void slotMountDevice( K3bDevice* dev );
  void slotDiskInfoReady( const K3bDiskInfo& info );
  void reload();
  void home();
  void slotDetectDiskInfo( K3bDevice* dev );
  void slotShowDiskInfo();
  void slotUnmountDisk();
  void slotEjectDisk();
  void slotFileTreeContextMenu( K3bDevice* dev, const QPoint& p );

 private:
  QWidgetStack* m_viewStack;
  QScrollView* m_scroll;

  K3bCdView*   m_cdView;
  K3bFilmView* m_filmView;
  K3bFileView* m_fileView;
  K3bDiskInfoView* m_infoView;

  KComboBox* m_urlCombo;
  QSplitter* m_mainSplitter;
  K3bFileTreeView* m_fileTreeView;

  KActionCollection* m_actionCollection;

  K3bDiskInfoDetector* m_diskInfoDetector;
  bool m_bViewDiskInfo;
  KActionMenu* m_devicePopupMenu;
  K3bDevice* m_lastDevice;
};

#endif
