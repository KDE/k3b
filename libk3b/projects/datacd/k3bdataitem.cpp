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


#include "k3bdataitem.h"
#include "k3bdiritem.h"
#include "k3bdatadoc.h"
#include <kdebug.h>

#include <math.h>


class K3bDataItem::Private
{
public:
  int flags;
};


K3bDataItem::K3bDataItem( K3bDataDoc* doc, K3bDataItem* parent, int flags )
  : m_bHideOnRockRidge(false),
    m_bHideOnJoliet(false),
    m_bRemoveable(true),
    m_bRenameable(true),
    m_bMovable(true),
    m_bHideable(true),
    m_bWriteToCd(true),
    m_sortWeight(0)
{
  d = new Private;
  d->flags = flags;

  m_doc = doc;
  m_bHideOnRockRidge = m_bHideOnJoliet = false;

  if( parent )
    m_parentDir = parent->getDirItem();
  else
    m_parentDir = 0;
}


K3bDataItem::K3bDataItem( const K3bDataItem& item )
  : m_k3bName( item.m_k3bName ),
    m_doc( 0 ),
    m_parentDir( 0 ),
    m_bHideOnRockRidge( item.m_bHideOnRockRidge ),
    m_bHideOnJoliet( item.m_bHideOnJoliet ),
    m_bRemoveable( item.m_bRemoveable ),
    m_bRenameable( item.m_bRenameable ),
    m_bMovable( item.m_bMovable ),
    m_bHideable( item.m_bHideable ),
    m_bWriteToCd( item.m_bWriteToCd ),
    m_extraInfo( item.m_extraInfo ),
    m_sortWeight( item.m_sortWeight )
{
  d = new Private;
  d->flags = item.d->flags;
}


K3bDataItem::~K3bDataItem()
{
  delete d;
}


void K3bDataItem::setFlags( int flags )
{
  d->flags = flags;
}


bool K3bDataItem::isBootItem() const
{
  return d->flags & BOOT_IMAGE;
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
    if ( name != m_k3bName ) {
        // test for not-allowed characters
        if( name.contains('/') ) {
            kDebug() << "(K3bDataItem) name contained invalid characters!";
            return;
        }

        if( parent() ) {
            K3bDataItem* item = parent()->find( name );
            if( item && item != this ) {
                kDebug() << "(K3bDataItem) item with that name already exists.";
                return;
            }
        }

        m_k3bName = name;
        m_doc->setModified();
    }
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
    return QString();  // the root item is the only one not having a parent
  else if( isDir() )
    return getParent()->k3bPath() + k3bName() + "/";
  else
    return getParent()->k3bPath() + k3bName();
}


QString K3bDataItem::writtenPath() const
{
  if( !getParent() )
    return QString();  // the root item is the only one not having a parent
  else if( isDir() )
    return getParent()->writtenPath() + writtenName() + "/";
  else
    return getParent()->writtenPath() + writtenName();
}


QString K3bDataItem::iso9660Path() const
{
  if( !getParent() )
    return QString();  // the root item is the only one not having a parent
  else if( isDir() )
    return getParent()->iso9660Path() + iso9660Name() + "/";
  else
    return getParent()->iso9660Path() + iso9660Name();
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
    if( ( !getParent() || !getParent()->hideOnRockRidge() ) &&
        b != m_bHideOnRockRidge ) {
        m_bHideOnRockRidge = b;
        if ( m_doc )
            m_doc->setModified();
    }
}


void K3bDataItem::setHideOnJoliet( bool b )
{
  // there is no use in changing the value if
  // it is already set by the parent
    if( ( !getParent() || !getParent()->hideOnJoliet() ) &&
        b != m_bHideOnJoliet ) {
    m_bHideOnJoliet = b;
    if ( m_doc )
        m_doc->setModified();
  }
}


int K3bDataItem::depth() const
{
  if( getParent() )
    return getParent()->depth() + 1;
  else
    return 0;
}
