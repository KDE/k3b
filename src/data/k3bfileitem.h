/***************************************************************************
                          k3bfileitem.h  -  description
                             -------------------
    begin                : Sat Apr 21 2001
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
  QString localPath();
	
  KIO::filesize_t k3bSize() const;

  K3bDirItem* getDirItem() const;
	
  bool isSymLink() const { return isLink(); }

  /** returns true if the item is not a link or 
   *  if the link's destination is part of the compilation */
  bool isValid() const;

 private:
  KIO::filesize_t m_size;
};

#endif
