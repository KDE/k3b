/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef K3BDATAVIEWITEM_H
#define K3BDATAVIEWITEM_H

#include <k3blistview.h>
#include <kfileitem.h>

class K3bDataItem;
class K3bFileItem;
class K3bDirItem;
class K3bDataDoc;
class K3bSpecialDataItem;
class K3bSessionImportItem;

class QPainter;
class QColorGroup;



class K3bDataViewItem : public K3bListViewItem
{
 public:
  K3bDataViewItem( K3bDataItem*, QListView* parent );
  K3bDataViewItem( K3bDataItem*, QListViewItem* parent );
  virtual ~K3bDataViewItem();
	
  virtual K3bDataItem* dataItem() const { return m_dataItem; }

  void setText( int col, const QString& text );

  /**
   * reimplemented to have directories always sorted before files
   */
  QString key( int, bool ) const;

  virtual void paintCell( QPainter* p, const QColorGroup& cg, int column, int width, int align );

 private:
  void init();

  K3bDataItem* m_dataItem;
};


class K3bDataDirViewItem : public K3bDataViewItem
{
 public:
  K3bDataDirViewItem( K3bDirItem* dir, QListView* parent );
  K3bDataDirViewItem( K3bDirItem* dir, QListViewItem* parent );
  ~K3bDataDirViewItem();
	
  virtual QString text( int ) const;
	
  K3bDirItem* dirItem() const { return m_dirItem; }

 protected:
  virtual void dragEntered();

 private:
  K3bDirItem* m_dirItem;
};


class K3bDataFileViewItem : public K3bDataViewItem, public KFileItem
{
 public:
  K3bDataFileViewItem( K3bFileItem*, QListView* parent );
  K3bDataFileViewItem( K3bFileItem*, QListViewItem* parent );
  ~K3bDataFileViewItem() {}
	
  QString text( int ) const;

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


class K3bSpecialDataViewItem : public K3bDataViewItem
{
 public:
  K3bSpecialDataViewItem( K3bSpecialDataItem*, QListView* );

  QString text( int ) const;
};


class K3bSessionImportViewItem : public K3bDataViewItem
{
 public:
  K3bSessionImportViewItem( K3bSessionImportItem*, QListView* );

  QString text( int ) const;
};

#endif
