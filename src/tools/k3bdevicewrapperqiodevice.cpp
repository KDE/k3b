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

  bool readCdFailed;
  bool read12Failed;
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
  if( maxlen == 0 )
    return 0;

  kdDebug() << "(K3bDeviceWrapperQIODevice) readBlock( " << (void*)data << ", " << maxlen << " ) at position " << d->pos << endl;
  kdDebug() << "(K3bDeviceWrapperQIODevice) readCd from " << (d->start+d->pos)/2048 << endl;

  unsigned long startSec = d->start+d->pos;
  int startSecOffset = 0;
  unsigned char* buffer = (unsigned char*)data;
  bool buffered = false;
  if( startSec%2048 ) {
    kdDebug() << "(K3bDeviceWrapperQIODevice) need to modify start sec." << endl;
    startSecOffset = startSec%2048;
    buffered = true;
  }

  unsigned long bufferLen = maxlen+startSecOffset;
  if( bufferLen%2048 > 0 ) {
    buffered = true;
    bufferLen = bufferLen+(2048-(bufferLen%2048));
    buffer = new unsigned char[bufferLen];
    kdDebug() << "(K3bDeviceWrapperQIODevice) using buffer of size: " << bufferLen << endl;
  }
  long read = -1;


  //
  // SOME EXPLANATION FOR THE FOLLOWING WEIRD CODE:
  // There seems to be a max value for the read buffer (somthing with DMA)
  // which I don't know how to obtain yet. So we simply split our read into
  // mutiple reads of one sector which will always work.
  //
//   d->read12Failed = d->readCdFailed = false;
//   unsigned char* currentBufPos = buffer;
//   long currentSec = startSec/2048;
//   while( currentBufPos < buffer+bufferLen ) {
//     if( !readSector( currentSec, currentBufPos ) )
//       break;

//     currentSec++;
//     currentBufPos += 2048;
//   }
//   if( currentBufPos > buffer )
//     read = currentBufPos-buffer;
//   else
//     read = -1;

  // FIXME: some drives may not support read12 but read10 which would be a better fallback
  //        but the strange thing is that the readCd command has been in the MMC since the first
  //        version while read12 came in MMC 3 and read10 in MMC 4
  if( m_device->readCd( buffer,
			bufferLen,
			0,
			false,
			startSec/2048,
 			bufferLen/2048,
			false,
			false,
			false,
			true,
			false,
			0,
			0 ) ) {
    read = maxlen;
  }
  else if( m_device->read12( buffer,
			     bufferLen,
			     startSec/2048,
			     bufferLen/2048 ) )
    read = maxlen;

  // fallback
  if( read < 0 ) {
    kdDebug() << "(K3bDeviceWrapperQIODevice) falling back to stdlib read" << endl;
    if( ::lseek( m_device->open(), startSec,SEEK_SET ) == -1 )
      kdDebug() << "(K3bDeviceWrapperQIODevice) seek failed." << endl;
    else {
      read = ::read( m_device->open(), buffer, bufferLen );
      if( read < 0 )
	kdDebug() << "(K3bDeviceWrapperQIODevice) stdlib read failed." << endl;
    }
  }

  if( buffered ) {
    if( read > 0 ) {
      ::memcpy( data, buffer+startSecOffset, read-startSecOffset );
      read -= startSecOffset;
    }
    delete [] buffer;
    kdDebug() << "(K3bDeviceWrapperQIODevice) deleted buffer." << endl;
  }

  return read;
}


bool K3bDeviceWrapperQIODevice::readSector( unsigned long sector, unsigned char* buf )
{
  bool success = false;

  if( !d->readCdFailed )
    if( m_device->readCd( buf,
			  2048,
			  2, // Mode 1
			  false,
			  sector,
			  1,
			  false,
			  false,
			  false,
			  true,
			  false,
			  0,
			  0 ) )
      success = true;
    else
      d->readCdFailed = true;

  if( !success && !d->read12Failed )
    if( m_device->read12( buf,
			  2048,
			  sector,
			  1 ) )
      success = true;
    else
      d->read12Failed = true;

  // fallback
  if( !success ) {
    kdDebug() << "(K3bDeviceWrapperQIODevice) falling back to stdlib read" << endl;
    if( ::lseek( m_device->open(), sector, SEEK_SET ) == -1 )
      kdDebug() << "(K3bDeviceWrapperQIODevice) seek failed." << endl;
    else {
      if( ::read( m_device->open(), buf, 2048 ) < 0 )
	kdDebug() << "(K3bDeviceWrapperQIODevice) stdlib read failed." << endl;
      else
	success = true;
    }
  }

  return success;
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
