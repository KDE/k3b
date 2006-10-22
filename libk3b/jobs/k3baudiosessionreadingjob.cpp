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

#include "k3baudiosessionreadingjob.h"

#include <k3bthread.h>
#include <k3btoc.h>
#include <k3bcdparanoialib.h>
#include <k3bwavefilewriter.h>
#include <k3bglobals.h>
#include <k3bdevice.h>
#include <k3bcore.h>

#include <kdebug.h>
#include <klocale.h>

#include <unistd.h>


class K3bAudioSessionReadingJob::WorkThread : public K3bThread
{
public:
  WorkThread()
    : K3bThread(),
      fd(-1),
      paranoia(0),
      waveFileWriter(0),
      paranoiaMode(0),
      retries(50),
      neverSkip(false) {
  }

  ~WorkThread() {
    delete waveFileWriter;
    delete paranoia;
  }

  void run() {
    canceled = false;

    if( !paranoia )
      paranoia = K3bCdparanoiaLib::create();

    if( !paranoia ) {
      emitInfoMessage( i18n("Could not load libcdparanoia."), K3bJob::ERROR );
      emitFinished(false);
      return;
    }

    if( toc.isEmpty() )
      toc = device->readToc();

    if( !paranoia->initParanoia( device, toc ) ) {
      emitInfoMessage( i18n("Could not open device %1").arg(device->blockDeviceName()),
		       K3bJob::ERROR );
      emitFinished(false);
      return;
    }

    if( !paranoia->initReading() ) {
      emitInfoMessage( i18n("Error while initializing audio ripping."), K3bJob::ERROR );
      emitFinished(false);    
      return;
    }

    device->block( true );

    // init settings
    paranoia->setMaxRetries( retries );
    paranoia->setParanoiaMode( paranoiaMode );
    paranoia->setNeverSkip( neverSkip );

    bool writeError = false;
    unsigned int trackNum = 1;
    unsigned int currentTrack = 0;
    unsigned long trackRead = 0;
    unsigned long totalRead = 0;
    unsigned int lastTrackPercent = 0;
    unsigned int lastTotalPercent = 0;
    bool newTrack = true;
    int status = 0;
    char* buffer = 0;
    while( !canceled && (buffer = paranoia->read( &status, &trackNum, fd == -1 /*when writing to a wav be want little endian */ )) ) {

      if( currentTrack != trackNum ) {
	emitNextTrack( trackNum, paranoia->toc().count() );
	trackRead = 0;
	lastTrackPercent = 0;

	currentTrack = trackNum;
	newTrack = true;
      }

      if( fd > 0 ) {
	if( ::write( fd, buffer, CD_FRAMESIZE_RAW ) != CD_FRAMESIZE_RAW ) {
	  kdDebug() << "(K3bAudioSessionCopyJob::WorkThread) error while writing to fd " << fd << endl;
	  writeError = true;
	  break;
	}
      }
      else {
	if( newTrack ) {
	  newTrack = false;

	  if( !waveFileWriter )
	    waveFileWriter = new K3bWaveFileWriter();

	  if( filenames.count() < currentTrack ) {
	    kdDebug() << "(K3bAudioSessionCopyJob) not enough image filenames given: " << currentTrack << endl;
	    writeError = true;
	    break;
	  }

	  if( !waveFileWriter->open( filenames[currentTrack-1] ) ) {
	    emitInfoMessage( i18n("Unable to open '%1' for writing.").arg(filenames[currentTrack-1]), K3bJob::ERROR );
	    writeError = true;
	    break;
	  }
	}

	waveFileWriter->write( buffer, 
			       CD_FRAMESIZE_RAW, 
			       K3bWaveFileWriter::LittleEndian );
      }

      trackRead++;
      totalRead++;

      unsigned int trackPercent = 100 * trackRead / toc[currentTrack-1].length().lba();
      if( trackPercent > lastTrackPercent ) {
	lastTrackPercent = trackPercent;
	emitSubPercent( lastTrackPercent );
      }
      unsigned int totalPercent = 100 * totalRead / paranoia->rippedDataLength();
      if( totalPercent > lastTotalPercent ) {
	lastTotalPercent = totalPercent;
	emitPercent( lastTotalPercent );
      }
    }

    if( waveFileWriter )
      waveFileWriter->close();

    paranoia->close();

    device->block( false );

    if( status != K3bCdparanoiaLib::S_OK ) {
      emitInfoMessage( i18n("Unrecoverable error while ripping track %1.").arg(trackNum), K3bJob::ERROR );
      emitFinished(false);
      return;
    }

    emitFinished( !writeError & !canceled );
  }


  void cancel() {
    canceled = true;
    // FIXME: add backup killing like in the audio ripping and make sure to close paranoia
  }

  bool canceled;

  int fd;
  K3bCdparanoiaLib* paranoia;
  K3bDevice::Device* device;
  K3bDevice::Toc toc;
  K3bWaveFileWriter* waveFileWriter;
  QStringList filenames;
  int paranoiaMode;
  int retries;
  bool neverSkip;
};


K3bAudioSessionReadingJob::K3bAudioSessionReadingJob( K3bJobHandler* jh, QObject* parent, const char* name )
  : K3bThreadJob( jh, parent, name )
{
  m_thread = new WorkThread();
  setThread( m_thread );
}


K3bAudioSessionReadingJob::~K3bAudioSessionReadingJob()
{
  delete m_thread;
}


void K3bAudioSessionReadingJob::setDevice( K3bDevice::Device* dev )
{
  m_thread->device = dev;
  m_thread->toc = K3bDevice::Toc();
}


void K3bAudioSessionReadingJob::setToc( const K3bDevice::Toc& toc )
{
  m_thread->toc = toc;
}


void K3bAudioSessionReadingJob::writeToFd( int fd )
{
  m_thread->fd = fd;
}

void K3bAudioSessionReadingJob::setImageNames( const QStringList& l )
{
  m_thread->filenames = l;
  m_thread->fd = -1;
}


void K3bAudioSessionReadingJob::setParanoiaMode( int m )
{
  m_thread->paranoiaMode = m;
}


void K3bAudioSessionReadingJob::setReadRetries( int r )
{
  m_thread->retries = r;
}

void K3bAudioSessionReadingJob::setNeverSkip( bool b )
{
  m_thread->neverSkip = b;
}


void K3bAudioSessionReadingJob::start()
{
  k3bcore->blockDevice( m_thread->device );
  K3bThreadJob::start();
}


void K3bAudioSessionReadingJob::cleanupJob( bool success )
{
  Q_UNUSED( success );
  k3bcore->unblockDevice( m_thread->device );
}

#include "k3baudiosessionreadingjob.moc"
