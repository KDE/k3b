/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */



#ifndef _K3B_MOVIX_LISTVIEW_H_
#define _K3B_MOVIX_LISTVIEW_H_

#include <k3blistview.h>
#include <kfileitem.h>

#include <qmap.h>
//Added by qt3to4:
#include <QDropEvent>


class K3bMovixDoc;
class K3bMovixFileItem;
class K3bFileItem;


class K3bMovixListViewItem : public K3bListViewItem
{
 public:
  K3bMovixListViewItem( K3bMovixDoc* doc, K3bMovixFileItem*, Q3ListView* parent, Q3ListViewItem* after );
  K3bMovixListViewItem( K3bMovixDoc* doc, K3bMovixFileItem*, Q3ListViewItem* parent );
  ~K3bMovixListViewItem();

  K3bMovixFileItem* fileItem() const { return m_fileItem; }
  K3bMovixDoc* doc() const { return m_doc; }

  virtual bool isMovixFileItem() const { return true; }

 private:
  K3bMovixDoc* m_doc;
  K3bMovixFileItem* m_fileItem;
};


class K3bMovixFileViewItem : public K3bMovixListViewItem, public KFileItem
{
 public:
  K3bMovixFileViewItem( K3bMovixDoc* doc, K3bMovixFileItem*, Q3ListView* parent, Q3ListViewItem* );

  QString text( int ) const;
  void setText(int col, const QString& text );

  /** always sort according to the playlist order */
  QString key( int, bool ) const;
};

class K3bMovixSubTitleViewItem : public K3bMovixListViewItem, public KFileItem
{
 public:
  K3bMovixSubTitleViewItem( K3bMovixDoc*, K3bMovixFileItem* item, K3bMovixListViewItem* parent );
  ~K3bMovixSubTitleViewItem();

  QString text( int ) const;

  bool isMovixFileItem() const { return false; }
};


class K3bMovixListView : public K3bListView
{
  Q_OBJECT

 public:
  K3bMovixListView( K3bMovixDoc* doc, QWidget* parent = 0, const char* name = 0 );
  ~K3bMovixListView();

  Q3DragObject* dragObject();

 protected:
  bool acceptDrag(QDropEvent* e) const;

 private slots:
  void slotNewFileItems();
  void slotFileItemRemoved( K3bMovixFileItem* );
  void slotSubTitleItemRemoved( K3bMovixFileItem* );
  void slotDropped( KListView*, QDropEvent* e, Q3ListViewItem* after );
  void slotChanged();

 private:
  K3bMovixDoc* m_doc;

  QMap<K3bFileItem*, K3bMovixFileViewItem*> m_itemMap;
};

#endif
