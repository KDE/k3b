/***************************************************************************
                          k3bdiritem.cpp  -  description
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

#include "k3bdiritem.h"
#include "k3bdatadoc.h"

#include <qstring.h>
#include <qptrlist.h>

K3bDirItem::K3bDirItem(const QString& name, K3bDataDoc* doc, K3bDirItem* parentDir)
  : K3bDataItem( doc, parentDir )
{
  m_k3bName = name;
  m_children = new QPtrList<K3bDataItem>();
}

K3bDirItem::~K3bDirItem()
{
  // delete all children
  m_children->setAutoDelete( true );
  delete m_children;
}

K3bDirItem* K3bDirItem::addDataItem( K3bDataItem* item )
{
  if( m_children->find( item ) == -1 )
    m_children->append( item );
	
  return this;
}

K3bDataItem* K3bDirItem::takeDataItem( K3bDataItem* item )
{
  int x = m_children->find( item );
  if( x > -1 )
    return m_children->take( x );
  else
    return 0;
}

K3bDataItem* K3bDirItem::takeDataItem( int index )
{
  return m_children->take( index );
}


QString K3bDirItem::k3bPath()
{
  if( !parent() )
    return k3bName() + "/";
  else
    return parent()->k3bPath() + k3bName() + "/";
}


K3bDataItem* K3bDirItem::nextSibling()
{
  if( !m_children->isEmpty() )
    return m_children->getFirst();
  else
    return K3bDataItem::nextSibling();
}


K3bDataItem* K3bDirItem::nextChild( K3bDataItem* prev )
{
  // search for prev in children
  if( m_children->find( prev ) < 0 ) {
    return 0;
  }
  else
    return m_children->next();
}


QString K3bDirItem::localPath()
{
  return doc()->dummyDir();
}



K3bRootItem::K3bRootItem( K3bDataDoc* doc )
  : K3bDirItem( "root", doc, 0 )
{
}


K3bRootItem::~K3bRootItem()
{
}

QString K3bRootItem::k3bPath()
{
  // graft-points have to start with the name of the directory or the file, not with a slash or anything!
  return "";
}

bool K3bDirItem::alreadyInDirectory( const QString& fileName ) const
{
  QListIterator<K3bDataItem> it( *m_children );
  for( ; it.current(); ++it ) {
    if( it.current()->k3bName() == fileName )
      return true;
  }
  return false;
}


long K3bDirItem::k3bSize() const
{
  QListIterator<K3bDataItem> it( *m_children );
  long size = 0;
  for( ; it.current(); ++it )
    size += it.current()->k3bSize();

  return size;
}


bool K3bDirItem::isSubItem( K3bDataItem* item ) const
{
  if( dynamic_cast<K3bDirItem*>(item) == this )
    return true;

  K3bDirItem* d = item->parent();
  while( d ) {
    if( d == this ) {
      return true;
    }
    d = d->parent();
  }

  return false;
}


int K3bDirItem::numFiles() const
{
  int num = 0;

  QListIterator<K3bDataItem> it( *m_children );
  for( ; it.current(); ++it )
    if( !it.current()->isDir() )
      num++;

  return num;
}


int K3bDirItem::numDirs() const
{
  return m_children->count() - numFiles();
}

