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


#include "k3bfileitem.h"
#include "k3bdatadoc.h"
#include "k3bdiritem.h"
#include "k3bisooptions.h"

#include <qfileinfo.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qregexp.h>
#include <qfile.h>

#include <kurl.h>
#include <kdebug.h>

#include <sys/stat.h>


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



K3bFileItem::K3bFileItem( const QString& filePath, K3bDataDoc* doc, K3bDirItem* dir, const QString& k3bName )
  : KFileItem( 0, 0, KURL::fromPathOrURL(filePath) ), K3bDataItem( doc, dir ),
    m_replacedItemFromOldSession(0)
{
  if( k3bName.isEmpty() )
    m_k3bName = QFileInfo(filePath).fileName();
  else
    m_k3bName = k3bName;


  // we determine the size here to avoid problems with removed or renamed files
  // we need to use lstat here since for symlinks both KDE and QT return the size of the file pointed to
  // instead the size of the link.
  struct stat statBuf;
  if( lstat( QFile::encodeName(filePath), &statBuf ) ) {
    m_size = size();
    kdError() << "(KFileItem) lstat failed." << endl;
  }
  else {
    m_size = (KIO::filesize_t)statBuf.st_size;

    //
    // integrate the device number into the inode since files on different
    // devices may have the same inode number!
    //
    m_id.inode = statBuf.st_ino;
    m_id.device = statBuf.st_dev;
  }

  // add automagically like a qlistviewitem
  if( parent() )
    parent()->addDataItem( this );
}


K3bFileItem::~K3bFileItem()
{
  // remove this from parentdir
  if( parent() )
    parent()->takeDataItem( this );

  // restore the item imported from an old session
  if( replaceItemFromOldSession() )
    parent()->addDataItem( replaceItemFromOldSession() );
}


KIO::filesize_t K3bFileItem::k3bSize() const
{
  return m_size;
}


bool K3bFileItem::exists() const
{
  return isLocalFile();
}

QString K3bFileItem::absIsoPath()
{
  //	return m_dir->absIsoPath() + m_isoName;
  return QString::null;
}


//K3bDataItem* K3bFileItem::nextSibling()
//{
//	K3bDataItem* _item = this;
//	K3bDataItem* _parentItem = parent();
//
//	while( _parentItem ) {
//		if( K3bDataItem* i = _parentItem->nextChild( _item ) )
//			return i;
//
//		_item = _parentItem;
//		_parentItem = _item->parent();
//	}
//
//	return 0;
//
//	if( parent() ) {
//		if( K3bDataItem* i = parent()->nextChild( this ) )
//			return i;
//		else {
//			// test if parent() has a parent
//			if( parent()->parent() )
//				return parent()->parent()->nextChild( parent() );
//			else
//				return 0;
//		}
//	}
//	else {
//		kdDebug() << "(K3bFileItem) ERROR: K3bFileItem without parent dirItem!!" << endl;
//		return 0;
//	}
//}


QString K3bFileItem::localPath() const
{
  return url().path();
}

K3bDirItem* K3bFileItem::getDirItem()
{
  return parent();
}


bool K3bFileItem::isSymLink() const
{
  // KFileItem::isLink seems to be broken
  return QFileInfo( localPath() ).isSymLink();
  //  return KFileItem::isLink();
}


bool K3bFileItem::isValid() const
{
  //
  // The link is always valid if the user wants all symlinks to be resolved
  //
  if( isSymLink() && !doc()->isoOptions().followSymbolicLinks() ) {
    QString dest = linkDest();

    if( dest[0] == '/' )
      return false;  // absolut links can never be part of the compilation!

    // parse the link
    K3bDirItem* dir = parent();

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
