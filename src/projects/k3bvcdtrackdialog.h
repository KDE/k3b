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

#include <KComboBox>
#include <KDialog>
#include <QLabel>
#include <QList>
#include <QTabWidget>

#include "k3bvcddoc.h"


class QLabel;
class QCheckBox;
class QGroupBox;
class QSpinBox;
class QTreeView;
class KSqueezedTextLabel;

namespace K3b {
    class ListView;
    class VcdTrack;
    class VcdTrackKeysModel;
    class VcdTrackKeysDelegate;

#ifdef __GNUC__
#warning We need a simple replacement for CutComboBox
#endif
    typedef KComboBox CutComboBox;

    class VcdTrackDialog : public KDialog
    {
        Q_OBJECT

    public:
        VcdTrackDialog( VcdDoc*, QList<VcdTrack*>& tracks, QList<VcdTrack*>& selectedTracks, QWidget* parent = 0 );
        ~VcdTrackDialog();

    protected Q_SLOTS:
        void slotOk();
        void slotApply();

    private Q_SLOTS:
        void slotPlayTimeChanged( int );
        void slotWaitTimeChanged( int );
        void slotPbcToggled( bool );
        void slotUseKeysToggled( bool );

    private:
        VcdDoc* m_vcdDoc;
        QList<VcdTrack*> m_tracks;
        QList<VcdTrack*> m_selectedTracks;
        QTabWidget* m_mainTabbed;

        KSqueezedTextLabel* m_displayFileName;
        QLabel* m_labelMimeType;
        QLabel* m_displaySize;
        QLabel* m_displayLength;
        QLabel* m_muxrate;

        QLabel* m_mpegver_audio;
        QLabel* m_rate_audio;
        QLabel* m_sampling_frequency_audio;
        QLabel* m_mode_audio;
        QLabel* m_copyright_audio;

        QLabel* m_mpegver_video;
        QLabel* m_rate_video;
        QLabel* m_chromaformat_video;
        QLabel* m_format_video;
        QLabel* m_resolution_video;
        QLabel* m_highresolution_video;

        QLabel* m_labelAfterTimeout;
        QLabel* m_labelWait;

        QGroupBox* m_groupPlay;
        QGroupBox* m_groupPbc;
        QGroupBox* m_groupKey;
        QWidget* m_widgetnumkeys;

        CutComboBox* m_pbc_previous;
        CutComboBox* m_pbc_next;
        CutComboBox* m_pbc_return;
        CutComboBox* m_pbc_default;
        CutComboBox* m_comboAfterTimeout;

        QCheckBox* m_check_reactivity;
        QCheckBox* m_check_pbc;
        QCheckBox* m_check_usekeys;
        QCheckBox* m_check_overwritekeys;
        QTreeView* m_keys_view;
        VcdTrackKeysModel* m_keys_model;
        VcdTrackKeysDelegate* m_keys_delegate;

        QSpinBox* m_spin_times;
        QSpinBox* m_spin_waittime;

        void prepareGui();
        void setupPbcTab();
        void setupPbcKeyTab();
        void setupAudioTab();
        void setupVideoTab();
        void fillGui();
        void fillPbcGui();

        void setPbcTrack( VcdTrack*, CutComboBox*, int );

        K3b::VcdOptions* VcdOptions()
        {
            return m_vcdDoc->vcdOptions();
        }
    };
}

#endif
