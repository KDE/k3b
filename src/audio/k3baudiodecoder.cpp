/* 
 *
 * $Id: $
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

#include <klocale.h>
#include <kdebug.h>
#include <qtimer.h>


K3bAudioDecoder::K3bAudioDecoder( K3bAudioDoc* doc, QObject* parent, const char* name )
  : K3bJob( parent, name ),
    m_doc(doc)
{
  m_currentTrack = 0;
}

K3bAudioDecoder::~K3bAudioDecoder()
{
}


void K3bAudioDecoder::start()
{
  m_currentTrackNumber = 0;
  m_canceled = false;
  m_decodedDataSize = 0;
  m_docSize = m_doc->size();
  m_suspended = false;
  m_startNewTrackWhenResuming = false;

  emit started();

  decodeNextTrack();
}


void K3bAudioDecoder::decodeNextTrack()
{
  m_startNewTrackWhenResuming = false;

  m_currentTrackNumber++;
  m_currentTrack = m_doc->at(m_currentTrackNumber-1);


  m_currentTrack->module()->disconnect( this );
  connect( m_currentTrack->module(), SIGNAL(output(const unsigned char*, int)), 
	   this, SLOT(slotModuleOutput(const unsigned char*, int)) );
  connect( m_currentTrack->module(), SIGNAL(finished(bool)),
	   this, SLOT(slotModuleFinished(bool)) );
  connect( m_currentTrack->module(), SIGNAL(percent(int)),
	   this, SLOT(slotModulePercent(int)) );

  emit nextTrack( m_currentTrackNumber, m_doc->numOfTracks() );

  m_currentTrack->module()->start( m_currentTrack );
}



void K3bAudioDecoder::resume()
{
  m_suspended = false;
  if( m_startNewTrackWhenResuming )
    decodeNextTrack();
  else if( m_currentTrack )
    QTimer::singleShot(0, m_currentTrack->module(), SLOT(resume()) );
}


void K3bAudioDecoder::cancel()
{
  m_canceled = true;

  if( m_currentTrack ) {
    m_currentTrack->module()->cancel();
    m_currentTrack->module()->disconnect( this );
  }
  emit canceled();
  emit finished(false);
}


void K3bAudioDecoder::slotModuleOutput( const unsigned char* d, int len )
{
  if( m_suspended ) {
    emit infoMessage( i18n("AudioDecoder internal error! Please report"), ERROR );
  }

  m_decodedDataSize += len;
  m_suspended = true;

  emit data( (const char*) d, len );
}


void K3bAudioDecoder::slotModuleFinished( bool success )
{
  if( !m_canceled ) {
    m_currentTrack->module()->disconnect( this );

    if( success ) {
      // check if we decoded the last track
      if( m_currentTrackNumber == m_doc->numberOfTracks() ) {
	m_currentTrack = 0;
	emit finished(true);
      }
      else { 
	if( m_suspended )
	  m_startNewTrackWhenResuming = true;
	else
	  decodeNextTrack();
      }
    }
    else {
      emit infoMessage( i18n("Error while decoding track %1").arg(m_currentTrackNumber), ERROR );
      m_currentTrack = 0;
      emit finished(false);
    }
  }
}


void K3bAudioDecoder::slotModulePercent( int p )
{
  emit subPercent( p );
  // create percent (100*m_decodedDataSize can become too big for unsigned long!)
  emit percent( (int)(100.0 * ((double)m_decodedDataSize / (double)m_docSize)) );
}


#include "k3baudiodecoder.moc"
