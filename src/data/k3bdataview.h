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
class K3bFileItem;
class K3bDirItem;
class K3bFillStatusDisplay;
class KListView;

/**
  *@author Sebastian Trueg
  */

class K3bDataView : public K3bView
{
   Q_OBJECT

public:
	K3bDataView(K3bDataDoc* doc, QWidget *parent=0, const char *name=0);
	~K3bDataView();
	
public slots:
	void slotAddFile( K3bFileItem* );
	void slotAddDir( K3bDirItem* );
	
protected slots:
	/** generates a dropped signal */
	void slotDropped( KListView*, QDropEvent* e, QListViewItem* after );
	
signals:
	void dropped(const QStringList&, K3bDirItem* );
	
private:
	class K3bPrivateDataDirTree;
	class K3bPrivateDataFileView;
	class K3bPrivateDataDirViewItem;
	class K3bPrivateDataFileViewItem;
	
	K3bPrivateDataDirTree* m_dataDirTree;
	K3bPrivateDataFileView* m_dataFileView;
	K3bFillStatusDisplay* m_fillStatusDisplay;
		
	K3bDataDoc* m_doc;
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
	
	QString text( int ) const;

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

	K3bFileItem* fileItem() const { return m_fileItem; }
	
private:
	K3bFileItem* m_fileItem;
};

#endif
