/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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


#ifndef K3BFILETREEVIEW_H
#define K3BFILETREEVIEW_H


#include <kfiletreeview.h>
#include "device/k3bdevice.h"
#include "device/k3bdevicemanager.h"

#include <qmap.h>

class KFileTreeBranch;
class KActionCollection;
class KActionMenu;
class QPoint;
class QDropEvent;
class QDragEnterEvent;


class K3bDeviceBranch : public KFileTreeBranch
{
  Q_OBJECT

 public:
  K3bDeviceBranch( KFileTreeView*, K3bDevice* dev, KFileTreeViewItem* item = 0 );

  K3bDevice* device() const { return m_device; }

// public slots:
//  bool populate( const KURL& url, KFileTreeViewItem* v );

 private:
  K3bDevice* m_device;
};



/**
  *@author Sebastian Trueg
  */
class K3bFileTreeView : public KFileTreeView
{
  Q_OBJECT

 public: 
  K3bFileTreeView( QWidget *parent = 0, const char *name = 0 );
  ~K3bFileTreeView();


  virtual KFileTreeBranch* addBranch( KFileTreeBranch* );
  virtual KFileTreeBranch* addBranch( const KURL& url, const QString& name, const QPixmap& , bool showHidden = false );

  /**
   * adds home and root dir branch
   */
  void addDefaultBranches();
  void addCdDeviceBranches( K3bDeviceManager* );

  /** returns 0 if no device is selected */
  K3bDevice* selectedDevice() const;
  /** returnes an empty url if no url is selected */
  KURL selectedUrl() const;
  void setSelectedDevice(K3bDevice* dev);
 public slots:
  void followUrl( const KURL& url );
  void setTreeDirOnlyMode( bool b );
  void enablePopupMenu( bool b ) { m_menuEnabled = b; }

 protected:
  virtual void contentsDropEvent(QDropEvent* event);
  void contentsDragMoveEvent ( QDragMoveEvent *e );
  virtual bool acceptDrag(QDropEvent* event) const;

 signals:
  void urlExecuted( const KURL& url );
  void deviceExecuted( K3bDevice* dev );

  /** only gets emitted if the menu is disabled */
  void contextMenu( K3bDevice*, const QPoint& );
  /** only gets emitted if the menu is disabled */
  void contextMenu( const KURL& url, const QPoint& );
  
 private slots:
  void slotItemExecuted( QListViewItem* item );
  void slotContextMenu( KListView*, QListViewItem*, const QPoint& );

 private:
  void initActions();

  bool m_dirOnlyMode;
  QMap<KFileTreeBranch*, K3bDevice*> m_deviceBranchesMap;
  KActionCollection* m_actionCollection;
  KActionMenu* m_devicePopupMenu;
  KActionMenu* m_urlPopupMenu;
  bool m_menuEnabled;
};

#endif
