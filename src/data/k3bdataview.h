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

#include <klistview.h>


class K3bDataDoc;
class K3bDataItem;
class K3bFileItem;
class K3bDirItem;
class K3bFillStatusDisplay;
class KListView;
class KPopupMenu;
class KAction;
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
	
   K3bProjectBurnDialog* burnDialog();
	
 public slots:
   void slotAddFile( K3bFileItem* );
   void slotAddDir( K3bDirItem* );
   void slotItemRemoved( K3bDataItem* );
	
 protected slots:
   /** generates a dropped signal */
   void slotDropped( KListView*, QDropEvent* e, QListViewItem* after );
   void showPopupMenu( QListViewItem* _item, const QPoint& );
   void slotRenameItem();
   void slotRemoveItem();
   void slotNewDir();
		
 signals:
   void dropped(const QStringList&, K3bDirItem* );
	
 private:
   K3bDataDirTreeView* m_dataDirTree;
   K3bDataFileView* m_dataFileView;
   K3bFillStatusDisplay* m_fillStatusDisplay;
		
   KPopupMenu* m_popupMenu;
   KAction* actionRemove;
   KAction* actionRename;
   KAction* actionNewDir;
		
   K3bDataDoc* m_doc;
   K3bDataBurnDialog* m_burnDialog;
	
   void setupPopupMenu();
};





class K3bDataDirViewItem : public KListViewItem
{
 public:
  K3bDataDirViewItem( K3bDirItem* dir, QListView* parent );
  K3bDataDirViewItem( K3bDirItem* dir, QListViewItem* parent );
  ~K3bDataDirViewItem();
	
  virtual QString text( int ) const;
	
  /** reimplemented from QListViewItem */
  void setText(int col, const QString& text );

  K3bDirItem* dirItem() const { return m_dirItem; }
	
 private:
  K3bDirItem* m_dirItem;
};


class K3bDataFileViewItem : public KListViewItem
{
 public:
  K3bDataFileViewItem( K3bFileItem*, QListView* parent );
  K3bDataFileViewItem( K3bFileItem*, QListViewItem* parent );
  ~K3bDataFileViewItem() {}
	
  QString text( int ) const;

  /** reimplemented from QListViewItem */
  void setText(int col, const QString& text );

  K3bFileItem* fileItem() const { return m_fileItem; }
	
 private:
  K3bFileItem* m_fileItem;
};


class K3bDataRootViewItem : public K3bDataDirViewItem
{
 public:
  K3bDataRootViewItem( K3bDataDoc*, QListView* parent );
  ~K3bDataRootViewItem() {}
	
  QString text( int ) const;
	
  /** reimplemented from QListViewItem */
  void setText(int col, const QString& text );
		
 private:
  K3bDataDoc* m_doc;
};

#endif
