/*
 *
 * $Id$
 * Copyright (C) 2003 Christian Kvasny <chris@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef K3BVCDTRACKDIALOG_H
#define K3BVCDTRACKDIALOG_H

#include <k3bvcddoc.h>
#include <kdialogbase.h>
#include <qptrlist.h>
#include <qtabwidget.h>

class K3bVcdTrack;
class QLabel;
class QCheckBox;
class QComboBox;
class QListView;
class QRadioButton;
class QButtonGroup;
class KCutLabel;


class K3bVcdTrackDialog : public KDialogBase
{
  Q_OBJECT

 public:
  K3bVcdTrackDialog( QList<K3bVcdTrack>& tracks, QList<K3bVcdTrack>& selectedTracks, QWidget *parent=0, const char *name=0);
  ~K3bVcdTrackDialog();

 protected slots:
  void slotOk();
  void slotApply();

 private slots:
  void slotPlayForever(bool);
  void slotWaitInfinite(bool);
  void slotPlayTimeChanged(int);
  void slotWaitTimeChanged(int);
  
 private:
  QList<K3bVcdTrack> m_tracks;
  QList<K3bVcdTrack> m_selectedTracks;
  QTabWidget* m_mainTabbed;
  
  KCutLabel* m_displayFileName;
  QLabel* m_labelMimeType;  
  QLabel* m_displaySize;
  QLabel* m_displayLength;

  QLabel* m_mpegver_audio;
  QLabel* m_duration_audio;
  QLabel* m_rate_audio;
  QLabel* m_framesize_audio;
  QLabel* m_mode_audio;
  QLabel* m_extmode_audio;
  QLabel* m_emphasis_audio;
  QLabel* m_copyright_audio;

  QLabel* m_mpegver_video;
  QLabel* m_duration_video;
  QLabel* m_rate_video;
  QLabel* m_chromaformat_video;
  QLabel* m_format_video;
  QLabel* m_size_video;
  QLabel* m_displaysize_video;
  
  QRadioButton* m_radio_playtime;
  QRadioButton* m_radio_playforever;
  QRadioButton* m_radio_waittime;
  QRadioButton* m_radio_waitinfinite;
  
  QComboBox* m_nav_previous;
  QComboBox* m_nav_next;
  QComboBox* m_nav_return;
  QComboBox* m_nav_default;
  QComboBox* m_comboAfterTimeout;
  
  QCheckBox* m_check_usekeys;
  QCheckBox* m_check_overwritekeys;
  QListView* m_list_keys;

  QSpinBox* m_spin_times;
  QSpinBox* m_spin_waittime;

  QButtonGroup* m_groupPlay;
  QButtonGroup* m_groupWait;
  
  void prepareGui();
  void setupNavigationTab();
  void setupAudioTab();
  void setupVideoTab();
  void fillGui();
};

#endif
