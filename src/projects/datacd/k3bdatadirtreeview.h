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


#ifndef K3BDATADIRTREEVIEW_H
#define K3BDATADIRTREEVIEW_H


#include <k3blistview.h>
#include <kurl.h>

#include <qmap.h>

class K3bDataView;
class K3bDataDoc;
class K3bDataDirViewItem;
class K3bDirItem;
class K3bDataItem;
class K3bDataFileView;
class KActionCollection;
class KActionMenu;
class KAction;
class K3bView;


/**
  *@author Sebastian Trueg
  */

class K3bDataDirTreeView : public K3bListView  
{
  Q_OBJECT

 public:
  K3bDataDirTreeView( K3bView*, K3bDataDoc*, QWidget* parent );
  virtual ~K3bDataDirTreeView();

  K3bDataDirViewItem* root() { return m_root; }
		
  void setFileView( K3bDataFileView* view ) { m_fileView = view; }

  KActionCollection* actionCollection() const { return m_actionCollection; }

 public slots:
  void updateContents();
  void setCurrentDir( K3bDirItem* );

 signals:
  //  void urlsDropped( const KURL::List&, QListViewItem* parent );
  void dirSelected( K3bDirItem* );

 protected:
  bool acceptDrag(QDropEvent* e) const;

  KActionCollection* m_actionCollection;
  KActionMenu* m_popupMenu;
  KAction* m_actionRemove;
  KAction* m_actionRename;
  KAction* m_actionNewDir;
  KAction* m_actionProperties;

 protected slots:
  virtual void slotDropped( QDropEvent* e, QListViewItem* after, QListViewItem* parent );

 private:
  void setupActions();
  void startDropAnimation( K3bDirItem* );
  void stopDropAnimation();

  K3bView* m_view;

  K3bDataDoc* m_doc;
  K3bDataDirViewItem* m_root;
  K3bDataFileView* m_fileView;

  /**
   * We save the dirItems in a map to have a fast way
   * for checking for new or removed items
   */
  QMap<K3bDirItem*, K3bDataDirViewItem*> m_itemMap;

  class Private;
  Private* d;

 private slots:
  void slotExecuted( QListViewItem* );
  void slotDataItemRemoved( K3bDataItem* );
  void showPopupMenu( KListView*, QListViewItem* _item, const QPoint& );
  void slotRenameItem();
  void slotRemoveItem();
  void slotNewDir();
  void slotProperties();
  void slotDropAnimate();
};

#endif
