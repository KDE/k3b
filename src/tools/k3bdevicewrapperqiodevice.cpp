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
#include <k3bdeviceglobals.h>

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

  bool resetErrorRecoveryParametersOnClose;
  unsigned char savedErrorRecoveryParameters;
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
  if( m_device->open() != -1 ) {
    unsigned char* data;
    int dataLen;
    if( m_device->modeSense( &data, dataLen, 0x01 ) ) {
      kdDebug() << "(K3bDeviceWrapperQIODevice) ERROR RECOVERY PARAMETERS MODE PAGE:" << endl;
      K3bCdDevice::debugBitfield( &data[2], 1 );

      d->savedErrorRecoveryParameters = data[2];

      // Enable error correction
      data[2] = 0x20;//(1<<7|1<<6|1<<3);

      K3bCdDevice::debugBitfield( &data[2], 1 );

      if( m_device->modeSelect( data, dataLen, true, false ) ) {
	d->resetErrorRecoveryParametersOnClose = true;
      }
      else {
	kdDebug() << "(K3bDeviceWrapperQIODevice) Unable to set ERROR RECOVERY PARAMETERS." << endl;
	d->resetErrorRecoveryParametersOnClose = false;
      }
    }

    return true;
  }
  else
    return false;
}


void K3bDeviceWrapperQIODevice::close()
{
  if( m_device->isOpen() ) {
    if( d->resetErrorRecoveryParametersOnClose ) {
      unsigned char* data;
      int dataLen;
      if( m_device->modeSense( &data, dataLen, 0x01 ) ) {
	kdDebug() << "(K3bDeviceWrapperQIODevice) ERROR RECOVERY PARAMETERS MODE PAGE:" << endl;
	K3bCdDevice::debugBitfield( &data[2], 1 );
	
	data[2] = d->savedErrorRecoveryParameters;
	if( !m_device->modeSelect( data, dataLen, true, false ) )
	  kdDebug() << "(K3bDeviceWrapperQIODevice) Unable to reset ERROR RECOVERY PARAMETERS." << endl;
      }
    }
    
    m_device->close();
  }
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

//   kdDebug() << "(K3bDeviceWrapperQIODevice) readBlock( " << (void*)data << ", " << maxlen << " ) at position " << d->pos << endl;
//   kdDebug() << "(K3bDeviceWrapperQIODevice) readCd from " << (d->start+d->pos)/2048 << endl;

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
  if( startSec%2048 || bufferLen%2048 ) {
    buffered = true;
    bufferLen = bufferLen+(2048-(bufferLen%2048));
    buffer = new unsigned char[bufferLen];
    //    kdDebug() << "(K3bDeviceWrapperQIODevice) using buffer of size: " << bufferLen << endl;
  }
  long read = -1;

  int retries = 10;  // TODO: no fixed value
  while( retries && !m_device->read10( buffer,
				       bufferLen,
				       startSec/2048,
				       bufferLen/2048 ) )
    retries--;

  if( retries > 0 )
    read = maxlen;

  
  // fallback
  if( read < 0 ) {
    kdDebug() << "(K3bDeviceWrapperQIODevice) falling back to stdlib read" << endl;
    if( ::lseek( m_device->open(), startSec, SEEK_SET ) == -1 )
      kdDebug() << "(K3bDeviceWrapperQIODevice) seek failed." << endl;
    else {
      read = ::read( m_device->open(), buffer, bufferLen );
      if( read < 0 )
	kdDebug() << "(K3bDeviceWrapperQIODevice) stdlib read failed." << endl;
    }
  }

  if( buffered ) {
    if( read > 0 )
      ::memcpy( data, buffer+startSecOffset, read );
    delete [] buffer;
    //    kdDebug() << "(K3bDeviceWrapperQIODevice) deleted buffer." << endl;
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
