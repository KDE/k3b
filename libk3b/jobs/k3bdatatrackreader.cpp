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

#include <k3blibdvdcss.h>
#include <k3bdevice.h>
#include <k3bdeviceglobals.h>
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
      m_noCorrection(false),
      m_retries(10),
      m_device(0),
      m_fd(-1),
      m_libcss(0) {
  }

  ~WorkThread() {
    delete m_libcss;
  }

  void run() {
    m_canceled = false;

    emitStarted();

    if( !m_device->open() ) {
      emitInfoMessage( i18n("Could not open device %1").arg(m_device->blockDeviceName()), K3bJob::ERROR );
      emitFinished(false);
      return;
    }

    // 1. determine sector size by checking the first sectors mode
    //    if impossible or MODE2 (mode2 formless) finish(false)

    m_useLibdvdcss = false;
    m_usedSectorSize = m_sectorSize;
    if( m_device->isDVD() ) {
      m_usedSectorSize = MODE1;

      //
      // In case of an encrypted VideoDVD we read with libdvdcss which takes care of decrypting the vobs
      //
      if( m_device->copyrightProtectionSystemType() > 0 ) {
	
	// close the device for libdvdcss
	m_device->close();
	
	kdDebug() << "(K3bDataTrackReader::WorkThread) found encrypted dvd. using libdvdcss." << endl;
	
	// open the libdvdcss stuff
	if( !m_libcss )
	  m_libcss = K3bLibDvdCss::create();
	if( !m_libcss ) {
	  emitInfoMessage( i18n("Unable to open libdvdcss."), K3bJob::ERROR );
	  emitFinished(false);
	  return;
	}
	
	if( !m_libcss->open(m_device) ) {
	  emitInfoMessage( i18n("Could not open device %1").arg(m_device->blockDeviceName()), K3bJob::ERROR );
	  emitFinished(false);
	  return;
	}

	emitInfoMessage( i18n("Retrieving all CSS keys. This might take a while."), K3bJob::INFO );
	if( !m_libcss->crackAllKeys() ) {
	  m_libcss->close();
	  emitInfoMessage( i18n("Failed to retrieve all CSS keys."), K3bJob::ERROR );
	  emitInfoMessage( i18n("Video DVD decryption failed."), K3bJob::ERROR );
	  emitFinished(false);
	  return;
	}

	m_useLibdvdcss = true;
      }
    }
    else {
      if( m_usedSectorSize == AUTO ) {
	switch( m_device->getDataMode( m_firstSector ) ) {
	case K3bDevice::Track::MODE1:
	case K3bDevice::Track::DVD:
	  m_usedSectorSize = MODE1;
	  break;
	case K3bDevice::Track::XA_FORM1:
	  m_usedSectorSize = MODE2FORM1;
	  break;
	case K3bDevice::Track::XA_FORM2:
	  m_usedSectorSize = MODE2FORM2;
	  break;
	case K3bDevice::Track::MODE2:
	  emitInfoMessage( i18n("No support for reading formless Mode2 sectors."), K3bJob::ERROR );
	default:
	  emitInfoMessage( i18n("Unsupported sector type."), K3bJob::ERROR );
	  m_device->close();
	  emitFinished(false);
	  return;
	}
      }
    }

    emitInfoMessage( i18n("Reading with sector size %1.").arg(m_usedSectorSize), K3bJob::INFO );
    kdDebug() << "(K3bDataTrackReader::WorkThread) reading " << (m_lastSector.lba() - m_firstSector.lba() + 1)
	      << " sectors with sector size: " << m_usedSectorSize << endl;

    QFile file;
    if( m_fd == -1 ) {
      file.setName( m_imagePath );
      if( !file.open( IO_WriteOnly ) ) {
	m_device->close();
	if( m_useLibdvdcss )	
	  m_libcss->close();
	emitInfoMessage( i18n("Unable to open '%1' for writing.").arg(m_imagePath), K3bJob::ERROR );
	emitFinished( false );
	return;
      }
    }

    m_device->block( true );

    //
    // set the error recovery mode to 0x21 or 0x20 depending on m_ignoreReadErrors
    // TODO: should we also set RC=1 in m_ignoreReadErrors mode (0x11 because TB is ignored)
    //
    setErrorRecovery( m_device, m_noCorrection ? 0x21 : 0x20 );

    //
    // Let the drive determine the optimal reading speed
    //
    m_device->setSpeed( 0xffff, 0xffff );

    s_bufferSizeSectors = 128;
    unsigned char* buffer = new unsigned char[m_usedSectorSize*s_bufferSizeSectors];
    while( read( buffer, m_firstSector.lba(), s_bufferSizeSectors ) < 0 ) {
      kdDebug() << "(K3bDataTrackReader) determine max read sectors: "
		<< s_bufferSizeSectors << " too high." << endl;
      s_bufferSizeSectors--;
    }
    kdDebug() << "(K3bDataTrackReader) determine max read sectors: " 
	      << s_bufferSizeSectors << " is max." << endl;

    //    s_bufferSizeSectors = K3bDevice::determineMaxReadingBufferSize( m_device, m_firstSector );
    if( s_bufferSizeSectors <= 0 ) {
      emitInfoMessage( i18n("Error while reading sector %1.").arg(m_firstSector.lba()), K3bJob::ERROR );
      emitFinished(false);
      m_device->block( false );
      return;
    }

    kdDebug() << "(K3bDataTrackReader) using buffer size of " << s_bufferSizeSectors << " blocks." << endl;

    // 2. get it on
    K3b::Msf currentSector = m_firstSector;
    m_nextReadSector = 0;
    m_errorSectorCount = 0;
    bool writeError = false;
    bool readError = false;
    int lastPercent = 0;
    unsigned long lastReadMb = 0;
    int bufferLen = s_bufferSizeSectors*m_usedSectorSize;
    while( !m_canceled && currentSector <= m_lastSector ) {

      int maxReadSectors = QMIN( bufferLen/m_usedSectorSize, m_lastSector.lba()-currentSector.lba()+1 );

      int readSectors = read( buffer, 
			      currentSector.lba(), 
			      maxReadSectors );
      if( readSectors < 0 ) {
	if( !retryRead( buffer,
			currentSector.lba(), 
			maxReadSectors ) ) {
	  readError = true;
	  break;
	}
	else
	  readSectors = maxReadSectors;
      }

      int readBytes = readSectors * m_usedSectorSize;

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

      currentSector += readSectors;

      int percent = 100 * (currentSector.lba() - m_firstSector.lba() + 1 ) / 
	(m_lastSector.lba() - m_firstSector.lba() + 1 );

      if( percent > lastPercent ) {
	lastPercent = percent;
	emitPercent( percent );
      }

      unsigned long readMb = (currentSector.lba() - m_firstSector.lba() + 1) / 512;
      if( readMb > lastReadMb ) {
	lastReadMb = readMb;
	emitProcessedSize( readMb, ( m_lastSector.lba() - m_firstSector.lba() + 1 ) / 512 );
      }
    }

    if( m_errorSectorCount > 0 )
      emitInfoMessage( i18n("Ignored %n erroneous sector.", "Ignored a total of %n erroneous sectors.", m_errorSectorCount ),
		       K3bJob::ERROR );

    // reset the error recovery mode
    setErrorRecovery( m_device, m_oldErrorRecoveryMode );

    m_device->block( false );

    // cleanup
    if( m_useLibdvdcss )
      m_libcss->close();
    m_device->close();
    delete [] buffer;

    if( m_canceled )
      emitCanceled();

    emitFinished( !m_canceled && !writeError && !readError );
  }


  int read( unsigned char* buffer, unsigned long sector, unsigned int len ) {

    //
    // Encrypted DVD reading with libdvdcss
    //
    if( m_useLibdvdcss ) {
      return m_libcss->readWrapped( reinterpret_cast<void*>(buffer), sector, len );
    }

    //
    // Standard reading
    //
    else {
      bool success = false;
      //      setErrorRecovery( m_device, m_ignoreReadErrors ? 0x21 : 0x20 );
      if( m_usedSectorSize == 2048 )
	success = m_device->read10( buffer, len*2048, sector, len );
      else
	success = m_device->readCd( buffer, 
				    len*m_usedSectorSize,
				    0,     // all sector types
				    false, // no dap
				    sector,
				    len,
				    false, // no sync
				    false, // no header
				    m_usedSectorSize != MODE1,  // subheader
				    true,  // user data
				    false, // no edc/ecc
				    0,     // no c2 error info... FIXME: should we check this??
				    0      // no subchannel data
				    );

      if( success )
	return len;
      else
	return -1;
    }
  }


  // here we read every single sector for itself to find the troubleing ones
  bool retryRead( unsigned char* buffer, unsigned long startSector, unsigned int len ) {

    emitInfoMessage( i18n("Problem while reading. Retrying from sector %1.").arg(startSector), K3bJob::WARNING );

    int sectorsRead = -1;
    bool success = true;
    int i = 0;
    for( unsigned long sector = startSector; sector < startSector+len; ++sector ) {
      int retry = m_retries;
      while( !m_canceled && retry && (sectorsRead = read( &buffer[i], sector, 1 )) < 0 )
	--retry;

      success = ( sectorsRead > 0 );

      if( m_canceled )
	return false;

      if( !success ) {
	if( m_ignoreReadErrors ) {
	  emitInfoMessage( i18n("Ignoring read error in sector %1.").arg(sector), K3bJob::ERROR );
	  ++m_errorSectorCount;
	  //	  ::memset( &buffer[i], 0, 1 );
	  success = true;
	}
	else {
	  emitInfoMessage( i18n("Error while reading sector %1.").arg(sector), K3bJob::ERROR );
	  break;
	}
      }

      ++i;
    }

    return success;
  }


  bool setErrorRecovery( K3bDevice::Device* dev, int code ) {
    unsigned char* data = 0;
    unsigned int dataLen = 0;
    if( !dev->modeSense( &data, dataLen, 0x01 ) )
      return false;
    
    // in MMC1 the page has 8 bytes (12 in MMC4 but we only need the first 3 anyway)
    if( dataLen < 8+8 ) {
      kdDebug() << "(K3bDataTrackReader) modepage 0x01 data too small: " << dataLen << endl;
      delete [] data;
      return false;
    }

    m_oldErrorRecoveryMode = data[8+2];
    data[8+2] = code;

    if( m_oldErrorRecoveryMode != code )
      kdDebug() << "(K3bDataTrackReader) changing data recovery mode from " << m_oldErrorRecoveryMode << " to " << code << endl;

    bool success = dev->modeSelect( data, dataLen, true, false );

    delete [] data;

    return success;
  }


  void cancel() {
    m_canceled = true;
  }

  bool m_canceled;
  bool m_ignoreReadErrors;
  bool m_noCorrection;
  int m_retries;
  K3bDevice::Device* m_device;
  K3b::Msf m_firstSector;
  K3b::Msf m_lastSector;
  K3b::Msf m_nextReadSector;
  int m_fd;
  QString m_imagePath;
  int m_sectorSize;
  bool m_useLibdvdcss;
  K3bLibDvdCss* m_libcss;

  int m_oldErrorRecoveryMode;

  int m_errorSectorCount;

private:
  int m_usedSectorSize;
};


K3bDataTrackReader::K3bDataTrackReader( K3bJobHandler* jh, QObject* parent, const char* name )
  : K3bThreadJob( jh, parent, name )
{
  m_thread = new WorkThread();
  setThread( m_thread );
}


K3bDataTrackReader::~K3bDataTrackReader()
{
  delete m_thread;
}


void K3bDataTrackReader::setDevice( K3bDevice::Device* dev )
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


void K3bDataTrackReader::setNoCorrection( bool b )
{
  m_thread->m_noCorrection = b;
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


void K3bDataTrackReader::setSectorSize( SectorSize size )
{
  m_thread->m_sectorSize = size;
}
