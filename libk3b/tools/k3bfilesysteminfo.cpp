/*
 *
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

#include <config-k3b.h>

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



class K3b::FileSystemInfo::Private
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
#ifndef Q_OS_WIN32	
        struct statfs fs;
        if( !::statfs( QFile::encodeName( QFileInfo(path).absolutePath() ), &fs ) ) {
            switch( fs.f_type ) {
            case 0x4d44: // MS-DOS
                type = FS_FAT;
            default:
                type = FS_UNKNOWN;
            }

            statDone = true;
        }
        else {
            kDebug() << "(K3b::FileSystemInfo) statfs failed: " << ::strerror(errno);
        }
#else
		statDone = true;
#endif
    }
};


K3b::FileSystemInfo::FileSystemInfo()
{
    d = new Private;
}


K3b::FileSystemInfo::FileSystemInfo( const QString& path )
{
    d = new Private;
    d->path = path;
}


K3b::FileSystemInfo::FileSystemInfo( const K3b::FileSystemInfo& other )
{
    d = new Private;
    d->type = other.d->type;
    d->path = other.d->path;
    d->statDone = other.d->statDone;
}


K3b::FileSystemInfo::~FileSystemInfo()
{
    delete d;
}


QString K3b::FileSystemInfo::path() const
{
    return d->path;
}


void K3b::FileSystemInfo::setPath( const QString& path )
{
    if( d->path != path ) {
        d->path = path;
        d->statDone = false;
    }
}


K3b::FileSystemInfo::FileSystemType K3b::FileSystemInfo::type() const
{
    if( !d->statDone )
        d->stat();
    return d->type;
}


QString K3b::FileSystemInfo::fixupPath( const QString& path )
{
    QString s = K3b::fixupPath( path );
    if( type() == K3b::FileSystemInfo::FS_FAT )
        return s.replace( QRegExp("[\"\\?\\*/\\\\[\\]\\|\\=\\:;]"), "_" );
    else
        return s;
}
