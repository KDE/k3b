/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bfilecompilationsizehandler.h"
#include "k3bdataitem.h"

#include <kdebug.h>

#include <qfile.h>
#include <qmap.h>
#include <qptrlist.h>


// TODO: remove the items from the project if the savedSize differs
// with some info-widget: "Files xxx have changed on disk. Removing them from the project."
// or we just update the sizes!


class InodeInfo
{
public:
  InodeInfo() {
    number = 0;
    savedSize = 0;
  }

  /**
   * How often has the file with
   * the corresponding inode been added
   */
  int number;

  /**
   * The size of the first added file. This has to be saved
   * to check further addings and to avoid the following situation:
   * A file with inode 1 is added, then deleted. Another file is created 
   * at inode 1 and added to the project. Now the first file gets
   * removed and then the second. If we had not saved the size we would
   * have added the size of the first and removed the size of the second
   * file resulting in a corrupted project size.
   * This way we always use the size of the first added file and may
   * warn the user if sizes differ.
   */
  KIO::filesize_t savedSize;

  KIO::filesize_t completeSize() const { return savedSize*number; }

  QPtrList<K3bDataItem> items;
};


class K3bFileCompilationSizeHandler::Private
{
public:
  /**
   * This maps from inodes to the number of occurrences of the inode.
   */
  QMap<int, InodeInfo> inodeMap;

  KIO::filesize_t size;

  QPtrList<K3bDataItem> specialItems;
};



K3bFileCompilationSizeHandler::K3bFileCompilationSizeHandler()
{
  d = new Private;
  d->size = 0;
}

K3bFileCompilationSizeHandler::~K3bFileCompilationSizeHandler()
{
  delete d;
}


const KIO::filesize_t& K3bFileCompilationSizeHandler::size() const
{
  return d->size;
}



void K3bFileCompilationSizeHandler::addFile( K3bDataItem* item )
{
  if( item->isSpecialFile() ) {
    // special files do not have a corresponding local file
    // so we just add their k3bSize
    d->size += item->k3bSize();
    d->specialItems.append( item );
  }
  else if( item->isFile() ) {
    InodeInfo& inodeInfo = d->inodeMap[item->localInode()];

    inodeInfo.items.append( item );

    if( inodeInfo.number == 0 ) {
      inodeInfo.savedSize = item->k3bSize();
      d->size += item->k3bSize();
    }

    if( item->k3bSize() != inodeInfo.savedSize ) {
      kdError() << "(K3bFileCompilationSizeHandler) savedSize differs!" << endl;
    }

    inodeInfo.number++;
  }
}


void K3bFileCompilationSizeHandler::removeFile( K3bDataItem* item )
{
  if( item->isSpecialFile() ) {
    // special files do not have a corresponding local file
    // so we just substract their k3bSize
    if( d->specialItems.findRef( item ) == -1 ) {
      kdError() << "(K3bFileCompilationSizeHandler) Special item "
		<< item->k3bName()
		<< " has been removed without being added!" << endl;
    }
    else {
      d->specialItems.removeRef( item );
      d->size -= item->k3bSize();
    }
  }
  else if( item->isFile() ) {
    InodeInfo& inodeInfo = d->inodeMap[item->localInode()];
    
    if( inodeInfo.items.findRef( item ) == -1 ) {
      kdError() << "(K3bFileCompilationSizeHandler) " 
		<< item->localPath()
		<< " has been removed without being added!" << endl;
    }
    else {
      if( item->k3bSize() != inodeInfo.savedSize ) {
	kdError() << "(K3bFileCompilationSizeHandler) savedSize differs!" << endl;
      }

      inodeInfo.items.removeRef( item );
      inodeInfo.number--;
      if( inodeInfo.number == 0 )
	d->size -= inodeInfo.savedSize;
    }
  }
}


void K3bFileCompilationSizeHandler::clear()
{
  d->inodeMap.clear();
  d->size = 0;
}
