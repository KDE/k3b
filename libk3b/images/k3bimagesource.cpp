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

#include "k3bimagesource.h"

#include <qtimer.h>
#include <qmap.h>

#include <unistd.h>


class K3bImageSource::Private
{
public:
  QMap<unsigned int, K3b::SectorSize> sectorSizeMap;
};


K3bImageSource::K3bImageSource( K3bJobHandler* hdl, QObject* parent )
  : K3bJob( hdl, parent ),
    m_session( 1 ),
    m_fd( -1 )
{
  d = new Private;
}


K3bImageSource::~K3bImageSource()
{
  delete d;
}


long K3bImageSource::writeToFd( char* data, long len )
{
  return ::write( m_fd, data, len );
}


void K3bImageSource::writeToFd( int fd )
{
  m_fd = fd;
}


void K3bImageSource::start( int session )
{
  setSession( session );
  start();
}


void K3bImageSource::determineToc()
{
  QTimer::singleShot( 0, this, SLOT(slotEmitTocReady()) );
}


void K3bImageSource::slotEmitTocReady()
{
  emit tocReady( true );
}


long K3bImageSource::read( char*, long )
{
  return -1;
}


K3b::SectorSize K3bImageSource::sectorSize( unsigned int track ) const
{
  if( d->sectorSizeMap.contains( track ) )
    return d->sectorSizeMap[track];
  else
    return K3b::SECTORSIZE_DATA_2048; // some default
}


void K3bImageSource::setSectorSize( unsigned int track, K3b::SectorSize size )
{
  d->sectorSizeMap[track] = size;
}


KIO::filesize_t K3bImageSource::trackSize( unsigned int track ) const
{
  return toc()[track-1].length().lba() * (KIO::filesize_t)sectorSize( track );
}


KIO::filesize_t K3bImageSource::tocSize() const
{
  KIO::filesize_t imageSpaceNeeded = 0;
  unsigned int numTracks = toc().count();
  for( unsigned int i = 1; i <= numTracks; ++i )
    imageSpaceNeeded += trackSize( i );
  return imageSpaceNeeded;
}

#include "k3bimagesource.moc"
