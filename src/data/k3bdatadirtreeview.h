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


#include "../kdelibs_patched/klistview.h"


#include <qmap.h>

class K3bDataView;
class K3bDataDoc;
class K3bDataDirViewItem;
class K3bDirItem;
class K3bDataItem;


/**
  *@author Sebastian Trueg
  */

class K3bDataDirTreeView : public KListView  
{
  Q_OBJECT

 public:
  K3bDataDirTreeView( K3bDataView*, K3bDataDoc*, QWidget* parent );
  ~K3bDataDirTreeView() {}

  K3bDataDirViewItem* root() { return m_root; }
		
 protected:
  bool acceptDrag(QDropEvent* e) const;
	
 private:
  K3bDataDoc* m_doc;
  K3bDataDirViewItem* m_root;
  K3bDataView* m_view;

  /**
   * We save the dirItems in a map to have a fast way
   * for checking for new or removed items
   */
  QMap<K3bDirItem*, K3bDataDirViewItem*> m_itemMap;

 public slots:
  void updateContents();
  void setCurrentDir( K3bDirItem* );
	
 private slots:
  void slotExecuted( QListViewItem* );
  void slotDataItemRemoved( K3bDataItem* );
	
 signals:
  void dirSelected( K3bDirItem* );
};

#endif
