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

//#include <klistview.h>
#include "../kdelibs_patched/klistview.h"    // patched version with the ability to set invalid chars and 
                                             // set certain KListViewItems unrenamable


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
  friend K3bDataView;
	
  Q_OBJECT

 public:
  K3bDataFileView( K3bDataDoc*, QWidget* parent );
  ~K3bDataFileView() {}
	
  K3bDirItem* currentDir() const { return m_currentDir; }
	
 public slots:
  void slotSetCurrentDir( K3bDirItem* );
  void updateContents();
	
 private slots:
  void slotDataItemRemoved( K3bDataItem* );

 protected:
  bool acceptDrag(QDropEvent* e) const;

 private:
  K3bDataDoc* m_doc;
  K3bDirItem* m_currentDir;
};

#endif
