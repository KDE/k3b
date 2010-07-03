/*
 *
 * Copyright (C) 2004 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
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
#include "k3baction.h"
#include "k3baudiodoc.h"
#include "k3baudiodocreader.h"
#include "k3baudiotrack.h"
#include "k3baudiotrackreader.h"

#include <KAction>
#include <KActionCollection>
#include <KLocale>

#include <QAudioDeviceInfo>
#include <QAudioFormat>
#include <QAudioOutput>
#include <QSlider>
#include <QToolTip>
#include <QWidgetAction>


namespace K3b {

namespace {

class AudioTrackPlayerSeekAction : public QWidgetAction
{
public:
    AudioTrackPlayerSeekAction( AudioTrackPlayer* player, QObject* parent );
    ~AudioTrackPlayerSeekAction();

    void setValue( int value );
    void setMaximum( int value );

protected:
    virtual QWidget* createWidget( QWidget* parent );

private:
    AudioTrackPlayer* m_player;
};


AudioTrackPlayerSeekAction::AudioTrackPlayerSeekAction( AudioTrackPlayer* player, QObject* parent )
    : QWidgetAction( parent ),
      m_player( player )
{
}


AudioTrackPlayerSeekAction::~AudioTrackPlayerSeekAction()
{
}


void AudioTrackPlayerSeekAction::setValue( int value )
{
    Q_FOREACH( QWidget* widget, createdWidgets() ) {
        if( QSlider* slider = qobject_cast<QSlider*>( widget ) ) {
            slider->setValue( value );
        }
    }
}


void AudioTrackPlayerSeekAction::setMaximum( int value )
{
    Q_FOREACH( QWidget* widget, createdWidgets() ) {
        if( QSlider* slider = qobject_cast<QSlider*>( widget ) ) {
            slider->setMaximum( value );
        }
    }
}


QWidget* AudioTrackPlayerSeekAction::createWidget( QWidget* container)
{
    QSlider* slider = new QSlider( container );
    slider->setRange( 0, 100 );
    slider->setOrientation( Qt::Horizontal );
    connect( slider, SIGNAL(sliderMoved(int)), m_player, SLOT(slotSeek(int)) );
    return slider;
}

} // namespace


class AudioTrackPlayer::Private
{
public:
    AudioDoc* doc;
    AudioDocReader* audioDocReader;
    QAudioOutput* audioOutput;

    KAction* actionPlay;
    KAction* actionPause;
    KAction* actionPlayPause;
    KAction* actionStop;
    KAction* actionNext;
    KAction* actionPrevious;
    AudioTrackPlayerSeekAction* actionSeek;
};


AudioTrackPlayer::AudioTrackPlayer( AudioDoc* doc, KActionCollection* actionCollection, QObject* parent )
  : QObject( parent ),
    d( new Private )
{
    d->doc = doc;
    d->audioDocReader = new AudioDocReader( *doc, this );

    QAudioFormat audioFormat;
    audioFormat.setFrequency( 44100 );
    audioFormat.setChannels( 2 );
    audioFormat.setSampleSize( 16 );
    audioFormat.setSampleType( QAudioFormat::SignedInt );
    audioFormat.setCodec( "audio/pcm" );
    audioFormat.setByteOrder( QAudioFormat::BigEndian );
    d->audioOutput = new QAudioOutput( QAudioDeviceInfo::defaultOutputDevice(), audioFormat, this );

    // create the actions
    // TODO: create shortcuts (is there a way to let the user change them?)
    d->actionPlay = createAction( this, i18n("Play"),
                                  "media-playback-start",
                                  QKeySequence(),
                                  this, SLOT(playPause()),
                                  actionCollection,
                                  "player_play" );
    d->actionPause = createAction( this, i18n("Pause"),
                                   "media-playback-pause",
                                   QKeySequence(),
                                   this, SLOT(playPause()),
                                   actionCollection,
                                   "player_pause" );
    d->actionPlayPause = createAction( this, i18n("Play/Pause"),
                                       "media-playback-start",
                                       QKeySequence(),
                                       this, SLOT(playPause()),
                                       actionCollection,
                                       "player_play_pause" );

    d->actionStop = createAction( this, i18n("Stop"),
                                  "media-playback-stop",
                                  QKeySequence(),
                                  this, SLOT(stop()),
                                  actionCollection,
                                  "player_stop" );
    d->actionNext = createAction( this, i18n("Next"),
                                  "media-skip-forward",
                                  QKeySequence(),
                                  this, SLOT(next()),
                                  actionCollection,
                                  "player_next" );
    d->actionPrevious = createAction( this, i18n("Previous"),
                                      "media-skip-backward",
                                      QKeySequence(),
                                      this, SLOT(previous()),
                                      actionCollection,
                                      "player_previous" );
    d->actionSeek = new AudioTrackPlayerSeekAction( this, actionCollection );
    if( actionCollection ) {
        actionCollection->addAction( "player_seek", d->actionSeek );
    }

    d->actionStop->setEnabled( false );
    d->actionPause->setEnabled( false );
    d->actionNext->setEnabled( false );
    d->actionPrevious->setEnabled( false );
    d->actionSeek->setEnabled( false );

    connect( d->doc, SIGNAL(changed()),
             this, SLOT(slotDocChanged()) );
    connect( d->doc, SIGNAL(trackChanged(K3b::AudioTrack*)),
             this, SLOT(slotTrackChanged(K3b::AudioTrack*)) );
    connect( d->doc, SIGNAL(trackRemoved(K3b::AudioTrack*)),
             this, SLOT(slotTrackRemoved(K3b::AudioTrack*)) );
    connect( d->audioOutput, SIGNAL(notify()),
             this, SLOT(slotUpdateSlider()) );
    connect( d->audioOutput, SIGNAL(stateChanged(QAudio::State)),
             this, SLOT(slotStateChanged(QAudio::State)) );
    connect( d->audioDocReader, SIGNAL(currentTrackChanged(K3b::AudioTrack)),
             this, SLOT(slotCurrentTrackChanged(K3b::AudioTrack)) );

    // tooltips
    d->actionPlay->setToolTip( i18n("Play") );
    d->actionStop->setToolTip( i18n("Stop") );
    d->actionPause->setToolTip( i18n("Pause") );
    d->actionNext->setToolTip( i18n("Next") );
    d->actionPrevious->setToolTip( i18n("Previous") );
}


AudioTrackPlayer::~AudioTrackPlayer()
{
}


AudioTrack* AudioTrackPlayer::currentPlayingTrack() const
{
    if( AudioTrackReader* reader = d->audioDocReader->currentTrackReader() )
        return &reader->track();
    else
        return 0;
}


void AudioTrackPlayer::playTrack( const K3b::AudioTrack& track )
{
    d->audioDocReader->setCurrentTrack( track );
}


void AudioTrackPlayer::playPause()
{
    if( d->audioOutput->state() == QAudio::StoppedState ) {
        d->actionPlayPause->setIcon( KIcon( "media-playback-start" ) );
        d->actionPause->setEnabled( true );
        d->actionPlay->setEnabled( false );
        d->actionStop->setEnabled( true );
        d->actionSeek->setEnabled( true );
        d->audioDocReader->open();
        d->audioOutput->start( d->audioDocReader );
    }
    else if( d->audioOutput->state() == QAudio::SuspendedState ) {
        d->actionPlayPause->setIcon( KIcon( "media-playback-pause" ) );
        d->actionPause->setEnabled( true );
        d->actionPlay->setEnabled( false );
        d->actionStop->setEnabled( true );
        d->audioOutput->resume();
    }
    else {
        d->actionPlayPause->setIcon( KIcon( "media-playback-start" ) );
        d->actionPause->setEnabled( false );
        d->actionPlay->setEnabled( true );
        d->actionStop->setEnabled( true );
        d->audioOutput->suspend();
    }
}


void AudioTrackPlayer::stop()
{
    d->actionStop->setEnabled( false );
    d->actionPause->setEnabled( false );
    d->actionPlay->setEnabled( true );
    d->actionSeek->setEnabled( false );
    d->actionNext->setEnabled( false );
    d->actionPrevious->setEnabled( false );
    d->audioOutput->stop();
    d->audioDocReader->close();

    d->actionPlayPause->setIcon( KIcon( "media-playback-start" ) );

    emit stopped();
}


void AudioTrackPlayer::next()
{
    d->audioDocReader->nextTrack();
}


void AudioTrackPlayer::previous()
{
    d->audioDocReader->previousTrack();
}


void AudioTrackPlayer::slotSeek( int bytes )
{
    if( AudioTrackReader* reader = d->audioDocReader->currentTrackReader() ) {
        reader->seek( bytes );
    }
}


void AudioTrackPlayer::slotTrackRemoved( K3b::AudioTrack* /*track*/ )
{
    /*if( m_currentTrack == track ) {
        stop();
        m_currentTrack = 0;
    }*/
}


void AudioTrackPlayer::slotTrackChanged( K3b::AudioTrack* /*track*/ )
{
/*
    if( m_currentTrack == track ) {
        d->actionSeek->setMaximum( track->length().audioBytes() );
    }
*/
}


void AudioTrackPlayer::slotUpdateSlider()
{
    if( AudioTrackReader* reader = d->audioDocReader->currentTrackReader() )
        d->actionSeek->setValue( reader->pos() );
}


void AudioTrackPlayer::slotDocChanged()
{/*
    // update the controls in case a new track has been added before or after
    // the current one and it has been the first or last track
    if( m_currentTrack ) {
        d->actionNext->setEnabled( m_currentTrack->next() != 0 );
        d->actionPrevious->setEnabled( m_currentTrack->prev() != 0 );
    }*/
}


void AudioTrackPlayer::slotCurrentTrackChanged( const K3b::AudioTrack& track )
{
    // we show the currently playing track as a tooltip on the slider
    d->actionSeek->setToolTip( i18n("Playing track %1: %2 - %3")
                                .arg(track.trackNumber())
                                .arg(track.artist())
                                .arg(track.title()) );
    d->actionSeek->setMaximum( track.length().audioBytes() );
    d->actionNext->setEnabled( &track != d->doc->lastTrack() );
    d->actionPrevious->setEnabled( &track != d->doc->firstTrack() );

    emit playingTrack( track );
}


void AudioTrackPlayer::slotStateChanged( QAudio::State state )
{
    switch( state )
    {
        case QAudio::ActiveState:
            emit started();
            break;
        case QAudio::SuspendedState:
            emit paused();
            break;
        case QAudio::IdleState:
        case QAudio::StoppedState:
        default:
            emit stopped();
            break;
    }
}

} // namespace K3b

#include "k3baudiotrackplayer.moc"
