/***************************************************************************
                          k3bdatafileview.h  -  description
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

#ifndef K3BDATAFILEVIEW_H
#define K3BDATAFILEVIEW_H

#include "../tools/k3blistview.h"

#include "../klistviewlineedit.h"

class K3bDataDoc;
class K3bDirItem;
class K3bDataView;
class K3bDataItem;
class QDropEvent;
class KActionCollection;
class KActionMenu;
class KAction;
class K3bDataDirTreeView;
class K3bView;
class QPainter;

/**
  *@author Sebastian Trueg
  */

class K3bDataFileView : public K3bListView
{
  Q_OBJECT

 public:
  K3bDataFileView( K3bView*, K3bDataDirTreeView*, K3bDataDoc*, QWidget* parent );
  ~K3bDataFileView();

  K3bDirItem* currentDir() const { return m_currentDir; }

  KActionCollection* actionCollection() const { return m_actionCollection; }

 signals:
  void dirSelected( K3bDirItem* );

 public slots:
  void slotSetCurrentDir( K3bDirItem* );
  void updateContents();

 private slots:
  void slotDataItemRemoved( K3bDataItem* );
  void slotExecuted( QListViewItem* );
  void slotDropped( QDropEvent* e, QListViewItem* after, QListViewItem* parent );
  void showPopupMenu( KListView*, QListViewItem* _item, const QPoint& );
  void slotRenameItem();
  void slotRemoveItem();
  void slotNewDir();
  void slotParentDir();
  void slotProperties();

 protected:
  bool acceptDrag(QDropEvent* e) const;

 private:
  void setupActions();

  KActionCollection* m_actionCollection;
  KActionMenu* m_popupMenu;
  KAction* m_actionParentDir;
  KAction* m_actionRemove;
  KAction* m_actionRename;
  KAction* m_actionNewDir;
  KAction* m_actionProperties;

  K3bView* m_view;

  K3bDataDoc* m_doc;
  K3bDirItem* m_currentDir;
  K3bDataDirTreeView* m_treeView;
  KListViewLineEdit* m_editor;
};

#endif
