/*

    SPDX-FileCopyrightText: 2003-2009 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2010 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/

#ifndef K3BAUDIOTRACKDIALOG_H
#define K3BAUDIOTRACKDIALOG_H

#include "k3bmsf.h"

#include <QList>
#include <QDialog>

namespace K3b {
    class AudioTrack;
    class AudioTrackWidget;

    /**
     *@author Sebastian Trueg
     */
    class AudioTrackDialog : public QDialog
    {
        Q_OBJECT

    public:
        explicit AudioTrackDialog( const QList<AudioTrack*>&, QWidget *parent=0);
        ~AudioTrackDialog() override;

    protected Q_SLOTS:
        void accept() override;
        void slotApply();

        void updateTrackLengthDisplay();

    private:
        QList<AudioTrack*> m_tracks;

        AudioTrackWidget* m_audioTrackWidget;
    };
}

#endif
