/***************************************************************************
                          k3bdiritem.h  -  description
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

#ifndef K3BDIRITEM_H
#define K3BDIRITEM_H


#include <qstring.h>
#include <qptrlist.h>

#include "k3bdataitem.h"

class K3bDataDoc;

/**
 *@author Sebastian Trueg
 */

class K3bDirItem : public K3bDataItem
{
 public: 
  K3bDirItem( const QString& name, K3bDataDoc*, K3bDirItem* parentDir = 0 );
  ~K3bDirItem();
	
  QPtrList<K3bDataItem>* children() const { return m_children; }
  K3bDirItem* addDataItem( K3bDataItem* item );
  K3bDataItem* takeDataItem( K3bDataItem* item );
  K3bDataItem* takeDataItem( int index );
  /** reimplemented from K3bDataItem */
  virtual QString k3bPath();
  virtual QString jolietPath();
	
  K3bDataItem* nextSibling();
  K3bDataItem* nextChild( K3bDataItem* );
  /** returns an empty dummy directory */
  QString localPath();

  bool alreadyInDirectory( const QString& fileName ) const;
  K3bDataItem* find( const QString& filename ) const;

  long k3bSize() const;

  int numFiles() const;
  int numDirs() const;

  /**
   * returns true if item is a subItem of 
   * this dir item 
   * (returns also true if item == this
   */
  bool isSubItem( K3bDataItem* item ) const;

  bool isDir() const { return true; }

  virtual bool isRemoveable() const;
	
 private:
  QPtrList<K3bDataItem>* m_children;
};


class K3bRootItem : public K3bDirItem
{
 public:
  K3bRootItem( K3bDataDoc* );
  ~K3bRootItem();

  /** reimplemented from K3bDataItem */
  QString k3bPath();
  QString jolietPath();

  const QString& k3bName();
  void setK3bName( const QString& );

  bool isMoveable() const { return false; }
  bool isRemoveable() const { return false; }
};
#endif
