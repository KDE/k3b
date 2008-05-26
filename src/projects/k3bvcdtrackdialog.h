/*
*
* Copyright (C) 2003-2004 Christian Kvasny <chris@k3b.org>
* Copyright (C) 2008 Sebastian Trueg <trueg@k3b.org>
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

#ifndef K3BVCDTRACKDIALOG_H
#define K3BVCDTRACKDIALOG_H

#include <kdialog.h>
#include <qlist.h>
#include <qtabwidget.h>
#include <QLabel>

#include <k3bvcddoc.h>
#include <k3blistview.h>

#include <KComboBox>

class K3bVcdTrack;
class QLabel;
class QCheckBox;
class QGroupBox;
class KSqueezedTextLabel;

#ifdef __GNUC__
#warning We need a simple replacement for K3bCutComboBox
#endif
typedef KComboBox K3bCutComboBox;

class K3bVcdTrackDialog : public KDialog
{
    Q_OBJECT

public:
    K3bVcdTrackDialog( K3bVcdDoc*, QList<K3bVcdTrack*>& tracks, QList<K3bVcdTrack*>& selectedTracks, QWidget* parent = 0 );
    ~K3bVcdTrackDialog();

protected Q_SLOTS:
    void slotOk();
    void slotApply();

private Q_SLOTS:
    void slotPlayTimeChanged( int );
    void slotWaitTimeChanged( int );
    void slotPbcToggled( bool );
    void slotUseKeysToggled( bool );
    void slotGroupkeyToggled( bool );


private:
    K3bVcdDoc* m_vcdDoc;
    QList<K3bVcdTrack*> m_tracks;
    QList<K3bVcdTrack*> m_selectedTracks;
    QMap<QString, K3bVcdTrack*> m_numkeysmap;
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

    K3bCutComboBox* m_pbc_previous;
    K3bCutComboBox* m_pbc_next;
    K3bCutComboBox* m_pbc_return;
    K3bCutComboBox* m_pbc_default;
    K3bCutComboBox* m_comboAfterTimeout;

    QCheckBox* m_check_reactivity;
    QCheckBox* m_check_pbc;
    QCheckBox* m_check_usekeys;
    QCheckBox* m_check_overwritekeys;
    K3bListView* m_list_keys;

    QSpinBox* m_spin_times;
    QSpinBox* m_spin_waittime;

    void prepareGui();
    void setupPbcTab();
    void setupPbcKeyTab();
    void setupAudioTab();
    void setupVideoTab();
    void fillGui();
    void fillPbcGui();

    void setPbcTrack( K3bVcdTrack*, K3bCutComboBox*, int );
    void setDefinedNumKeys( );
    QString displayName( K3bVcdTrack* );
    K3bVcdOptions* VcdOptions()
    {
        return m_vcdDoc->vcdOptions();
    }
};

#endif
