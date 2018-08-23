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

#ifndef _K3B_AUDIO_TRACK_PLAYER_H_
#define _K3B_AUDIO_TRACK_PLAYER_H_

#include <QObject>
#include <QScopedPointer>
#include <qaudio.h>

class KActionCollection;

namespace K3b {

    class AudioDoc;
    class AudioTrack;

    class AudioTrackPlayer : public QObject
    {
        Q_OBJECT

    public:
        enum State {
            Stopped,
            Playing,
            Paused
        };

    public:
        AudioTrackPlayer( AudioDoc* doc, KActionCollection* actionCollection, QObject* parent = 0 );
        ~AudioTrackPlayer();

        State state() const;

        AudioTrack* currentTrack() const;

    public Q_SLOTS:
        void playTrack( const K3b::AudioTrack& track );
        void play();
        void pause();
        void stop();
        void next();
        void previous();

    Q_SIGNALS:
        void playingTrack( const K3b::AudioTrack& track );
        void stateChanged();

    private Q_SLOTS:
        void slotSeek( int bytes );
        void slotUpdateSlider();
        void slotCurrentTrackChanged( const K3b::AudioTrack& track );
        void slotStateChanged( QAudio::State state );

    private:
        class Private;
        QScopedPointer<Private> d;
        Q_DISABLE_COPY(AudioTrackPlayer)
    };
}

#endif
