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

#include <config.h>
#include <k3bglobals.h>

#include "k3bfileitem.h"
#include "k3bdatadoc.h"
#include "k3bdiritem.h"
#include "k3bisooptions.h"
#include <k3bglobals.h>

#include <qfileinfo.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qregexp.h>
#include <qfile.h>

#include <kurl.h>
#include <kdebug.h>

#include <errno.h>
#include <string.h>


bool operator==( const K3bFileItem::Id& id1, const K3bFileItem::Id& id2 )
{
  return ( id1.device == id2.device && id1.inode == id2.inode );
}


bool operator<( const K3bFileItem::Id& id1, const K3bFileItem::Id& id2 )
{
  if( id1.device == id2.device )
    return ( id1.inode < id2.inode );
  else
    return ( id1.device < id2.device );
}


bool operator>( const K3bFileItem::Id& id1, const K3bFileItem::Id& id2 )
{
  return !( id2 < id1 || id1 == id2 );
}



K3bFileItem::K3bFileItem( const QString& filePath, K3bDataDoc* doc, K3bDirItem* dir, const QString& k3bName, int flags )
  : K3bDataItem( doc, dir, flags ),
    m_replacedItemFromOldSession(0),
    m_localPath(filePath)
{
  if( k3bName.isEmpty() )
    m_k3bName = filePath.section( '/', -1 );
  else
    m_k3bName = k3bName;

  // we determine the size here to avoid problems with removed or renamed files
  // we need to use lstat here since for symlinks both KDE and QT return the size of the file pointed to
  // instead the size of the link.
  k3b_struct_stat statBuf;
  if( k3b_lstat( QFile::encodeName(filePath), &statBuf ) ) {
    m_size = K3b::filesize( filePath );
    m_id.inode = 0;
    m_id.device = 0;
    m_bSymLink = false;

    kdError() << "(KFileItem) lstat failed: " << strerror(errno) << endl;

    // since we have no proper inode info, disable the inode caching in the doc
    if( doc ) {
      K3bIsoOptions o( doc->isoOptions() );
      o.setDoNotCacheInodes( true );
      doc->setIsoOptions( o );
    }
  }
  else {
    m_size = (KIO::filesize_t)statBuf.st_size;

    m_bSymLink = S_ISLNK(statBuf.st_mode);

    //
    // integrate the device number into the inode since files on different
    // devices may have the same inode number!
    //
    m_id.inode = statBuf.st_ino;
    m_id.device = statBuf.st_dev;
  }

  m_idFollowed = m_id;
  m_sizeFollowed = m_size;

  if( isSymLink() ) {
    k3b_struct_stat statBuf;
    if( k3b_stat( QFile::encodeName(filePath), &statBuf ) == 0 ) {
      m_idFollowed.inode = statBuf.st_ino;
      m_idFollowed.device = statBuf.st_dev;

      m_sizeFollowed = (KIO::filesize_t)statBuf.st_size;
    }
  }

  // add automagically like a qlistviewitem
  if( parent() )
    parent()->addDataItem( this );
}


K3bFileItem::K3bFileItem( const k3b_struct_stat* stat, 
			  const k3b_struct_stat* followedStat,
			  const QString& filePath, K3bDataDoc* doc, K3bDirItem* dir, const QString& k3bName )
  : K3bDataItem( doc, dir ),
    m_replacedItemFromOldSession(0),
    m_localPath(filePath)
{
  if( k3bName.isEmpty() )
    m_k3bName = filePath.section( '/', -1 );
  else
    m_k3bName = k3bName;

  m_size = (KIO::filesize_t)stat->st_size;
  m_bSymLink = S_ISLNK(stat->st_mode);

  //
  // integrate the device number into the inode since files on different
  // devices may have the same inode number!
  //
  m_id.inode = stat->st_ino;
  m_id.device = stat->st_dev;
  
  if( isSymLink() ) {
    m_idFollowed.inode = followedStat->st_ino;
    m_idFollowed.device = followedStat->st_dev;
    
    m_sizeFollowed = (KIO::filesize_t)followedStat->st_size;
  }
  else {
    m_idFollowed = m_id;
    m_sizeFollowed = m_size;
  }

  if( parent() )
    parent()->addDataItem( this );
}


K3bFileItem::K3bFileItem( const K3bFileItem& item )
  : K3bDataItem( item ),
    m_replacedItemFromOldSession(0),
    m_size( item.m_size ),
    m_sizeFollowed( item.m_sizeFollowed ),
    m_id( item.m_id ),
    m_idFollowed( item.m_idFollowed ),
    m_localPath( item.m_localPath ),
    m_bSymLink( item.m_bSymLink )
{
}


K3bFileItem::~K3bFileItem()
{
  // remove this from parentdir
  take();
}


K3bDataItem* K3bFileItem::copy() const
{
  return new K3bFileItem( *this );
}


KIO::filesize_t K3bFileItem::itemSize( bool followSymlinks ) const
{
  if( followSymlinks )
    return m_sizeFollowed;
  else
    return m_size;
}


K3bFileItem::Id K3bFileItem::localId() const
{
  return localId( doc() ? doc()->isoOptions().followSymbolicLinks() || !doc()->isoOptions().createRockRidge() : false );
}


K3bFileItem::Id K3bFileItem::localId( bool followSymlinks ) const
{
  if( followSymlinks )
    return m_idFollowed;
  else
    return m_id;
}


bool K3bFileItem::exists() const
{
  return true;
}

QString K3bFileItem::absIsoPath()
{
  //	return m_dir->absIsoPath() + m_isoName;
  return QString::null;
}


QString K3bFileItem::localPath() const
{
  return m_localPath;
}

K3bDirItem* K3bFileItem::getDirItem() const
{
  return getParent();
}


bool K3bFileItem::isSymLink() const
{
  return m_bSymLink;
}


QString K3bFileItem::linkDest() const
{
  return QFileInfo( localPath() ).readLink();
}


bool K3bFileItem::isValid() const
{
  if( isSymLink() ) {

    // this link is not valid if we cannot follow it if we want to
    if( doc()->isoOptions().followSymbolicLinks() ) {
      return QFile::exists( K3b::resolveLink( localPath() ) );
    }

    QString dest = linkDest();

    if( dest[0] == '/' )
      return false;  // absolut links can never be part of the compilation!

    // parse the link
    K3bDirItem* dir = getParent();

    QStringList tokens = QStringList::split( QRegExp("/+"), dest );  // two slashes or more do the same as one does!

    unsigned int i = 0;
    while( i < tokens.size() ) {
      if( tokens[i] == "." ) {
	// ignore it
      }
      else if( tokens[i] == ".." ) {
	// change the directory
	dir = dir->parent();
	if( dir == 0 )
	  return false;
      }
      else {
	// search for the item in dir
	K3bDataItem* d = dir->find( tokens[i] );
	if( d == 0 )
	  return false;

	if( d->isDir() ) {
	  // change directory
	  dir = (K3bDirItem*)d;
	}
	else {
	  if( i+1 != tokens.size() )
	    return false;  // if di is a file we need to be at the last token
	  else
	    return (dest[dest.length()-1] != '/');   // if the link destination ends with a slash
                                            	   // it can only point to a directory!
	}
      }

      i++;
    }

    return true;
  }
  else
    return true;
}
