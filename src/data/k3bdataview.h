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


#ifndef K3BDATAVIEW_H
#define K3BDATAVIEW_H

#include <k3bview.h>
#include "k3bdatadirtreeview.h"
#include "k3bdatafileview.h"
#include <device/k3bdevice.h>

class K3bDataDoc;
class K3bDataItem;
class K3bFileItem;
class K3bDirItem;
class K3bFillStatusDisplay;

namespace KIO {
  class Job;
}


/**
  *@author Sebastian Trueg
  */

class K3bDataView : public K3bView
{
   Q_OBJECT

 public:
   K3bDataView(K3bDataDoc* doc, QWidget *parent=0, const char *name=0);
   virtual ~K3bDataView();
	
   virtual void burnDialog( bool );

   K3bDirItem* currentDir() const;

 public slots:
   void importSession();
   void clearImportedSession();
   void editBootImages();

 private slots:
   void slotMountFinished( KIO::Job* job );

 protected:
   K3bDataDirTreeView* m_dataDirTree;
   K3bDataFileView* m_dataFileView;
   K3bFillStatusDisplay* m_fillStatusDisplay;
		
 private:
   K3bDataDoc* m_doc;

   // used for mounting when importing old session
   K3bDevice* m_device;
};


#endif
