/*
 *
 * Copyright (C) 2003-2004 Christian Kvasny <chris@k3b.org>
 * Copyright (C) 2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2010 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef K3BVCDTRACKDIALOG_H
#define K3BVCDTRACKDIALOG_H

#include <KDialog>
#include <QList>

namespace K3b {
    class VcdDoc;
    class VcdTrack;

    class VcdTrackDialog : public KDialog
    {
        Q_OBJECT

    public:
        VcdTrackDialog( VcdDoc* doc, const QList<VcdTrack*>& tracks, QList<VcdTrack*>& selectedTracks, QWidget* parent = 0 );
        ~VcdTrackDialog();

    protected Q_SLOTS:
        void slotOk();
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
