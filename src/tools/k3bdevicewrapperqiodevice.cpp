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

#include "k3bdevicewrapperqiodevice.h"

#include <k3bdevice.h>

#include <kdebug.h>

#include <unistd.h>


class K3bDeviceWrapperQIODevice::Private
{
public:
  Private()
    : pos(0),
      start(0),
      end(0xffffffff) {
  }

  QIODevice::Offset pos;
  QIODevice::Offset start;
  QIODevice::Offset end;
};


K3bDeviceWrapperQIODevice::K3bDeviceWrapperQIODevice( K3bCdDevice::CdDevice* dev )
  : QIODevice(),
    m_device(dev)
{
  d = new Private();
}


K3bDeviceWrapperQIODevice::~K3bDeviceWrapperQIODevice()
{
  delete d;
}



void K3bDeviceWrapperQIODevice::setTrackBorders( unsigned long start, unsigned long end )
{
  d->start = start;
  d->end = end;
}


bool K3bDeviceWrapperQIODevice::open( int )
{
  return ( m_device->open() != -1 );
}


void K3bDeviceWrapperQIODevice::close()
{
  m_device->close();
}


void K3bDeviceWrapperQIODevice::flush()
{
  // UNUSED
}


QIODevice::Offset K3bDeviceWrapperQIODevice::at() const
{
  return d->pos;
}


bool K3bDeviceWrapperQIODevice::at( QIODevice::Offset pos )
{
  if( d->start + pos <= d->end ) {
    d->pos = pos;
    return true;
  }
  else  
    return false;
}


bool K3bDeviceWrapperQIODevice::atEnd() const
{
  return d->start + d->pos >= d->end;
}


Q_LONG K3bDeviceWrapperQIODevice::readBlock( char* data, Q_ULONG maxlen )
{
  // FIXME: for now this only works for mode1 sectors because of the
  //        read length

  kdDebug() << "(K3bDeviceWrapperQIODevice) readBlock( " << (void*)data << ", " << maxlen << " ) at position " << d->pos << endl;
  kdDebug() << "(K3bDeviceWrapperQIODevice) readCd from " << (d->start+d->pos)/2048 << endl;

  if( (d->start+d->pos)%2048 || maxlen%2048 )
    kdDebug() << "(K3bDeviceWrapperQIODevice) WARNING: need to modify data." << endl;

//   unsigned char* buffer = (unsigned char*)data;
//   bool buffered = false;
//   unsigned long bufferLen = maxlen;
//   if( maxlen%2048 > 0 ) {
//     buffered = true;
//     bufferLen = maxlen+(2048-(maxlen%2048));
//     buffer = new unsigned char[bufferLen];
//     kdDebug() << "(K3bDeviceWrapperQIODevice) using buffer of size: " << bufferLen << endl;
//   }
  long read = -1;

  // FIXME: some drives may not support read12 but read10 which would be a better fallback

  if( m_device->read12( (unsigned char*)data,
			maxlen,
			(d->start + d->pos)/2048,
 			maxlen/2048 ) )
    read = maxlen;

  // fallback
  if( read < 0 ) {
    kdDebug() << "(K3bDeviceWrapperQIODevice) falling back to stdlib read" << endl;
    read = ::read( m_device->open(), data, maxlen );
  }

//   if( buffered ) {
//     if( read > 0 )
//       ::memcpy( data, buffer, maxlen );
//     delete [] buffer;
//     kdDebug() << "(K3bDeviceWrapperQIODevice) deleted buffer." << endl;
//   }

  return read;
}


Q_LONG K3bDeviceWrapperQIODevice::writeBlock( const char*, Q_ULONG )
{
  // UNUSED
  return -1;
}


int K3bDeviceWrapperQIODevice::getch()
{
  // UNUSED
  return -1;
}


int K3bDeviceWrapperQIODevice::putch( int )
{
  // UNUSED
  return -1;
}


int K3bDeviceWrapperQIODevice::ungetch( int )
{
  // UNUSED
  return -1;
}


QIODevice::Offset K3bDeviceWrapperQIODevice::size() const
{
  return d->end - d->start + 1;
}
