/*
 *
 * Copyright (C) 2007-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bdirsizejob.h"

#include <k3bthread.h>
#include <k3bthreadjob.h>
#include <k3bsimplejobhandler.h>
#include <k3bglobals.h>

#include <kdebug.h>
#include <kglobal.h>

#include <qfileinfo.h>
#include <qdir.h>


class K3bDirSizeJob::Private
{
public:
    Private()
        : followSymlinks(false),
          totalSize(0),
          totalFiles(0),
          totalDirs(0),
          totalSymlinks(0) {
    }

    KUrl::List urls;
    bool followSymlinks;

    KIO::filesize_t totalSize;
    KIO::filesize_t totalFiles;
    KIO::filesize_t totalDirs;
    KIO::filesize_t totalSymlinks;
};



K3bDirSizeJob::K3bDirSizeJob( QObject* parent )
    : K3bThreadJob( new K3bSimpleJobHandler(), parent ),
      d( new Private() )
{
}


K3bDirSizeJob::~K3bDirSizeJob()
{
    delete d;
    delete jobHandler();
}


KIO::filesize_t K3bDirSizeJob::totalSize() const
{
    return d->totalSize;
}


KIO::filesize_t K3bDirSizeJob::totalFiles() const
{
    return d->totalFiles;
}


KIO::filesize_t K3bDirSizeJob::totalDirs() const
{
    return d->totalDirs;
}


KIO::filesize_t K3bDirSizeJob::totalSymlinks() const
{
    return d->totalSymlinks;
}


void K3bDirSizeJob::setUrls( const KUrl::List& urls )
{
    d->urls = urls;
}


void K3bDirSizeJob::setFollowSymlinks( bool b )
{
    d->followSymlinks = b;
}

bool K3bDirSizeJob::run()
{
    d->totalSize = 0;
    d->totalFiles = 0;
    d->totalDirs = 0;
    d->totalSymlinks = 0;

    QStringList l;
    for( KUrl::List::const_iterator it = d->urls.begin();
         it != d->urls.end(); ++it ) {
        const KUrl& url = *it;

        if( !url.isLocalFile() ) {
            kDebug() << "(K3bDirSizeJob) no remote support.";
            return false;
        }

        l.append( url.path() );
    }

    return countFiles( l, QString() );
}


bool K3bDirSizeJob::countDir( const QString& dir )
{
    QStringList l = QDir(dir).entryList( QDir::TypeMask|QDir::Hidden|QDir::System|QDir::NoDotAndDotDot );
    return countFiles( l, dir );
}


bool K3bDirSizeJob::countFiles( const QStringList& l, const QString& dir )
{
    for( QStringList::const_iterator it = l.begin();
         it != l.end(); ++it ) {

        if( canceled() )
            return false;

        k3b_struct_stat s;
        if( k3b_lstat( QFile::encodeName( dir + *it ), &s ) )
            return false;

        if( S_ISLNK( s.st_mode ) ) {
            ++d->totalSymlinks;
            if( d->followSymlinks ) {
                if( k3b_stat( QFile::encodeName( dir + *it ), &s ) )
                    return false;
            }
        }

        if( S_ISDIR( s.st_mode ) ) {
            ++d->totalDirs;
            if( !countDir( dir + *it + '/' ) )
                return false;
        }
        else if( !S_ISLNK( s.st_mode ) ) {
            ++d->totalFiles;
            d->totalSize += (KIO::filesize_t)s.st_size;
        }
    }

    return true;
}

#include "k3bdirsizejob.moc"
