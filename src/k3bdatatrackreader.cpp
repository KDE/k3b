/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bdatatrackreader.h"

#include <k3bdevice.h>
#include <k3btrack.h>
#include <k3bthread.h>

#include <klocale.h>
#include <kdebug.h>

#include <qfile.h>

#include <unistd.h>



// FIXME: determine max DMA buffer size
static int s_bufferSizeSectors = 10;


class K3bDataTrackReader::WorkThread : public K3bThread
{
public:
  WorkThread()
    : K3bThread(),
      m_canceled(false),
      m_ignoreReadErrors(false),
      m_retries(10),
      m_device(0),
      m_fd(-1) {
  }

  void run() {
    m_canceled = false;

    emitStarted();

    if( m_device->open() < 0 ) {
      emitInfoMessage( i18n("Could not open device %1").arg(m_device->blockDeviceName()), K3bJob::ERROR );
      emitFinished(false);
      return;
    }

    // 1. determine sector size by checking the first sectors mode
    //    if impossible or MODE2 (mode2 formless) finish(false)

    m_sectorSize = 0;
    switch( m_device->getDataMode( m_firstSector ) ) {
    case K3bCdDevice::Track::MODE1:
    case K3bCdDevice::Track::DVD:
      m_sectorSize = 2048;
      break;
    case K3bCdDevice::Track::XA_FORM1:
      m_sectorSize = 2056;
      break;
    case K3bCdDevice::Track::XA_FORM2:
      m_sectorSize = 2332;
      break;
    case K3bCdDevice::Track::MODE2:
      emitInfoMessage( i18n("No support for reading formless Mode2 sectors."), K3bJob::ERROR );
    default:
      emitInfoMessage( i18n("Unsupported sector type."), K3bJob::ERROR );
      m_device->close();
      emitFinished(false);
      return;
    }
    
    emitInfoMessage( i18n("Reading with sector size %1.").arg(m_sectorSize), K3bJob::INFO );
    kdDebug() << "(K3bDataTrackReader::WorkThread) reading " << (m_lastSector.lba() - m_firstSector.lba() + 1)
	      << " sectors with sector size: " << m_sectorSize << endl;

    QFile file;
    if( m_fd == -1 ) {
      file.setName( m_imagePath );
      if( !file.open( IO_WriteOnly ) ) {
	m_device->close();
	emitInfoMessage( i18n("Unable to open '%1' for writing.").arg(m_imagePath), K3bJob::ERROR );
	emitFinished( false );
	return;
      }
    }

    // 2. get it on
    K3b::Msf currentSector = m_firstSector;
    bool writeError = false;
    bool readError = false;
    int lastPercent = 0;
    unsigned long lastReadMb = 0;
    int bufferLen = s_bufferSizeSectors*m_sectorSize;
    unsigned char* buffer = new unsigned char[bufferLen];
    while( !m_canceled && currentSector <= m_lastSector ) {

      int readSectors = QMIN( bufferLen/m_sectorSize, m_lastSector.lba()-currentSector.lba()+1 );

      if( !read( buffer, 
		 currentSector.lba(), 
		 readSectors ) ) {
	if( !retryRead( buffer,
			currentSector.lba(), 
			readSectors ) ) {
	  readError = true;
	  break;
	}
      }

      int readBytes = readSectors * m_sectorSize;

      if( m_fd != -1 ) {
	if( ::write( m_fd, reinterpret_cast<void*>(buffer), readBytes ) != readBytes ) {
	  kdDebug() << "(K3bDataTrackReader::WorkThread) error while writing to fd " << m_fd 
		    << " current sector: " << (currentSector.lba()-m_firstSector.lba()) << endl;
	  writeError = true;
	  break;
	}
      }
      else {
	if( file.writeBlock( reinterpret_cast<char*>(buffer), readBytes ) != readBytes ) {
	  kdDebug() << "(K3bDataTrackReader::WorkThread) error while writing to file " << m_imagePath
		    << " current sector: " << (currentSector.lba()-m_firstSector.lba()) << endl;
	  writeError = true;
	  break;
	}
      }
      
      int percent = 100 * (currentSector.lba() - m_firstSector.lba() + 1 ) / (m_lastSector.lba() - m_firstSector.lba() + 1 );
      if( percent > lastPercent ) {
	lastPercent = percent;
	emitPercent( percent );
      }

      unsigned long readMb = (currentSector.lba() - m_firstSector.lba() + 1) / 512;
      if( readMb > lastReadMb ) {
	lastReadMb = readMb;
	emitProcessedSize( readMb, ( m_lastSector.lba() - m_firstSector.lba() + 1 ) / 512 );
      }
      
      currentSector += readSectors;
    }

    // cleanup
    m_device->close();
    delete [] buffer;

    if( m_canceled )
      emitCanceled();

    emitFinished( !m_canceled && !writeError && !readError );
  }


  bool read( unsigned char* buffer, unsigned long sector, unsigned int len ) {
    if( m_sectorSize == 2048 )
      return m_device->read10( buffer, len*2048, sector, len );
    else
      return m_device->readCd( buffer, 
			       len*m_sectorSize,
			       0,     // all sector types
			       false, // no dap
			       sector,
			       len,
			       false, // no sync
			       false, // no header
			       true,  // subheader
			       true,  // user data
			       false, // no edc/ecc
			       0,     // no c2 error info... FIXME: should we check this??
			       0      // no subchannel data
			       );
  }


  // here we read every single sector for itself to find the troubleing ones
  bool retryRead( unsigned char* buffer, unsigned long startSector, unsigned int len ) {

    emitInfoMessage( i18n("Problem while reading. Retrying from sector %1.").arg(startSector), K3bJob::WARNING );

    bool success = false;
    int i = 0;
    for( unsigned long sector = startSector; sector < startSector+len; ++sector ) {
      int retry = m_retries;
      while( !m_canceled && retry && !(success = read( &buffer[i], sector, 1 )) )
	--retry;

      if( m_canceled )
	return false;

      if( !success ) {
	emitInfoMessage( i18n("Error while reading sector %1.").arg(sector), K3bJob::ERROR );
	if( m_ignoreReadErrors ) {
	  kdDebug() << "(K3bDataTrackReader::WorkThread) ignoring read error in sector " << sector << endl;
	  ::memset( &buffer[i], 0, 1 );
	  success = true;
	}
	else
	  break;
      }

      ++i;
    }

    return success;
  }


  void cancel() {
    m_canceled = true;
  }

  bool m_canceled;
  bool m_ignoreReadErrors;
  int m_retries;
  K3bCdDevice::CdDevice* m_device;
  K3b::Msf m_firstSector;
  K3b::Msf m_lastSector;
  int m_fd;
  QString m_imagePath;
  int m_sectorSize;
};


K3bDataTrackReader::K3bDataTrackReader( QObject* parent, const char* name )
  : K3bThreadJob( parent, name )
{
  m_thread = new WorkThread();
  setThread( m_thread );
}


K3bDataTrackReader::~K3bDataTrackReader()
{
  delete m_thread;
}


void K3bDataTrackReader::setDevice( K3bCdDevice::CdDevice* dev )
{
  m_thread->m_device = dev;
}


void K3bDataTrackReader::setSectorRange( const K3b::Msf& start, const K3b::Msf& end )
{
  m_thread->m_firstSector = start;
  m_thread->m_lastSector = end;
}


void K3bDataTrackReader::setRetries( int r )
{
  m_thread->m_retries = r;
}


void K3bDataTrackReader::setIgnoreErrors( bool b )
{
  m_thread->m_ignoreReadErrors = b;
}


void K3bDataTrackReader::writeToFd( int fd )
{
  m_thread->m_fd = fd;
}


void K3bDataTrackReader::setImagePath( const QString& p )
{
  m_thread->m_imagePath = p;
  m_thread->m_fd = -1;
}
