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

#include "../kdelibs_patched/klistview.h"


class K3bDataDoc;
class K3bDirItem;
class K3bDataView;
class K3bDataItem;
class QDropEvent;


/**
  *@author Sebastian Trueg
  */

class K3bDataFileView : public KListView  
{
  Q_OBJECT

 public:
  K3bDataFileView( K3bDataView*, K3bDataDoc*, QWidget* parent );
  ~K3bDataFileView() {}
	
  K3bDirItem* currentDir() const { return m_currentDir; }

 signals:
  void dirSelected( K3bDirItem* );
	
 public slots:
  void slotSetCurrentDir( K3bDirItem* );
  void updateContents();
	
 private slots:
  void slotDataItemRemoved( K3bDataItem* );
  void slotExecuted( QListViewItem* );

 protected:
  bool acceptDrag(QDropEvent* e) const;

 private:
  K3bDataDoc* m_doc;
  K3bDirItem* m_currentDir;
  K3bDataView* m_view;
};

#endif
