/*
    SPDX-FileCopyrightText: 2004-2008 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef _K3B_AUDIO_TRACK_WIDGET_H_
#define _K3B_AUDIO_TRACK_WIDGET_H_

#include "ui_base_k3baudiotrackwidget.h"

#include "k3bmsf.h"

#include <QList>


namespace K3b {
    class AudioTrack;

    /**
     * This class is used internally by AudioTrackDialog.
     */
    class AudioTrackWidget : public QWidget, public Ui::base_K3bAudioTrackWidget
    {
        Q_OBJECT

    public:
        explicit AudioTrackWidget( const QList<AudioTrack*>& tracks,
                          QWidget* parent = 0 );
        ~AudioTrackWidget() override;

    public Q_SLOTS:
        void save();
        void load();

    private:
        QList<AudioTrack*> m_tracks;
    };
}

#endif
