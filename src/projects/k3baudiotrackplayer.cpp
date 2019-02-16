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
#include "k3bmsf.h"

#include <KLocalizedString>
#include <KActionCollection>

#include <QAudioDeviceInfo>
#include <QAudioFormat>
#include <QAudioOutput>
#include <QAction>
#include <QSlider>
#include <QToolTip>
#include <QWidgetAction>


namespace K3b {

namespace {

class AudioTrackPlayerSeekAction : public QWidgetAction
{
public:
    AudioTrackPlayerSeekAction( AudioTrackPlayer* player, QObject* parent );

    void setValue( int value );
    void setCurrentTrack( const K3b::AudioTrack& track );
    void reset();

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


void AudioTrackPlayerSeekAction::setValue( int value )
{
    Q_FOREACH( QWidget* widget, createdWidgets() ) {
        if( QSlider* slider = qobject_cast<QSlider*>( widget ) ) {
            slider->setValue( value );
        }
    }
}



void AudioTrackPlayerSeekAction::reset()
{
    setValue( 0 );
}


void AudioTrackPlayerSeekAction::setCurrentTrack( const K3b::AudioTrack& track )
{
    Q_FOREACH( QWidget* widget, createdWidgets() ) {
        if( QSlider* slider = qobject_cast<QSlider*>( widget ) ) {
            // we show the currently playing track as a tooltip on the slider
            slider->setToolTip( i18n("Playing track %1: %2 - %3",
                                track.trackNumber(),
                                track.artist(),
                                track.title()) );
            slider->setMaximum( track.length().audioBytes() );
        }
    }
}


QWidget* AudioTrackPlayerSeekAction::createWidget( QWidget* container)
{
    QSlider* slider = new QSlider( container );
    slider->setRange( 0, 100 );
    slider->setSingleStep( Msf::fromSeconds( 10 ).audioBytes() );
    slider->setPageStep( Msf::fromSeconds( 30 ).audioBytes() );
    slider->setOrientation( Qt::Horizontal );
    slider->setTracking( false );
    connect( slider, SIGNAL(valueChanged(int)), m_player, SLOT(slotSeek(int)) );
    return slider;
}

} // namespace


class AudioTrackPlayer::Private
{
public:
    AudioDoc* doc;
    AudioDocReader* audioDocReader;
    QAudioOutput* audioOutput;

    QAction* actionPlay;
    QAction* actionPause;
    QAction* actionStop;
    QAction* actionNext;
    QAction* actionPrevious;
    AudioTrackPlayerSeekAction* actionSeek;
    State state;
};


AudioTrackPlayer::AudioTrackPlayer( AudioDoc* doc, KActionCollection* actionCollection, QObject* parent )
  : QObject( parent ),
    d( new Private )
{
    d->doc = doc;
    d->audioDocReader = new AudioDocReader( *doc, this );
    d->state = Stopped;

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
    d->actionPlay = new QAction( QIcon::fromTheme( "media-playback-start" ), i18n("Play"), this );
    d->actionPlay->setToolTip( i18n("Play") );
    d->actionPause = new QAction( QIcon::fromTheme( "media-playback-pause" ), i18n("Pause"), this );
    d->actionPause->setVisible( false );
    d->actionPause->setToolTip( i18n("Pause") );
    d->actionStop = new QAction( QIcon::fromTheme( "media-playback-stop" ), i18n("Stop"), this );
    d->actionStop->setEnabled( false );
    d->actionStop->setToolTip( i18n("Stop") );
    d->actionNext = new QAction( QIcon::fromTheme( "media-skip-forward" ), i18n("Next"), this );
    d->actionNext->setEnabled( false );
    d->actionNext->setToolTip( i18n("Next") );
    d->actionPrevious = new QAction( QIcon::fromTheme( "media-skip-backward" ), i18n("Previous"), this );
    d->actionPrevious->setEnabled( false );
    d->actionPrevious->setToolTip( i18n("Previous") );
    d->actionSeek = new AudioTrackPlayerSeekAction( this, actionCollection );
    d->actionSeek->setEnabled( false );

    if( actionCollection ) {
        actionCollection->addAction( "player_play", d->actionPlay );
        actionCollection->addAction( "player_pause", d->actionPause );
        actionCollection->addAction( "player_stop", d->actionStop );
        actionCollection->addAction( "player_next", d->actionNext );
        actionCollection->addAction( "player_previous", d->actionPrevious );
        actionCollection->addAction( "player_seek", d->actionSeek );
    }

    connect( d->audioOutput, SIGNAL(notify()),
             this, SLOT(slotUpdateSlider()) );
    connect( d->audioOutput, SIGNAL(stateChanged(QAudio::State)),
             this, SLOT(slotStateChanged(QAudio::State)) );
    connect( d->audioDocReader, SIGNAL(currentTrackChanged(K3b::AudioTrack)),
             this, SLOT(slotCurrentTrackChanged(K3b::AudioTrack)) );
    connect( d->actionPlay, SIGNAL(triggered(bool)),
             this, SLOT(play()) );
    connect( d->actionPause, SIGNAL(triggered(bool)),
             this, SLOT(pause()) );
    connect( d->actionStop, SIGNAL(triggered(bool)),
             this, SLOT(stop()) );
    connect( d->actionNext, SIGNAL(triggered(bool)),
             this, SLOT(next()) );
    connect( d->actionPrevious, SIGNAL(triggered(bool)),
             this, SLOT(previous()) );
}


AudioTrackPlayer::~AudioTrackPlayer()
{
}


AudioTrackPlayer::State AudioTrackPlayer::state() const
{
    return d->state;
}


AudioTrack* AudioTrackPlayer::currentTrack() const
{
    if( AudioTrackReader* reader = d->audioDocReader->currentTrackReader() )
        return &reader->track();
    else
        return 0;
}


void AudioTrackPlayer::playTrack( const K3b::AudioTrack& track )
{
    play();
    d->audioDocReader->setCurrentTrack( track );
}


void AudioTrackPlayer::play()
{
    if( d->audioOutput->state() == QAudio::StoppedState ||
        d->audioOutput->state() == QAudio::IdleState ) {
        if( d->audioDocReader->open() ) {
            d->audioOutput->start( d->audioDocReader );
        }
    }
    else if( d->audioOutput->state() == QAudio::SuspendedState ) {
        d->audioOutput->resume();
    }
}


void AudioTrackPlayer::pause()
{
    d->audioOutput->suspend();
}


void AudioTrackPlayer::stop()
{
    d->audioOutput->stop();
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


void AudioTrackPlayer::slotUpdateSlider()
{
    if( AudioTrackReader* reader = d->audioDocReader->currentTrackReader() )
        d->actionSeek->setValue( reader->pos() );
}


void AudioTrackPlayer::slotCurrentTrackChanged( const K3b::AudioTrack& track )
{
    d->actionSeek->setCurrentTrack( track );
    d->actionNext->setEnabled( &track != d->doc->lastTrack() );
    d->actionPrevious->setEnabled( &track != d->doc->firstTrack() );

    emit playingTrack( track );
}


void AudioTrackPlayer::slotStateChanged( QAudio::State state )
{
    switch( d->audioOutput->error() ) {
        case QAudio::OpenError:
            qDebug() << "QAudioOutput error: OpenError";
            break;
        case QAudio::IOError:
            qDebug() << "QAudioOutput error: IOError";
            break;
        case QAudio::UnderrunError:
            qDebug() << "QAudioOutput error: UnderrunError";
            break;
        case QAudio::FatalError:
            qDebug() << "QAudioOutput error: FatalError";
            break;
        default:
            break;
    }

    if( QAudio::ActiveState == state ) {
        d->actionPause->setEnabled( true );
        d->actionPlay->setEnabled( false );
        d->actionStop->setEnabled( true );
        d->actionSeek->setEnabled( true );
        d->state = Playing;
    }
    else if( QAudio::SuspendedState == state ) {
        d->actionPause->setEnabled( false );
        d->actionPlay->setEnabled( true );
        d->actionStop->setEnabled( true );
        d->state = Paused;
    }
    else /*if( QAudio::IdleState == state || QAudio::StoppedState == state )*/ {
        d->actionPause->setEnabled( false );
        d->actionPlay->setEnabled( true );
        d->actionStop->setEnabled( false );
        d->actionSeek->setEnabled( false );
        d->actionNext->setEnabled( false );
        d->actionPrevious->setEnabled( false );
        d->audioDocReader->close();
        d->actionSeek->reset();
        d->state = Stopped;
    }
    emit stateChanged();
}

} // namespace K3b


