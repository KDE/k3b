/***************************************************************************
                          k3bdataview.h  -  description
                             -------------------
    begin                : Thu May 10 2001
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

#ifndef K3BDATAVIEW_H
#define K3BDATAVIEW_H

#include "../k3bview.h"
#include "k3bdatadirtreeview.h"
#include "k3bdatafileview.h"



class K3bDataDoc;
class K3bDataItem;
class K3bFileItem;
class K3bDirItem;
class K3bFillStatusDisplay;
class KListView;
class K3bProjectBurnDialog;
class K3bDataBurnDialog;

/**
  *@author Sebastian Trueg
  */

class K3bDataView : public K3bView
{
   Q_OBJECT

 public:
   K3bDataView(K3bDataDoc* doc, QWidget *parent=0, const char *name=0);
   ~K3bDataView();
	
   void burnDialog( bool );

   K3bDirItem* currentDir() const;

 private:
   K3bDataDirTreeView* m_dataDirTree;
   K3bDataFileView* m_dataFileView;
   K3bFillStatusDisplay* m_fillStatusDisplay;
		
   K3bDataDoc* m_doc;
   K3bDataBurnDialog* m_burnDialog;
};


#endif
