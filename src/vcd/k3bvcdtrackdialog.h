/*
 *
 * $Id: $
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


#include <kdialogbase.h>
#include <qptrlist.h>

class K3bVcdTrack;
class QLineEdit;
class QMultiLineEdit;
class QLabel;
class QCheckBox;
class QComboBox;
class KToggleAction;
class KIntNumInput;
class KCutLabel;


/**
  *@author Sebastian Trueg
  */

class K3bVcdTrackDialog : public KDialogBase
{
  Q_OBJECT

 public:
  K3bVcdTrackDialog( QList<K3bVcdTrack>&, QWidget *parent=0, const char *name=0);
  ~K3bVcdTrackDialog();

 protected slots:
  void slotOk();

 private:
  QList<K3bVcdTrack> m_tracks;

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

  void setupGui();
  void fillGui();
};

#endif
