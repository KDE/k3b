/***************************************************************************
                          k3bdatadirview.h  -  description
                             -------------------
    begin                : Sat Oct 20 2001
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

#ifndef K3BDATADIRTREEVIEW_H
#define K3BDATADIRTREEVIEW_H


#include <klistview.h>
#include <kurl.h>

#include <qmap.h>

class K3bDataView;
class K3bDataDoc;
class K3bDataDirViewItem;
class K3bDirItem;
class K3bDataItem;
class KListViewLineEdit;
class K3bDataFileView;
class KActionCollection;
class KActionMenu;
class KAction;
class K3bView;


/**
  *@author Sebastian Trueg
  */

class K3bDataDirTreeView : public KListView  
{
  Q_OBJECT

 public:
  K3bDataDirTreeView( K3bView*, K3bDataDoc*, QWidget* parent );
  ~K3bDataDirTreeView();

  K3bDataDirViewItem* root() { return m_root; }
		
  void setFileView( K3bDataFileView* view ) { m_fileView = view; }

  KActionCollection* actionCollection() const { return m_actionCollection; }

 signals:
  void urlsDropped( const KURL::List&, QListViewItem* parent );

 protected:
  bool acceptDrag(QDropEvent* e) const;
	
 private:
  void setupActions();

  KActionCollection* m_actionCollection;
  KActionMenu* m_popupMenu;
  KAction* m_actionRemove;
  KAction* m_actionRename;
  KAction* m_actionNewDir;
  KAction* m_actionProperties;

  K3bView* m_view;

  K3bDataDoc* m_doc;
  K3bDataDirViewItem* m_root;
  K3bDataFileView* m_fileView;
  KListViewLineEdit* m_editor;

  /**
   * We save the dirItems in a map to have a fast way
   * for checking for new or removed items
   */
  QMap<K3bDirItem*, K3bDataDirViewItem*> m_itemMap;

 public slots:
  void updateContents();
  void setCurrentDir( K3bDirItem* );
  void rename( QListViewItem* item, int	col );

 private slots:
  void slotExecuted( QListViewItem* );
  void slotDataItemRemoved( K3bDataItem* );
  void slotDropped( QDropEvent* e, QListViewItem* after, QListViewItem* parent );
  void showPopupMenu( QListViewItem* _item, const QPoint& );
  void slotRenameItem();
  void slotRemoveItem();
  void slotNewDir();
  void slotProperties();

 signals:
  void dirSelected( K3bDirItem* );
};

#endif
