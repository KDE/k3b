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


#include "k3baudiomodule.h"
#include "../k3baudiotrack.h"

#include <qtimer.h>


K3bAudioModule::K3bAudioModule( QObject* parent, const char* name )
  : QObject( parent, name )
{
  m_track = 0;
  m_currentlyAnalysedTrack = 0;
  connect( this, SIGNAL(trackAnalysed(K3bAudioTrack*)), this, SLOT(slotAnalysingFinished(K3bAudioTrack*)) );
}


K3bAudioModule::~K3bAudioModule()
{
}


void K3bAudioModule::start( K3bAudioTrack* track )
{
  m_track = track;
  QTimer::singleShot( 0, this, SLOT(startDecoding()) );
}


void K3bAudioModule::resume()
{
  slotConsumerReady();
}


void K3bAudioModule::addTrackToAnalyse( K3bAudioTrack* track )
{
  if( m_tracksToAnalyse.containsRef( track ) <= 0 ) {
    m_tracksToAnalyse.append( track );

    if( m_currentlyAnalysedTrack == 0 ) {
      m_currentlyAnalysedTrack = m_tracksToAnalyse.first();
      m_track = m_currentlyAnalysedTrack;
      analyseTrack();
    }
  }
}


void K3bAudioModule::removeTrackToAnalyse( K3bAudioTrack* track )
{
  // stop eventually ongoing analysing
  if( m_currentlyAnalysedTrack == track ) {
    stopAnalysingTrack();
  }
  
  slotAnalysingFinished( track );
}


void K3bAudioModule::slotAnalysingFinished( K3bAudioTrack* track )
{
  m_tracksToAnalyse.removeRef( track );
  m_currentlyAnalysedTrack = 0;
  m_track = 0;

  if( !m_tracksToAnalyse.isEmpty() ) {
    m_currentlyAnalysedTrack = m_tracksToAnalyse.first();
    m_track = m_currentlyAnalysedTrack;
    analyseTrack();
  }
}

bool K3bAudioModule::allTracksAnalysed()
{
  return m_tracksToAnalyse.isEmpty(); 
}


#include "k3baudiomodule.moc"
