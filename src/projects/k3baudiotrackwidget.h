/*
 *
 * Copyright (C) 2004-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
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
