/*
    SPDX-FileCopyrightText: 2004 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
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
