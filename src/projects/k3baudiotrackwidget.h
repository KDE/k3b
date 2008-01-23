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

#include <k3bmsf.h>

#include <QtCore/QList>


class K3bAudioTrack;

/**
 * This class is used internally by K3bAudioTrackDialog.
 */
class K3bAudioTrackWidget : public QWidget, public Ui::base_K3bAudioTrackWidget
{
    Q_OBJECT

public:
    K3bAudioTrackWidget( const QList<K3bAudioTrack*>& tracks, 
                         QWidget* parent = 0 );
    ~K3bAudioTrackWidget();

public Q_SLOTS:
    void save();
    void load();

private:
    QList<K3bAudioTrack*> m_tracks;
};

#endif
