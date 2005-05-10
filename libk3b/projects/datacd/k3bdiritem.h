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


#ifndef K3BDIRITEM_H
#define K3BDIRITEM_H


#include <qstring.h>
#include <qptrlist.h>

#include <kio/global.h>

#include "k3bdataitem.h"

class K3bDataDoc;

/**
 *@author Sebastian Trueg
 */

class K3bDirItem : public K3bDataItem
{
 public: 
  K3bDirItem( const QString& name, K3bDataDoc*, K3bDirItem* parentDir = 0 );
  virtual ~K3bDirItem();
	
  K3bDirItem* getDirItem() const;

  const QPtrList<K3bDataItem>& children() const { return m_children; }
  K3bDirItem* addDataItem( K3bDataItem* item );
  K3bDataItem* takeDataItem( K3bDataItem* item );
	
  K3bDataItem* nextSibling() const;
  K3bDataItem* nextChild( K3bDataItem* ) const;

  bool alreadyInDirectory( const QString& fileName ) const;
  K3bDataItem* find( const QString& filename ) const;
  K3bDataItem* findByPath( const QString& );

  KIO::filesize_t size() const;

  long numFiles() const;
  long numDirs() const;

  /**
   * returns true if item is a subItem of 
   * this dir item 
   * (returns also true if item == this
   */
  bool isSubItem( K3bDataItem* item ) const;

  bool isDir() const { return true; }

  virtual bool isRemoveable() const;

  /**
   * \return true if some child is from an old session.
   */
  virtual bool isFromOldSession() const;

 private:
  /**
   * this recursivly updates the size of the directories.
   * The size of this dir and the parent dir is updated.
   * These values are just used for user information.
   */
  void updateSize( KIO::filesize_t s );
  /**
   * Updates the number of files and directories. These values are
   * just used for user information.
   */
  void updateFiles( long files, long dirs );

  mutable QPtrList<K3bDataItem> m_children;

  KIO::filesize_t m_size;
  long m_files;
  long m_dirs;
};


class K3bRootItem : public K3bDirItem
{
 public:
  K3bRootItem( K3bDataDoc* );
  ~K3bRootItem();

  const QString& k3bName();
  void setK3bName( const QString& );

  bool isMoveable() const { return false; }
  bool isRemoveable() const { return false; }
};
#endif
