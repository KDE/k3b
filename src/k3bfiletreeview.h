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


#ifndef K3BFILETREEVIEW_H
#define K3BFILETREEVIEW_H


#include <kfiletreeview.h>

class KFileTreeBranch;
class KActionCollection;
class KActionMenu;
class QPoint;
class QDropEvent;
class QDragEnterEvent;

namespace K3bCdDevice {
  class CdDevice;
  class DeviceManager;
}

namespace KIO {
  class Job;
}


class K3bDeviceBranch : public KFileTreeBranch
{
  Q_OBJECT

 public:
  K3bDeviceBranch( KFileTreeView*, K3bCdDevice::CdDevice* dev, KFileTreeViewItem* item = 0 );

  K3bCdDevice::CdDevice* device() const { return m_device; }

 signals:
  /**
   * mountPoint is empty if not successful
   */
  void mountFinished( K3bDeviceBranch*, const QString& mountPoint );
  void unmountFinished( K3bDeviceBranch*, bool success );

 public slots:
  void mount();
  void unmount();

//  bool populate( const KURL& url, KFileTreeViewItem* v );

 private slots:
  void slotMountFinished( KIO::Job* );
  void slotUnmountFinished( KIO::Job* );

 private:
  K3bCdDevice::CdDevice* m_device;
};


class K3bFileTreeBranch : public KFileTreeBranch
{
  Q_OBJECT

 public:
  K3bFileTreeBranch( KFileTreeView*,
		     const KURL& url,
		     const QString& name,
		     const QPixmap& pix,
		     bool showHidden = false,
		     KFileTreeViewItem* item = 0 );
};


class K3bDeviceBranchViewItem : public KFileTreeViewItem
{
 public:
  K3bDeviceBranchViewItem( KFileTreeViewItem*, KFileItem*, KFileTreeBranch* );
  K3bDeviceBranchViewItem( KFileTreeView *, KFileItem*, KFileTreeBranch* );

  QString key( int column, bool ascending ) const;
};


class K3bFileTreeViewItem : public KFileTreeViewItem
{
 public:
  K3bFileTreeViewItem( KFileTreeViewItem*, KFileItem*, KFileTreeBranch* );
  K3bFileTreeViewItem( KFileTreeView *, KFileItem*, KFileTreeBranch* );

  QString key( int column, bool ascending ) const;
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

  K3bDeviceBranch* branch( K3bCdDevice::CdDevice* dev );

  /**
   * returns 0 if no device is selected 
   */
  K3bCdDevice::CdDevice* selectedDevice() const;

  /** 
   * returnes an empty url if no url is selected
   */
  KURL selectedUrl() const;
  void setSelectedDevice(K3bCdDevice::CdDevice* dev);

 public slots:
  /**
   * adds home and root dir branch
   */
  void addDefaultBranches();
  void addCdDeviceBranches( K3bCdDevice::DeviceManager* );

  void followUrl( const KURL& url );
  void setTreeDirOnlyMode( bool b );
  void enablePopupMenu( bool b ) { m_menuEnabled = b; }

  /**
   * @reimplemented
   */
  virtual void clear();

 protected:
  virtual void contentsDropEvent(QDropEvent* event);
  void contentsDragMoveEvent ( QDragMoveEvent *e );
  virtual bool acceptDrag(QDropEvent* event) const;

 signals:
  void urlExecuted( const KURL& url );
  void deviceExecuted( K3bCdDevice::CdDevice* dev );

  /** only gets emitted if the menu is disabled */
  void contextMenu( K3bCdDevice::CdDevice*, const QPoint& );
  /** only gets emitted if the menu is disabled */
  void contextMenu( const KURL& url, const QPoint& );

  void mountFinished( K3bDeviceBranch*, const QString& mountPoint );
  void unmountFinished( K3bDeviceBranch*, bool success );
  
 private slots:
  void slotItemExecuted( QListViewItem* item );
  void slotContextMenu( KListView*, QListViewItem*, const QPoint& );

 private:
  void initActions();

  class Private;
  Private* d;

  bool m_dirOnlyMode;
  KActionCollection* m_actionCollection;
  KActionMenu* m_devicePopupMenu;
  KActionMenu* m_urlPopupMenu;
  bool m_menuEnabled;
};

#endif
