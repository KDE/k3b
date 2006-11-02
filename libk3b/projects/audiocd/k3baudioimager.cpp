/* 
 *
 * $Id$
 * Copyright (C) 2004 Sebastian Trueg <trueg@k3b.org>
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

#include "k3baudioimager.h"
#include "k3baudiodoc.h"
#include "k3baudiotrack.h"
#include "k3baudiodatasource.h"

#include <k3bthread.h>
#include <k3bwavefilewriter.h>

#include <klocale.h>
#include <kdebug.h>

#include <qfile.h>

#include <unistd.h>


class K3bAudioImager::WorkThread : public K3bThread
{
public:
  WorkThread( K3bAudioDoc* doc );

  void run();

  void cancel();

  bool m_canceled;
  int m_fd;
  QStringList m_imageNames;
  K3bAudioImager::ErrorType lastError;

private:
  K3bAudioDoc* m_doc;
};


K3bAudioImager::WorkThread::WorkThread( K3bAudioDoc* doc )
  : K3bThread(),
    m_canceled(false),
    m_fd(-1),
    m_doc(doc)
{
}


void K3bAudioImager::WorkThread::run()
{
  m_canceled = false;

  emitStarted();

  lastError = K3bAudioImager::ERROR_UNKNOWN;

  //
  // 
  //
  QStringList::iterator imageFileIt = m_imageNames.begin();
  K3bWaveFileWriter waveFileWriter;

  K3bAudioTrack* track = m_doc->firstTrack();
  int trackNumber = 1;
  unsigned long long totalSize = m_doc->length().audioBytes();
  unsigned long long totalRead = 0;
  char buffer[2352 * 10];

  while( track ) {

    emitNextTrack( trackNumber, m_doc->numOfTracks() );

    //
    // Seek to the beginning of the track
    //
    if( !track->seek(0) ) {
      emitInfoMessage( i18n("Unable to seek in track %1.").arg(trackNumber), K3bJob::ERROR );
      emitFinished(false);
      return;
    }

    //
    // Initialize the reading
    //
    int read = 0;
    unsigned long long trackRead = 0;

    //
    // Create the image file
    //
    if( m_fd == -1 ) {
      if( !waveFileWriter.open( *imageFileIt ) ) {
	emitInfoMessage( i18n("Could not open %1 for writing").arg(*imageFileIt), K3bJob::ERROR );
	emitFinished(false);
	return;
      }
    }

    //
    // Read data from the track
    //
    while( (read = track->read( buffer, sizeof(buffer) )) > 0 ) {
      if( m_fd == -1 ) {
	waveFileWriter.write( buffer, read, K3bWaveFileWriter::BigEndian );
      }
      else {
	if( ::write( m_fd, reinterpret_cast<void*>(buffer), read ) != read ) {
	  kdDebug() << "(K3bAudioImager::WorkThread) writing to fd " << m_fd << " failed." << endl;
	  lastError = K3bAudioImager::ERROR_FD_WRITE;
	  emitFinished(false);
	  return;
	}
      }

      if( m_canceled ) {
	emitCanceled();
	emitFinished(false);
	return;
      }

      //
      // Emit progress
      //
      totalRead += read;
      trackRead += read;
	
      emitSubPercent( 100*trackRead/track->length().audioBytes() );
      emitPercent( 100*totalRead/totalSize );
      emitProcessedSubSize( trackRead/1024/1024, track->length().audioBytes()/1024/1024 );
      emitProcessedSize( totalRead/1024/1024, totalSize/1024/1024 );
    }

    if( read < 0 ) {
      emitInfoMessage( i18n("Error while decoding track %1.").arg(trackNumber), K3bJob::ERROR );
      kdDebug() << "(K3bAudioImager::WorkThread) read error on track " << trackNumber
		<< " at pos " << K3b::Msf(trackRead/2352) << endl;
      lastError = K3bAudioImager::ERROR_DECODING_TRACK;
      emitFinished(false);
      return;
    }

    track = track->next();
    trackNumber++;
    imageFileIt++;
  }

  emitFinished(true);
}


void K3bAudioImager::WorkThread::cancel()
{
  m_canceled = true;
}




K3bAudioImager::K3bAudioImager( K3bAudioDoc* doc, K3bJobHandler* jh, QObject* parent, const char* name )
  : K3bThreadJob( jh, parent, name ),
    m_doc(doc)
{
  m_thread = new WorkThread(doc);
  setThread( m_thread );
}


K3bAudioImager::~K3bAudioImager()
{
  delete m_thread;
}


void K3bAudioImager::writeToFd( int fd )
{
  m_thread->m_fd = fd;
}


void K3bAudioImager::setImageFilenames( const QStringList& p )
{
  m_thread->m_imageNames = p;
  m_thread->m_fd = -1;
}


K3bAudioImager::ErrorType K3bAudioImager::lastErrorType() const
{
  return m_thread->lastError;
}

#include "k3baudioimager.moc"
