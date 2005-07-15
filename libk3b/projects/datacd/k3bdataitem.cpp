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


#include "k3bdataitem.h"
#include "k3bdiritem.h"
#include "k3bdatadoc.h"
#include <kdebug.h>

#include <math.h>


K3bDataItem::K3bDataItem( K3bDataDoc* doc, K3bDataItem* parent )
  : m_bHideOnRockRidge(true),
    m_bHideOnJoliet(true),
    m_bRemoveable(true),
    m_bRenameable(true),
    m_bMovable(true),
    m_bHideable(true),
    m_bWriteToCd(true),
    m_sortWeigth(0)
{
  m_doc = doc;
  m_bHideOnRockRidge = m_bHideOnJoliet = false;

  if( parent )
    m_parentDir = parent->getDirItem();
  else
    m_parentDir = 0;
}

K3bDataItem::~K3bDataItem()
{
}


KIO::filesize_t K3bDataItem::size() const
{
  return itemSize( m_doc 
		   ? m_doc->isoOptions().followSymbolicLinks() || 
		   !m_doc->isoOptions().createRockRidge()
		   : false );
}


K3b::Msf K3bDataItem::blocks() const
{
  return itemBlocks( m_doc 
		     ? m_doc->isoOptions().followSymbolicLinks() || 
		     !m_doc->isoOptions().createRockRidge()
		     : false );
}


K3b::Msf K3bDataItem::itemBlocks( bool followSymbolicLinks ) const
{
  return (long)::ceil( (double)itemSize( followSymbolicLinks ) / 2048.0 );
}


void K3bDataItem::setK3bName( const QString& name ) {
  // test for not-allowed characters
  if( name.contains('/') ) {
    kdDebug() << "(K3bDataItem) name contained invalid characters!" << endl;
    return;
  }
   
  if( parent() ) {
    if( parent()->find( name ) ) {
      kdDebug() << "(K3bDataItem) item with that name already exists." << endl;
      return;
    }
  }

  m_k3bName = name;
}


const QString& K3bDataItem::k3bName() const
{
  return m_k3bName;
}


K3bDataItem* K3bDataItem::take()
{
  if( parent() )
    parent()->takeDataItem( this );

  return this;
}


QString K3bDataItem::k3bPath() const
{
  if( !getParent() )
    return QString::null;  // the root item is the only one not having a parent
  else if( isDir() )
    return getParent()->k3bPath() + k3bName() + "/";
  else
    return getParent()->k3bPath() + k3bName();
}


QString K3bDataItem::writtenPath() const
{
  if( !getParent() )
    return QString::null;  // the root item is the only one not having a parent
  else if( isDir() )
    return getParent()->writtenPath() + writtenName() + "/";
  else
    return getParent()->writtenPath() + writtenName();
}


K3bDataItem* K3bDataItem::nextSibling() const
{
  K3bDataItem* item = const_cast<K3bDataItem*>(this); // urg, but we know that we don't mess with it, so...
  K3bDirItem* parentItem = getParent();
	
  while( parentItem ) {
    if( K3bDataItem* i = parentItem->nextChild( item ) )
      return i;
		
    item = parentItem;
    parentItem = item->getParent();
  }

  return 0;
}


void K3bDataItem::reparent( K3bDirItem* newParent )
{
  // addDataItem will do all the stuff including taking this
  newParent->addDataItem( this );
}


bool K3bDataItem::hideOnRockRidge() const
{ 
  if( !isHideable() )
    return false;
  if( getParent() )
    return m_bHideOnRockRidge || getParent()->hideOnRockRidge();
  else 
    return m_bHideOnRockRidge;
}


bool K3bDataItem::hideOnJoliet() const
{
  if( !isHideable() )
    return false;
  if( getParent() ) 
    return m_bHideOnJoliet || getParent()->hideOnJoliet();
  else
    return m_bHideOnJoliet;
}


void K3bDataItem::setHideOnRockRidge( bool b )
{
  // there is no use in changing the value if 
  // it is already set by the parent
  if( !getParent() || !getParent()->hideOnRockRidge() )
    m_bHideOnRockRidge = b;
}


void K3bDataItem::setHideOnJoliet( bool b ) 
{ 
  // there is no use in changing the value if 
  // it is already set by the parent
  if( !getParent() || !getParent()->hideOnJoliet() )
    m_bHideOnJoliet = b;
}


int K3bDataItem::depth() const
{
  if( getParent() )
    return getParent()->depth() + 1;
  else
    return 0;
}
