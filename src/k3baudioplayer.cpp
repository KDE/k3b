/***************************************************************************
                          k3baudioplayer.cpp  -  description
                             -------------------
    begin                : Sun Feb 10 2002
    copyright            : (C) 2002 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "k3baudioplayer.h"
#include "k3b.h"

#include <string.h>

#include <arts/artsflow.h>

#include <qtimer.h>



K3bAudioPlayer::K3bAudioPlayer( QObject* parent, const char* name )
  : QObject( parent, name ), m_playObject( Arts::PlayObject::null() )
{
  m_endTimer = new QTimer( this );
  connect( m_endTimer, SIGNAL(timeout()), this, SLOT(slotCheckEnd()) );
}


K3bAudioPlayer::~K3bAudioPlayer()
{
  // we remove the reference to the play object
  // if we don't do this it won't be removed and K3b will crash (not sure why)
  m_playObject = Arts::PlayObject::null();
}


int K3bAudioPlayer::state()
{
  if( !m_playObject.isNull() ) {
    switch( m_playObject.state() ) {
    case Arts::posIdle:
      return STOPPED;
    case Arts::posPlaying:
      return PLAYING;
    case Arts::posPaused:
      return PAUSED;
    }
  }

  return EMPTY;
}


void K3bAudioPlayer::play()
{
  if( !m_playObject.isNull() ) {
    if( m_playObject.state() == Arts::posIdle ) {
      // we need to recreate the playObject since otherwise it does not seem to work after it has been stopped (??)
      // and we want it to!
      Arts::PlayObjectFactory factory = Arts::Reference("global:Arts_PlayObjectFactory");
      m_playObject = factory.createPlayObject( string(m_filename.latin1()) );
    }
    if( m_playObject.state() != Arts::posPlaying ) {
      m_playObject.play();
      emit started();
      m_endTimer->start( 1000 );
    }
  }
}


void K3bAudioPlayer::stop()
{
  if( !m_playObject.isNull() ) {
    if( m_playObject.state() != Arts::posIdle ) {
      m_endTimer->stop();
      m_playObject.halt();
      emit stopped();
    }
  }
}


void K3bAudioPlayer::pause()
{
  if( !m_playObject.isNull() ) {
    if( m_playObject.state() == Arts::posPlaying ) {
      m_endTimer->stop();
      m_playObject.pause();
      emit paused();
    }
  }
}


void K3bAudioPlayer::seek( long pos )
{
  if( !m_playObject.isNull() ) {
    if( m_playObject.state() != Arts::posIdle ) {
      if( pos < 0 ) {
	m_playObject.seek( Arts::poTime() );
      }
      else if( m_playObject.overallTime().seconds < pos ) {
	m_playObject.seek( m_playObject.overallTime() );
      }
      else if( pos != m_playObject.currentTime().seconds ) {
	m_playObject.seek( Arts::poTime( pos, 0, -1, "" ) );
      }
    }
  }
}



void K3bAudioPlayer::seek( int pos )
{
  seek( (long)pos );
}


long K3bAudioPlayer::length()
{
  if( !m_playObject.isNull() ) {
    return m_playObject.overallTime().seconds;
  }
  return 0;
}


long K3bAudioPlayer::position()
{
  if( !m_playObject.isNull() ) {
    return m_playObject.currentTime().seconds;
  }
  return 0;
}


// FIXME: let my do some useful stuff!
bool K3bAudioPlayer::supportsMimetype( const QString& mimetype )
{
  if( mimetype.contains("audio") || mimetype.contains("ogg") )
    return true;
  else
    return false;
}


bool K3bAudioPlayer::playFile( const QString& filename )
{
  Arts::PlayObjectFactory factory = Arts::Reference("global:Arts_PlayObjectFactory");

  m_playObject = factory.createPlayObject( string(filename.latin1()) );
  if( m_playObject.isNull() ) {
    qDebug( "(K3bAudioPlayer) no aRts module available for: " + filename );
    m_filename = QString::null;
    return false;
  }

  qDebug("(K3bAudioPlayer) playing file: " + filename );
  m_filename = filename;

  m_playObject.play();

  emit started();
  emit started( filename );

  m_endTimer->start( 1000 );

  return true;
}


void K3bAudioPlayer::slotCheckEnd()
{
  if( !m_playObject.isNull() ) {
    if( m_playObject.state() == Arts::posIdle ) {
      m_endTimer->stop();
      emit ended();
    }
  }
}


#include "k3baudioplayer.moc"
