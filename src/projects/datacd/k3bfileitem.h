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


#ifndef K3BFILEITEM_H
#define K3BFILEITEM_H


#include "k3bdataitem.h"

#include <kfileitem.h>
#include <kio/global.h>
#include <qstring.h>

class K3bDataDoc;
class K3bDirItem;

/**
  *@author Sebastian Trueg
  */


class K3bFileItem : public KFileItem, public K3bDataItem
{
public:
  /**
   * Creates a new K3bFileItem
   */
  K3bFileItem( const QString& fileName, K3bDataDoc* doc, K3bDirItem* dir, const QString& k3bName = 0 );
  virtual ~K3bFileItem();
	
  bool exists() const;
	
  QString absIsoPath();

  /** reimplemented from K3bDataItem */
  QString localPath() const;
  int localInode() const { return m_inode; }

  KIO::filesize_t k3bSize() const;

  K3bDirItem* getDirItem();
	
  bool isSymLink() const;
  bool isFile() const { return true; }

  /** returns true if the item is not a link or 
   *  if the link's destination is part of the compilation */
  bool isValid() const;

  K3bDataItem* replaceItemFromOldSession() const { return m_replacedItemFromOldSession; }
  void setReplacedItemFromOldSession( K3bDataItem* item ) { m_replacedItemFromOldSession = item; }

 private:
  K3bDataItem* m_replacedItemFromOldSession;

  KIO::filesize_t m_size;
  int m_inode;
};

#endif
