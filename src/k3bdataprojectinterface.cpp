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

#include "k3bdataprojectinterface.h"

#include <k3bdatadoc.h>
#include <k3bdiritem.h>
#include <k3bisooptions.h>


K3bDataProjectInterface::K3bDataProjectInterface( K3bDataDoc* doc, const char* name )
  : K3bProjectInterface( doc, name ),
    m_dataDoc(doc)
{
}


K3bDataProjectInterface::~K3bDataProjectInterface()
{
}


void K3bDataProjectInterface::createFolder( const QString& name )
{
  createFolder( name, "/" );
}


void K3bDataProjectInterface::createFolder( const QString& name, const QString& parent )
{
  K3bDataItem* p = m_dataDoc->root()->findByPath( parent );
  if( p && p->isDir() )
    m_dataDoc->addEmptyDir( name, static_cast<K3bDirItem*>(p) );
}


void K3bDataProjectInterface::addUrl( const KURL& url, const QString& parent )
{
  addUrls( KURL::List(url), parent );
}


void K3bDataProjectInterface::addUrls( const KURL::List& urls, const QString& parent )
{
  K3bDataItem* p = m_dataDoc->root()->findByPath( parent );
  if( p && p->isDir() )
    m_dataDoc->addUrls( urls, static_cast<K3bDirItem*>(p) );
}


void K3bDataProjectInterface::removeItem( const QString& path )
{
  K3bDataItem* p = m_dataDoc->root()->findByPath( path );
  if( p )
    m_dataDoc->removeItem( p );
}


void K3bDataProjectInterface::renameItem( const QString& path, const QString& newName )
{
  K3bDataItem* p = m_dataDoc->root()->findByPath( path );
  if( p && !newName.isEmpty() )
    p->setK3bName( newName );
}


void K3bDataProjectInterface::setVolumeID( const QString& id )
{
  m_dataDoc->setVolumeID( id );
}

bool K3bDataProjectInterface::isFolder( const QString& path ) const
{
  K3bDataItem* p =  m_dataDoc->root()->findByPath( path );
  if( p )
    return p->isDir();
  else
    return false;
}


QStringList K3bDataProjectInterface::children( const QString& path ) const
{
  QStringList l;
  K3bDataItem* item =  m_dataDoc->root()->findByPath( path );
  if( item && item->isDir() ) {
    const QPtrList<K3bDataItem>& cl = static_cast<K3bDirItem*>(item)->children();
    for( QPtrListIterator<K3bDataItem> it( cl ); *it; ++it )
      l.append( it.current()->k3bName() );
  }

  return l;
}
