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
		
signals:
	void dropped(const QStringList&, K3bDirItem* );
	
private:
	class K3bPrivateDataDirTree;
	class K3bPrivateDataFileView;
	class K3bPrivateDataDirViewItem;
	class K3bPrivateDataFileViewItem;
	class K3bPrivateDataRootViewItem;
	
	K3bPrivateDataDirTree* m_dataDirTree;
	K3bPrivateDataFileView* m_dataFileView;
	K3bFillStatusDisplay* m_fillStatusDisplay;
		
	KPopupMenu* m_popupMenu;
	KAction* actionRemove;
	KAction* actionRename;
		
	K3bDataDoc* m_doc;
	K3bDataBurnDialog* m_burnDialog;
	
	void setupPopupMenu();
};


class K3bDataView::K3bPrivateDataDirTree : public KListView
{
	friend K3bDataView;
	
	Q_OBJECT

public:
	K3bPrivateDataDirTree( K3bDataDoc*, QWidget* parent );
	~K3bPrivateDataDirTree() {}

	K3bPrivateDataDirViewItem* root() { return m_root; }
		
protected:
	bool acceptDrag(QDropEvent* e) const;
	
private:
	K3bDataDoc* m_doc;
	K3bDataView::K3bPrivateDataDirViewItem* m_root;
	
private slots:
	void slotExecuted( QListViewItem* );
	
signals:
	void dirSelected( K3bDirItem* );
};



class K3bDataView::K3bPrivateDataFileView : public KListView
{
	friend K3bDataView;
	
	Q_OBJECT

public:
	K3bPrivateDataFileView( K3bDataDoc*, QWidget* parent );
	~K3bPrivateDataFileView() {}
	
	K3bDirItem* currentDir() const { return m_currentDir; }
	
	/** reloads the current dir from m_doc **/
	void reload();

public slots:
	void slotSetCurrentDir( K3bDirItem* dir );
			
protected:
	bool acceptDrag(QDropEvent* e) const;

private:
	K3bDataDoc* m_doc;
	K3bDirItem* m_currentDir;
};



class K3bDataView::K3bPrivateDataDirViewItem : public QListViewItem
{
public:
	K3bPrivateDataDirViewItem( K3bDirItem* dir, QListView* parent );
	K3bPrivateDataDirViewItem( K3bDirItem* dir, QListViewItem* parent );
	~K3bPrivateDataDirViewItem() {}
	
	virtual QString text( int ) const;
	
	/** reimplemented from QListViewItem */
	void setText(int col, const QString& text );

	K3bDirItem* dirItem() const { return m_dirItem; }
	
private:
	K3bDirItem* m_dirItem;
};


class K3bDataView::K3bPrivateDataFileViewItem : public QListViewItem
{
public:
	K3bPrivateDataFileViewItem( K3bFileItem*, QListView* parent );
	K3bPrivateDataFileViewItem( K3bFileItem*, QListViewItem* parent );
	~K3bPrivateDataFileViewItem() {}
	
	QString text( int ) const;

	/** reimplemented from QListViewItem */
	void setText(int col, const QString& text );

	K3bFileItem* fileItem() const { return m_fileItem; }
	
private:
	K3bFileItem* m_fileItem;
};


class K3bDataView::K3bPrivateDataRootViewItem : public K3bPrivateDataDirViewItem
{
public:
	K3bPrivateDataRootViewItem( K3bDataDoc*, QListView* parent );
	~K3bPrivateDataRootViewItem() {}
	
	QString text( int ) const;
		
private:
	K3bDataDoc* m_doc;
};

#endif
