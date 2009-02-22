/* 
 *
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

#ifndef K3BDATAVIEWITEM_H
#define K3BDATAVIEWITEM_H

#include <k3blistview.h>
#include <kmimetype.h>
//Added by qt3to4:
#include <QPixmap>

namespace K3b {
    class DataItem;
}
namespace K3b {
    class FileItem;
}
namespace K3b {
    class DirItem;
}
namespace K3b {
    class DataDoc;
}
namespace K3b {
    class SpecialDataItem;
}
namespace K3b {
    class SessionImportItem;
}

class QPainter;
class QColorGroup;



namespace K3b {
class DataViewItem : public ListViewItem
{
 public:
  DataViewItem( DataItem*, Q3ListView* parent );
  DataViewItem( DataItem*, Q3ListViewItem* parent );
  virtual ~DataViewItem();
	
  virtual DataItem* dataItem() const { return m_dataItem; }

  void setText( int col, const QString& text );

  /**
   * reimplemented to have directories always sorted before files
   */
  QString key( int, bool ) const;

  virtual void paintCell( QPainter* p, const QColorGroup& cg, int column, int width, int align );

 private:
  void init();

  DataItem* m_dataItem;
};
}


namespace K3b {
class DataDirViewItem : public DataViewItem
{
 public:
  DataDirViewItem( DirItem* dir, Q3ListView* parent );
  DataDirViewItem( DirItem* dir, Q3ListViewItem* parent );
  ~DataDirViewItem();
	
  virtual QString text( int ) const;
	
  DirItem* dirItem() const { return m_dirItem; }

  void highlightIcon( bool );

 protected:
  virtual void dragEntered();

 private:
  DirItem* m_dirItem;
  QPixmap m_pixmap;
};
}


namespace K3b {
class DataFileViewItem : public DataViewItem
{
 public:
  DataFileViewItem( FileItem*, Q3ListView* parent );
  DataFileViewItem( FileItem*, Q3ListViewItem* parent );
  ~DataFileViewItem() {}
	
  QString text( int ) const;

  FileItem* fileItem() const { return m_fileItem; }

  const KMimeType::Ptr mimeType() const { return m_pMimeType; }

 private:
  void init( FileItem* );

  FileItem* m_fileItem;
  KMimeType::Ptr m_pMimeType;
};
}


namespace K3b {
class DataRootViewItem : public DataDirViewItem
{
 public:
  DataRootViewItem( DataDoc*, Q3ListView* parent );
  ~DataRootViewItem();
	
  QString text( int ) const;
	
  /** reimplemented from QListViewItem */
  void setText(int col, const QString& text );
		
 private:
  DataDoc* m_doc;
};
}


namespace K3b {
class SpecialDataViewItem : public DataViewItem
{
 public:
  SpecialDataViewItem( SpecialDataItem*, Q3ListView* );

  QString text( int ) const;
};
}


namespace K3b {
class SessionImportViewItem : public DataViewItem
{
 public:
  SessionImportViewItem( SessionImportItem*, Q3ListView* );

  QString text( int ) const;
};
}

#endif
