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

#include "k3bdataprojectinterface.h"

#include <k3bdatadoc.h>
#include <k3bdiritem.h>
#include <k3bisooptions.h>
//Added by qt3to4:
#include <Q3PtrList>


K3b::DataProjectInterface::DataProjectInterface( K3b::DataDoc* doc )
  : K3b::ProjectInterface( doc ),
    m_dataDoc(doc)
{
}


K3b::DataProjectInterface::~DataProjectInterface()
{
}


bool K3b::DataProjectInterface::createFolder( const QString& name )
{
  return createFolder( name, "/" );
}


bool K3b::DataProjectInterface::createFolder( const QString& name, const QString& parent )
{
  K3b::DataItem* p = m_dataDoc->root()->findByPath( parent );
  if( p && p->isDir() && !static_cast<K3b::DirItem*>(p)->find( name ) ) {
    m_dataDoc->addEmptyDir( name, static_cast<K3b::DirItem*>(p) );
    return true;
  }
  return false;
}


void K3b::DataProjectInterface::addUrl( const QString& url, const QString& parent )
{
  addUrls( QStringList(url), parent );
}


void K3b::DataProjectInterface::addUrls( const QStringList& urls, const QString& parent )
{
  K3b::DataItem* p = m_dataDoc->root()->findByPath( parent );
  if( p && p->isDir() )
    m_dataDoc->addUrls( KUrl::List(urls), static_cast<K3b::DirItem*>(p) );
}


bool K3b::DataProjectInterface::removeItem( const QString& path )
{
  K3b::DataItem* p = m_dataDoc->root()->findByPath( path );
  if( p && p->isRemoveable() ) {
    m_dataDoc->removeItem( p );
    return true;
  }
  else
    return false;
}


bool K3b::DataProjectInterface::renameItem( const QString& path, const QString& newName )
{
  K3b::DataItem* p = m_dataDoc->root()->findByPath( path );
  if( p && p->isRenameable() && !newName.isEmpty() ) {
    p->setK3bName( newName );
    return true;
  }
  else
    return false;
}


void K3b::DataProjectInterface::setVolumeID( const QString& id )
{
  m_dataDoc->setVolumeID( id );
}

bool K3b::DataProjectInterface::isFolder( const QString& path ) const
{
  K3b::DataItem* p =  m_dataDoc->root()->findByPath( path );
  if( p )
    return p->isDir();
  else
    return false;
}


QStringList K3b::DataProjectInterface::children( const QString& path ) const
{
  QStringList l;
  K3b::DataItem* item =  m_dataDoc->root()->findByPath( path );
  if( item && item->isDir() ) {
    const Q3PtrList<K3b::DataItem>& cl = static_cast<K3b::DirItem*>(item)->children();
    for( Q3PtrListIterator<K3b::DataItem> it( cl ); *it; ++it )
      l.append( it.current()->k3bName() );
  }

  return l;
}


bool K3b::DataProjectInterface::setSortWeight( const QString& path, long weight ) const
{
  K3b::DataItem* item =  m_dataDoc->root()->findByPath( path );
  if( item ) {
    item->setSortWeight( weight );
    return true;
  }
  else
    return false;
}
