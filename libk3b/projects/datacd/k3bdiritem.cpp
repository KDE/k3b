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


#include "k3bdiritem.h"
#include "k3bdatadoc.h"
#include "k3bsessionimportitem.h"
#include "k3bfileitem.h"

#include <qstring.h>
#include <qptrlist.h>

#include <kdebug.h>


K3bDirItem::K3bDirItem(const QString& name, K3bDataDoc* doc, K3bDirItem* parentDir)
  : K3bDataItem( doc, parentDir ),
    m_size(0),
    m_followSymlinksSize(0),
    m_blocks(0),
    m_followSymlinksBlocks(0),
    m_files(0),
    m_dirs(0)
{
  m_k3bName = name;
  
  // add automagically like a qlistviewitem
  if( parent() )
    parent()->addDataItem( this );
}


K3bDirItem::K3bDirItem( const K3bDirItem& item )
  : K3bDataItem( item ),
    m_size(0),
    m_followSymlinksSize(0),
    m_blocks(0),
    m_followSymlinksBlocks(0),
    m_files(0),
    m_dirs(0),
    m_localPath( item.m_localPath )
{
  for( QPtrListIterator<K3bDataItem> it( item.children() ); *it; ++it )
    addDataItem( (*it)->copy() );
}

K3bDirItem::~K3bDirItem()
{
  // delete all children
  // doing this by hand is much saver than using the 
  // auto-delete feature since some of the items' destructors
  // may change the list
  K3bDataItem* i = m_children.first();
  while( i ) {
    // it is important to use takeDataItem here to be sure
    // the size gets updated properly
    takeDataItem(i);
    delete i;
    i = m_children.first();
  }    

  // this has to be done after deleting the children
  // because the directory itself has a size of 0 in K3b
  // and all it's files' sizes have already been substracted
  take();
}


K3bDataItem* K3bDirItem::copy() const
{
  return new K3bDirItem( *this );
}


K3bDirItem* K3bDirItem::getDirItem() const
{
  return const_cast<K3bDirItem*>(this);
}

K3bDirItem* K3bDirItem::addDataItem( K3bDataItem* item )
{
  // check if we are a subdir of item
  if( K3bDirItem* dirItem = dynamic_cast<K3bDirItem*>(item) ) {
    if( dirItem->isSubItem( this ) ) {
      kdDebug() << "(K3bDirItem) trying to move a dir item down in it's own tree." << endl;
      return this;
    }
  }

  if( m_children.findRef( item ) == -1 ) {
    if( item->isFile() ) {
      // do we replace an old item?
      QString name = item->k3bName();
      int cnt = 1;
      while( K3bDataItem* oldItem = find( name ) ) {
	if( !oldItem->isDir() && oldItem->isFromOldSession() ) {
	  // in this case we remove this item from it's parent and save it in the new one
	  // to be able to recover it
	  oldItem->take();
	  static_cast<K3bSessionImportItem*>(oldItem)->setReplaceItem( static_cast<K3bFileItem*>(item) );
	  static_cast<K3bFileItem*>(item)->setReplacedItemFromOldSession( oldItem );
	  break;
	}
	else {
	  //
	  // add a counter to the filename
	  //
	  if( item->k3bName()[item->k3bName().length()-4] == '.' )
	    name = item->k3bName().left( item->k3bName().length()-4 ) + QString::number(cnt++) + item->k3bName().right(4);
	  else
	    name = item->k3bName() + QString::number(cnt++);
	}
      }
      item->setK3bName( name );
    }

    m_children.append( item->take() );
    updateSize( item, false );
    if( item->isDir() )
      updateFiles( ((K3bDirItem*)item)->numFiles(), ((K3bDirItem*)item)->numDirs()+1 );
    else
      updateFiles( 1, 0 );

    item->m_parentDir = this;

    // inform the doc
    if( doc() )
      doc()->itemAddedToDir( this, item );
  }

  return this;
}


K3bDataItem* K3bDirItem::takeDataItem( K3bDataItem* item )
{
  int x = m_children.findRef( item );
  if( x > -1 ) {
    K3bDataItem* item = m_children.take();
    updateSize( item, true );
    if( item->isDir() )
      updateFiles( -1*((K3bDirItem*)item)->numFiles(), -1*((K3bDirItem*)item)->numDirs()-1 );
    else
      updateFiles( -1, 0 );

    item->m_parentDir = 0;
    
    // inform the doc
    if( doc() )
      doc()->itemRemovedFromDir( this, item );
    
    if( item->isFile() ) {
      // restore the item imported from an old session
      if( static_cast<K3bFileItem*>(item)->replaceItemFromOldSession() )
	addDataItem( static_cast<K3bFileItem*>(item)->replaceItemFromOldSession() );
    }

    return item;
  }
  else
    return 0;
}


K3bDataItem* K3bDirItem::nextSibling() const
{
  if( !m_children.isEmpty() )
    return m_children.getFirst();
  else
    return K3bDataItem::nextSibling();
}


K3bDataItem* K3bDirItem::nextChild( K3bDataItem* prev ) const
{
  // search for prev in children
  if( m_children.findRef( prev ) < 0 ) {
    return 0;
  }
  else
    return m_children.next();
}


bool K3bDirItem::alreadyInDirectory( const QString& filename ) const
{
  return (find( filename ) != 0);
}


K3bDataItem* K3bDirItem::find( const QString& filename ) const
{
  for( QPtrListIterator<K3bDataItem> it( m_children ); it.current(); ++it ) {
    if( it.current()->k3bName() == filename )
      return it.current();
  }
  return 0;
}


K3bDataItem* K3bDirItem::findByPath( const QString& p )
{
  if( p.isEmpty() || p == "/" )
    return this;

  QString path = p;
  if( path.startsWith("/") )
    path = path.mid(1);
  int pos = path.find( "/" );
  if( pos < 0 )
    return find( path );
  else {
    // do it recursivly
    K3bDataItem* item = find( path.left(pos) );
    if( item && item->isDir() )
      return ((K3bDirItem*)item)->findByPath( path.mid( pos+1 ) );
    else
      return 0;
  }
}


bool K3bDirItem::mkdir( const QString& dirPath )
{
  // 
  // An absolut path always starts at the root item
  //
  if( dirPath[0] == '/' ) {
    if( parent() )
      return parent()->mkdir( dirPath );
    else
      return mkdir( dirPath.mid( 1 ) );
  }

  if( findByPath( dirPath ) )
    return false;

  QString restPath;
  QString dirName;
  int pos = dirPath.find( '/' );
  if( pos == -1 ) {
    dirName = dirPath;
  }
  else {
    dirName = dirPath.left( pos );
    restPath = dirPath.mid( pos+1 );
  }

  K3bDataItem* dir = find( dirName );
  if( !dir )
    dir = new K3bDirItem( dirName, doc(), this );
  else if( !dir->isDir() )
    return false;

  if( !restPath.isEmpty() )
    return static_cast<K3bDirItem*>(dir)->mkdir( restPath );

  return true;
}


KIO::filesize_t K3bDirItem::itemSize( bool followsylinks ) const
{
  if( followsylinks )
    return m_followSymlinksSize;
  else
    return m_size;
}


K3b::Msf K3bDirItem::itemBlocks( bool followSymlinks ) const
{
  if( followSymlinks )
    return m_followSymlinksBlocks;
  else
    return m_blocks;
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


long K3bDirItem::numFiles() const
{
  return m_files;
}


long K3bDirItem::numDirs() const
{
  return m_dirs;
}


bool K3bDirItem::isRemoveable() const
{
  if( !K3bDataItem::isRemoveable() )
    return false;

  for( QPtrListIterator<K3bDataItem> it( m_children ); it.current(); ++it ) {
    if( !it.current()->isRemoveable() )
      return false;
  }

  return true;
}


void K3bDirItem::updateSize( K3bDataItem* item, bool removed )
{
  if( removed ) {
    m_followSymlinksSize -= item->itemSize( true );
    m_size -= item->itemSize( false );
    m_followSymlinksBlocks -= item->itemBlocks( true ).lba();
    m_blocks -= item->itemBlocks( false ).lba();
  }
  else {
    m_followSymlinksSize += item->itemSize( true );
    m_size += item->itemSize( false );
    m_followSymlinksBlocks += item->itemBlocks( true ).lba();
    m_blocks += item->itemBlocks( false ).lba();
  }

  if( parent() )
    parent()->updateSize( item, removed );
}

void K3bDirItem::updateFiles( long files, long dirs )
{
  m_files += files;
  m_dirs += dirs;
  if( parent() )
    parent()->updateFiles( files, dirs );
}


bool K3bDirItem::isFromOldSession() const
{
  for( QPtrListIterator<K3bDataItem> it( m_children ); it.current(); ++it ) {
    if( (*it)->isFromOldSession() )
      return true;
  }
  return false;
}


bool K3bDirItem::writeToCd() const
{
  // check if this dir contains items to write
  for( QPtrListIterator<K3bDataItem> it( m_children ); it.current(); ++it ) {
    if( (*it)->writeToCd() )
      return true;
  }
  return K3bDataItem::writeToCd();
}


K3bRootItem::K3bRootItem( K3bDataDoc* doc )
  : K3bDirItem( "root", doc, 0 )
{
}


K3bRootItem::~K3bRootItem()
{
}


const QString& K3bRootItem::k3bName()
{
  return doc()->isoOptions().volumeID();
}


void K3bRootItem::setK3bName( const QString& text )
{
  doc()->setVolumeID( text );
}
