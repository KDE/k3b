/*
    SPDX-FileCopyrightText: 2003-2004 Christian Kvasny <chris@k3b.org>
    SPDX-FileCopyrightText: 2008 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2010 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef K3BVCDTRACKDIALOG_H
#define K3BVCDTRACKDIALOG_H

#include <QList>
#include <QDialog>

namespace K3b {
    class VcdDoc;
    class VcdTrack;

    class VcdTrackDialog : public QDialog
    {
        Q_OBJECT

    public:
        VcdTrackDialog( VcdDoc* doc, const QList<VcdTrack*>& tracks, QList<VcdTrack*>& selectedTracks, QWidget* parent = 0 );
        ~VcdTrackDialog() override;

    protected Q_SLOTS:
        void accept() override;
        void slotApply();

    private Q_SLOTS:
        void slotPlayTimeChanged( int value );
        void slotWaitTimeChanged( int value );
        void slotPbcToggled( bool checked );
        void slotUseKeysToggled( bool checked );

    private:
        void prepareGui();
        void setupPbcTab();
        void setupPbcKeyTab();
        void setupAudioTab();
        void setupVideoTab();
        void fillGui();
        void fillPbcGui();
        
        class Private;
        Private* const d;
    };
}

#endif
