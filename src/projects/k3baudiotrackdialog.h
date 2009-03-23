/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef K3BAUDIOTRACKDIALOG_H
#define K3BAUDIOTRACKDIALOG_H


#include <kdialog.h>

#include <k3bmsf.h>

#include <QtCore/QList>

namespace K3b {
    class AudioTrack;
    class AudioTrackWidget;

    /**
     *@author Sebastian Trueg
     */
    class AudioTrackDialog : public KDialog
    {
        Q_OBJECT

    public:
        AudioTrackDialog( const QList<AudioTrack*>&, QWidget *parent=0);
        ~AudioTrackDialog();

    protected Q_SLOTS:
        void slotOk();
        void slotApply();

        void updateTrackLengthDisplay();

    private:
        QList<AudioTrack*> m_tracks;

        AudioTrackWidget* m_audioTrackWidget;

        void setupGui();
        void setupConnections();
    };
}

#endif
