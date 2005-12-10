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

#include "k3biso9660backend.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <qfile.h>

#include <k3bdevice.h>


// 
// K3bIso9660DeviceBackend -----------------------------------
// 

K3bIso9660DeviceBackend::K3bIso9660DeviceBackend( K3bDevice::Device* dev )
  : m_device( dev ),
    m_isOpen(false)
{
}


K3bIso9660DeviceBackend::~K3bIso9660DeviceBackend()
{
  close();
}


bool K3bIso9660DeviceBackend::open()
{
  if( m_isOpen )
    return true;
  else if( m_device->open() ) {
    // set optimal reading speed
    m_device->setSpeed( 0xffff, 0xffff );
    m_isOpen = true;
    return true;
  }
  else
    return false;
}


void K3bIso9660DeviceBackend::close()
{
  if( m_isOpen ) {
    m_isOpen = false;
    m_device->close();
  }
}


int K3bIso9660DeviceBackend::read( unsigned int sector, char* data, int len )
{
  int read = -1;

  if( isOpen() ) {
    int retries = 10;  // TODO: no fixed value
    while( retries && !m_device->read10( (unsigned char*)data,
					 len*2048,
					 sector,
					 len ) )
      retries--;
    
    if( retries > 0 )
      read = len;
  }

  return read;
}


// 
// K3bIso9660FileBackend -----------------------------------
// 

K3bIso9660FileBackend::K3bIso9660FileBackend( const QString& filename )
  : m_filename( filename ),
    m_fd( -1 ),
    m_closeFd( true )
{
}


K3bIso9660FileBackend::K3bIso9660FileBackend( int fd )
  : m_fd( fd ),
    m_closeFd( false )
{
}


K3bIso9660FileBackend::~K3bIso9660FileBackend()
{
  close();
}


#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif

bool K3bIso9660FileBackend::open()
{
  if( m_fd > 0 )
    return true;
  else {
    m_fd = ::open( QFile::encodeName( m_filename ), O_RDONLY|O_LARGEFILE );
    return ( m_fd > 0 );
  }
}


void K3bIso9660FileBackend::close()
{
  if( m_closeFd && m_fd > 0 ) {
    ::close( m_fd );
    m_fd = -1;
  }
}



bool K3bIso9660FileBackend::isOpen() const
{
  return ( m_fd > 0 );
}


int K3bIso9660FileBackend::read( unsigned int sector, char* data, int len )
{
  int read = 0;
  if( ::lseek( m_fd, static_cast<unsigned long long>(sector)*2048, SEEK_SET ) != -1 )
    if( (read = ::read( m_fd, data, len*2048 )) != -1 )
      return read / 2048;

  return -1;
}
