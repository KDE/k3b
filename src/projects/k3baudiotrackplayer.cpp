/*
 *
 * Copyright (C) 2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3baudiotrackplayer.h"
#include "k3baudiodoc.h"
#include "k3baudiotrack.h"
#include "k3baudioserver.h"

#include <kactionclasses.h>
#include <klocale.h>
#include <kaction.h>

#include <qslider.h>
#include <qtimer.h>
#include <qmutex.h>
#include <qtooltip.h>

#if 0
K3b::AudioTrackPlayerSeekAction::AudioTrackPlayerSeekAction( K3b::AudioTrackPlayer* player, QObject* parent )
    : K3b::WidgetFactoryAction( parent ),
      m_player( player )
{
}


K3b::AudioTrackPlayerSeekAction::~AudioTrackPlayerSeekAction()
{
}

void K3b::AudioTrackPlayerSeekAction::setValue( int v )
{
    int len = containerCount();
    for( int i = 0; i < len; ++i ) {
        QWidget* w = widget( container( i ) );
        if ( w ) {
            static_cast<QSlider*>( w )->setValue( v );
        }
        else
            kDebug() << "(K3b::AudioTrackPlayerSeekAction::setValue) no widget found for container " << container( i );
    }
}


void K3b::AudioTrackPlayerSeekAction::setMaxValue( int v )
{
    int len = containerCount();
    for( int i = 0; i < len; ++i ) {
        QWidget* w = widget( container( i ) );
        if ( w ) {
            static_cast<QSlider*>( w )->setMaxValue( v );
        }
        else
            kDebug() << "(K3b::AudioTrackPlayerSeekAction::setMaxValue) no widget found for container " << container( i );
    }
}


QWidget* K3b::AudioTrackPlayerSeekAction::createWidget( QWidget* container)
{
    QSlider* seekSlider = new QSlider( 0, 100, 1, 0, Qt::Horizontal, container );
    connect( seekSlider, SIGNAL(sliderMoved(int)), m_player, SLOT(slotSeek(int)) );
    return seekSlider;
}
#endif

class K3b::AudioTrackPlayer::Private
{
public:
  KAction* actionPlay;
  KAction* actionPause;
  KAction* actionPlayPause;
  KAction* actionStop;
  KAction* actionNext;
  KAction* actionPrev;
  //K3b::AudioTrackPlayerSeekAction* actionSeek;

  // just to handle them easily;
  KActionCollection* actionCollection;

  QTimer sliderTimer;

  // used to make sure that no seek and read operation occur in parallel
  QMutex mutex;

  bool playing;
  bool paused;
};


K3b::AudioTrackPlayer::AudioTrackPlayer( K3b::AudioDoc* doc, QObject* parent )
  : QObject( parent ),
    K3b::AudioClient(),
    m_doc( doc ),
    m_currentTrack( 0 )
{
  d = new Private;
  d->paused = false;
  d->playing = false;

  // TODO: handle the shortcuts: pass a widget to the action collection (perhaps the trackview?)
  d->actionCollection = new KActionCollection( 0, this );

  // create the actions
  // TODO: create shortcuts (is there a way to let the user change them?)
  d->actionPlay = new KAction( i18n("Play"),
			       "player_play",
			       KShortcut(),
			       this, SLOT(playPause()),
			       d->actionCollection,
			       "play" );
  d->actionPause = new KAction( i18n("Pause"),
				"player_pause",
				KShortcut(),
				this, SLOT(playPause()),
				d->actionCollection,
				"pause" );
  d->actionPlayPause = new KAction( i18n("Play/Pause"),
				    "player_play",
				    KShortcut(),
				    this, SLOT(playPause()),
				    d->actionCollection,
				    "play_pause" );

  d->actionStop = new KAction( i18n("Stop"),
			       "player_stop",
			       KShortcut(),
			       this, SLOT(stop()),
			       d->actionCollection,
			       "stop" );
  d->actionNext = new KAction( i18n("Next"),
			       "player_end",
			       KShortcut(),
			       this, SLOT(next()),
			       d->actionCollection,
			       "next" );
  d->actionPrev = new KAction( i18n("Prev"),
			       "player_start",
			       KShortcut(),
			       this, SLOT(prev()),
			       d->actionCollection,
			       "prev" );
  //d->actionSeek = new K3b::AudioTrackPlayerSeekAction( this, d->actionCollection, "seek" );

  d->actionStop->setEnabled(false);
  d->actionPause->setEnabled(false);
  d->actionNext->setEnabled(false);
  d->actionPrev->setEnabled(false);
  //d->actionSeek->setEnabled(false);

  connect( m_doc, SIGNAL(changed()),
	   this, SLOT(slotDocChanged()) );
  connect( m_doc, SIGNAL(trackChanged(K3b::AudioTrack*)),
	   this, SLOT(slotTrackChanged(K3b::AudioTrack*)) );
  connect( m_doc, SIGNAL(trackRemoved(K3b::AudioTrack*)),
	   this, SLOT(slotTrackRemoved(K3b::AudioTrack*)) );
  connect( &d->sliderTimer, SIGNAL(timeout()),
	   this, SLOT(slotUpdateSlider()) );

  // we just stop the player if the audio server has an error. K3b::MainWindow will show the error message
  // This is all very hacky and has to be improved for K3b 2.0. But then we will probably use Phonon anyway...
  connect( K3b::AudioServer::instance(), SIGNAL(error(const QString&)), this, SLOT(stop()) );

  // tooltips
  d->actionPlay->setToolTip( i18n("Play") );
  d->actionStop->setToolTip( i18n("Stop") );
  d->actionPause->setToolTip( i18n("Pause") );
  d->actionNext->setToolTip( i18n("Next") );
  d->actionPrev->setToolTip( i18n("Previous") );
}


K3b::AudioTrackPlayer::~AudioTrackPlayer()
{
  stop();
  delete d;
}


KAction* K3b::AudioTrackPlayer::action( int action ) const
{
  switch( action ) {
  case ACTION_PLAY:
    return d->actionPlay;
  case ACTION_PAUSE:
    return d->actionPause;
  case ACTION_PLAY_PAUSE:
    return d->actionPlayPause;
  case ACTION_STOP:
    return d->actionStop;
  case ACTION_NEXT:
    return d->actionNext;
  case ACTION_PREV:
    return d->actionPrev;
  //case ACTION_SEEK:
    //return d->actionSeek;
  default:
    return 0;
  }
}


void K3b::AudioTrackPlayer::playTrack( K3b::AudioTrack* track )
{
  if( track ) {
    // we show the currently playing track as a tooltip on the slider
/*
    d->actionSeek->setToolTip( i18n("Playing track %1: %2 - %3")
                               .arg(track->trackNumber())
                               .arg(track->artist())
                               .arg(track->title()) );
    d->actionSeek->setMaxValue( track->length().totalFrames() );
*/
    m_currentTrack = track;
    d->paused = true;

    d->actionNext->setEnabled( m_currentTrack->next() != 0 );
    d->actionPrev->setEnabled( m_currentTrack->prev() != 0 );

    seek(0);
    playPause();

    emit playingTrack( track );
  }
}


void K3b::AudioTrackPlayer::playPause()
{
  if( !m_currentTrack ) {
    playTrack( m_doc->firstTrack() );
  }
  else {
    if( !d->playing ) {
      seek( m_currentPosition );
      d->playing = true;
      d->actionPlayPause->setIcon( "media-playback-start" );
      d->actionPause->setEnabled(true);
      d->actionPlay->setEnabled(false);
      //d->actionSeek->setEnabled(true);
      startStreaming();
      d->sliderTimer.start(1000);
    }
    else if( d->paused ) {
      d->paused = false;
      d->actionPlayPause->setIcon( "media-playback-pause" );
      d->actionPause->setEnabled(true);
      d->actionPlay->setEnabled(false);
      startStreaming();
      d->sliderTimer.start(1000);

      emit paused( false );
    }
    else {
      d->paused = true;
      d->actionPlayPause->setIcon( "media-playback-start" );
      d->actionPause->setEnabled(false);
      d->actionPlay->setEnabled(true);
      stopStreaming();
      d->sliderTimer.stop();

      emit paused( true );
    }

    d->actionStop->setEnabled(true);
  }
}


void K3b::AudioTrackPlayer::stop()
{
  m_currentTrack = 0;
  m_currentPosition = 0;
  stopStreaming();
  d->paused = false;
  d->playing = false;
  d->actionStop->setEnabled(false);
  d->actionPause->setEnabled(false);
  d->actionPlay->setEnabled(true);
  //d->actionSeek->setEnabled(false);
  d->actionNext->setEnabled(false);
  d->actionPrev->setEnabled(false);
  d->sliderTimer.stop();

  d->actionPlayPause->setIcon( "media-playback-start" );

  emit stopped();
}


void K3b::AudioTrackPlayer::next()
{
  if( m_currentTrack && m_currentTrack->next() ) {
    playTrack( m_currentTrack->next() );
  }
}


void K3b::AudioTrackPlayer::prev()
{
  if( m_currentTrack && m_currentTrack->prev() ) {
    playTrack( m_currentTrack->prev() );
  }
}


void K3b::AudioTrackPlayer::seek( const K3b::Msf& msf )
{
  if( m_currentTrack ) {
    if( msf < m_currentTrack->length() ) {
      d->mutex.lock();
      m_currentTrack->seek( msf );
      m_currentPosition = msf;
      slotUpdateSlider();
      d->mutex.unlock();
    }
    else
      next();
  }
}


void K3b::AudioTrackPlayer::slotSeek( int frames )
{
  seek( K3b::Msf( frames ) );
}


int K3b::AudioTrackPlayer::read( char* data, int maxlen )
{
  if( m_currentTrack ) {
    d->mutex.lock();
    int len = m_currentTrack->read( data, maxlen );
    d->mutex.unlock();
    if( len > 0 ) {
      m_currentPosition += (int)( (double)len / 2352.0 + 0.5 );
    }
    else if( m_currentTrack->next() ) {
      // play the next track
      next();
      return read( data, maxlen );
    }
    else {
      stop();
      return -1; // no more tracks
    }

    return len;
  }
  else
    return -1;
}


void K3b::AudioTrackPlayer::slotTrackRemoved( K3b::AudioTrack* track )
{
  if( m_currentTrack == track ) {
    stop();
    m_currentTrack = 0;
  }
}


void K3b::AudioTrackPlayer::slotTrackChanged( K3b::AudioTrack* track )
{
/*
  if( m_currentTrack == track ) {
    d->actionSeek->setMaxValue( track->length().totalFrames() );
  }
*/
}


void K3b::AudioTrackPlayer::slotUpdateSlider()
{
  //d->actionSeek->setValue( m_currentPosition.totalFrames() );
}


void K3b::AudioTrackPlayer::slotDocChanged()
{
  // update the controls in case a new track has been added before or after
  // the current one and it has been the first or last track
  if( m_currentTrack ) {
    d->actionNext->setEnabled( m_currentTrack->next() != 0 );
    d->actionPrev->setEnabled( m_currentTrack->prev() != 0 );
  }
}

#include "k3baudiotrackplayer.moc"
