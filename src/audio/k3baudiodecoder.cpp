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

#include "k3baudiodecoder.h"
#include "k3baudiodoc.h"
#include "k3baudiotrack.h"
#include "input/k3baudiomodule.h"
#include <k3bthread.h>

#include <klocale.h>
#include <kdebug.h>
#include <qwaitcondition.h>

#include <unistd.h>



class K3bAudioDecoder::DecoderThread : public K3bThread
{
public:
  DecoderThread( K3bAudioDoc* doc, QObject* parent )
    : K3bThread( parent ),
      m_doc(doc),
      m_fdToWriteTo(-1),
      m_suspended(false),
      m_canceled(false) {
  }

  void cancel() {
    m_canceled = true;
    m_condResume.wakeAll();
  }

  void resume() {
    m_suspended = false;
    m_condResume.wakeAll();
  }

  void run() {
    m_canceled = false;
    kdDebug() << "(K3bAudioDecoder::DecoderThread) run()" << endl;

    int dataLen = 2352*10;
    char* data = new char[dataLen];
    int length = 0;
    bool finished = false;
    bool success = true;
    K3bAudioModule* module = 0;
    K3bAudioTrack* track = 0;
    int currentTrackNumber = 0;
    unsigned long decodedDataSize = 0;
    unsigned long decodedTrackSize = 0;
    unsigned long docSize = m_doc->size();

    while( !finished ) {
      if( m_canceled ) {
	finished = true;
	success = false;
      }
      else {
	currentTrackNumber++;
	if( currentTrackNumber > m_doc->numOfTracks() ) {
	  finished = true;
	  success = true;
	}
	else {
	  emitNextTrack( currentTrackNumber, m_doc->numOfTracks() );

	  track = m_doc->at( currentTrackNumber-1 );
	  module = track->module();
	  decodedTrackSize = 0;

	  kdDebug() << "(K3bAudioDecoder::DecoderThread) decoding track "
		    << currentTrackNumber << ": " << track->absPath() << endl;

	  if( module->initDecoding( track->absPath(), track->size() ) ) {
	    kdDebug() << "(K3bAudioDecoder::DecoderThread) module successfully initialized." << endl;

	    // now decode the track
	    while( !finished && (length = module->decode( data, dataLen )) > 0 ) {
	      if( m_canceled ) {
		finished = true;
		success = false;
	      }
	      else {
		if( m_fdToWriteTo == -1 ) {
		  m_suspended = true;

		  emitData( data, length );

		  // wait to be resumed or canceled
		  if( !m_canceled && m_suspended )
		    m_condResume.wait();
		}
		else if( ::write( m_fdToWriteTo, data, length ) == -1 ) {
		  kdError() << "(K3bAudioDecoder::DecoderThread) could not write to " << m_fdToWriteTo << endl;
		  finished = true;
		  success = false;
		}

		if( !m_canceled ) {
		  decodedDataSize += length;
		  decodedTrackSize += length;
		  emitPercent( (int)( (double)decodedDataSize/(double)docSize*100.0 ) );
		  emitSubPercent( (int)( (double)decodedTrackSize/(double)track->size()*100.0 ) );
		}
	      }
	    }

	    if( length == 0 ) {
	      // the module finished
	      kdDebug() << "(K3bAudioDecoder::DecoderThread) successfully decoded " << track->fileName() << endl;
	    }
	    else if( length < 0 ) {
	      // an error occured
	      kdError() << "(K3bAudioDecoder::DecoderThread) error while decoding " << track->fileName() << endl;
	      finished = true;
	      success = false;
	    }

	    module->cleanup();
	  }
	  else {
	    kdError() << "(K3bAudioDecoder::DecoderThread) Error while initializing AudioModule." << endl;
	    finished = true;
	    success = false;
	  }
	}
      }
    }

    delete [] data;

    if( !success )
      if( !m_canceled )
	emitInfoMessage( i18n("Decoding canceled"), ERROR );
      else
	emitInfoMessage( i18n("Error while decoding track %1").arg(currentTrackNumber), ERROR );
    emitFinished( success );
  }

  void writeToFd( int fd ) {
    m_fdToWriteTo = fd;
  }

private:
  K3bAudioDoc* m_doc;
  int m_fdToWriteTo;
  bool m_suspended;
  bool m_canceled;

  QWaitCondition m_condResume;
};



K3bAudioDecoder::K3bAudioDecoder( K3bAudioDoc* doc, QObject* parent, const char* name )
  : K3bThreadJob( parent, name )
{
  m_thread = new DecoderThread( doc, this );
  setThread( m_thread );
}

K3bAudioDecoder::~K3bAudioDecoder()
{
  delete m_thread;
}


void K3bAudioDecoder::resume()
{
  m_thread->resume();
}


void K3bAudioDecoder::writeToFd( int fd )
{
  m_thread->writeToFd(fd);
}

#include "k3baudiodecoder.moc"
