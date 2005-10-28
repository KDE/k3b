/* 
 *
 * $Id: sourceheader 380067 2005-01-19 13:03:46Z trueg $
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bplainimagesource.h"

#include <k3btoc.h>
#include <k3btrack.h>
#include <k3bglobals.h>

#include <klocale.h>

#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <qfile.h>


class K3bPlainImageSource::Private
{
public:
  Private()
    : fd( -1 ),
      skipBytes( 0 ) {
  }

  int fd;
  long skipBytes;
  long padBytes;
  long alreadyPadded;
  QString filename;
};




K3bPlainImageSource::K3bPlainImageSource( const QString& filename, K3bJobHandler* jh, QObject* parent )
  : K3bSimpleImageSource( jh, parent )
{
  d = new Private();
  setFilename( filename );
}


K3bPlainImageSource::K3bPlainImageSource( K3bJobHandler* jh, QObject* parent )
  : K3bSimpleImageSource( jh, parent )
{
  d = new Private();
}


K3bPlainImageSource::~K3bPlainImageSource()
{
  if( d->fd > 0 )
    ::close( d->fd );
}


void K3bPlainImageSource::setFilename( const QString& name )
{
  d->filename = name;
}


bool K3bPlainImageSource::init()
{
  d->fd = ::open( QFile::encodeName( d->filename ), O_LARGEFILE|O_RDONLY );
  if( d->fd > 0 ) {
    d->padBytes = (long)( K3b::filesize( d->filename ) % (KIO::filesize_t)2048 );
    d->alreadyPadded = 0;
    return ( (int)::lseek64( d->fd, (off64_t)d->skipBytes, SEEK_SET ) == d->skipBytes );
  }
  else {
    emit infoMessage( i18n("Could not open file '%1'.").arg(d->filename), ERROR );
    return false;
  }
}


K3bDevice::Toc K3bPlainImageSource::toc() const
{
  K3b::Msf size( (int)(( (double)K3b::filesize( d->filename ) - 
			 (double)d->skipBytes + 0.5 ) / 2048.0) );
  K3bDevice::Toc toc;
  toc.append( K3bDevice::Track( 0, size-1, K3bDevice::Track::DATA, K3bDevice::Track::MODE1 ) );
  return toc;
}



void K3bPlainImageSource::skip( long bytes )
{
  d->skipBytes = bytes;
}


long K3bPlainImageSource::simpleRead( char* data, long maxLen )
{
  long r = 0;
  if( d->fd > 0 ) {
    r = ::read( d->fd, data, maxLen );
    if( r < 0 )
      emit infoMessage( i18n("Read error on file '%1'.").arg(d->filename), ERROR );
  }
  
  if( r == 0 && d->padBytes > d->alreadyPadded ) {
    if( d->fd > 0 ) {
      ::close( d->fd );
      d->fd = -1;
    }

    maxLen = QMIN( d->padBytes, maxLen );
    ::memset( data, 0, maxLen );
    d->alreadyPadded += maxLen;

    kdDebug() << "(K3bPlainImageSource) padding " << maxLen << " bytes." << endl;

    return maxLen;
  }
  return r;
}
