/***************************************************************************
                          k3bdataitem.cpp  -  description
                             -------------------
    begin                : Wed Apr 25 2001
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

#include "k3bdataitem.h"
#include "k3bdiritem.h"
#include <kdebug.h>


K3bDataItem::K3bDataItem( K3bDataDoc* doc, K3bDataItem* parent )
{
  m_doc = doc;
	
  // add automagically like a qlistviewitem
  if( parent )
    m_parentDir = parent->addDataItem( this );
  else
    m_parentDir = 0;
}

K3bDataItem::~K3bDataItem()
{
  // remove this from parentdir
  if( m_parentDir )
    m_parentDir->takeDataItem( this );
}


void K3bDataItem::setK3bName( const QString& name ) {
  // test for not-allowed characters
  // TODO: use QRegExp
  if( name.contains('/') || name.contains('?') || name.contains('*') ) {
    kdDebug() << "(K3bDataItem) name contained invalid characters!" << endl;
    return;
  }
//   if( parent() ) {
//     QPtrList<K3bDataItem>* _itemsInDir = parent()->children();
//     for( K3bDataItem* _it = _itemsInDir->first(); _it; _it = _itemsInDir->next() ) {
//       if( _it != this && _it->k3bName() == name ) {
// 	kdDebug() << "(K3bDataItem) already a file with that name in directory: " << _it->k3bName() << endl;
// 	return;
//       }
//     }
//   }

  m_k3bName = name;
}


void K3bDataItem::setJolietName( const QString& name )
{
  m_jolietName = name;
}


const QString& K3bDataItem::k3bName()
{
  return m_k3bName;
}


const QString& K3bDataItem::jolietName()
{
  return m_jolietName;
}


QString K3bDataItem::k3bPath()
{
  if( !m_parentDir )
    return k3bName();
  else
    return m_parentDir->k3bPath() + k3bName();
}


QString K3bDataItem::jolietPath()
{
  if( !m_parentDir )
    return jolietName();
  else
    return m_parentDir->jolietPath() + jolietName();
}


K3bDataItem* K3bDataItem::nextSibling()
{
  K3bDataItem* _item = this;
  K3bDirItem* _parentItem = parent();
	
  while( _parentItem ) {
    if( K3bDataItem* i = _parentItem->nextChild( _item ) )
      return i;
		
    _item = _parentItem;
    _parentItem = _item->parent();
  }

  return 0;
}


void K3bDataItem::reparent( K3bDirItem* newParent )
{
  if( m_parentDir ) {
    m_parentDir->takeDataItem( this );
  }

  m_parentDir = newParent->addDataItem( this );
}
