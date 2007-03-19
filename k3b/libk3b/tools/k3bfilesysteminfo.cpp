/* 
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 * Copyright (C) 2006 Sebastian Trueg <trueg@k3b.org>
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

#include <config.h>

#include "k3bfilesysteminfo.h"

#include <k3bglobals.h>

#include <qfile.h>
#include <qfileinfo.h>
#include <qregexp.h>

#include <kdebug.h>

#ifdef Q_OS_FREEBSD
#include <sys/param.h>
#include <sys/mount.h>
#endif
#ifdef HAVE_SYS_STATVFS_H
#  include <sys/statvfs.h>
#  if defined(Q_OS_NETBSD)
#    include <sys/param.h>
#    if __NetBSD_Version__ > 299000000
#      define statfs		statvfs
#      define f_type		f_fsid
#    endif
#  endif
#endif
#ifdef HAVE_SYS_VFS_H
#  include <sys/vfs.h>
#endif

#include <errno.h>
#include <string.h>



class K3bFileSystemInfo::Private
{
public:
  Private()
    : type(FS_UNKNOWN),
      statDone(false) {
  }

  FileSystemType type;
  QString path;

  bool statDone;

  void stat() {
    struct statfs fs;
    if( !::statfs( QFile::encodeName( QFileInfo(path).dirPath( true ) ), &fs ) ) {
      switch( fs.f_type ) {
      case 0x4d44: // MS-DOS
	type = FS_FAT;
      default:
	type = FS_UNKNOWN;
      }

      statDone = true;
    }
    else {
      kdDebug() << "(K3bFileSystemInfo) statfs failed: " << ::strerror(errno) << endl;
    }
  }
};


K3bFileSystemInfo::K3bFileSystemInfo()
{
  d = new Private;
}


K3bFileSystemInfo::K3bFileSystemInfo( const QString& path )
{
  d = new Private;
  d->path = path;
}


K3bFileSystemInfo::K3bFileSystemInfo( const K3bFileSystemInfo& other )
{
  d = new Private;
  d->type = other.d->type;
  d->path = other.d->path;
  d->statDone = other.d->statDone;
}


K3bFileSystemInfo::~K3bFileSystemInfo()
{
  delete d;
}


QString K3bFileSystemInfo::path() const
{
  return d->path;
}


void K3bFileSystemInfo::setPath( const QString& path )
{
  if( d->path != path ) {
    d->path = path;
    d->statDone = false;
  }
}


K3bFileSystemInfo::FileSystemType K3bFileSystemInfo::type() const
{
  if( !d->statDone )
    d->stat();
  return d->type;
}


QString K3bFileSystemInfo::fixupPath( const QString& path )
{
  QString s = K3b::fixupPath( path );
  if( type() == K3bFileSystemInfo::FS_FAT )
    return s.replace( QRegExp("[\"\\?\\*/\\\\[\\]\\|\\=\\:;]"), "_" );
  else
    return s;
}
