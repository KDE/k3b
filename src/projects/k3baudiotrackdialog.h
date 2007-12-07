/* 
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
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

#include <q3ptrlist.h>

class K3bAudioTrack;
class K3bAudioTrackWidget;

/**
  *@author Sebastian Trueg
  */

class K3bAudioTrackDialog : public KDialog
{
  Q_OBJECT

 public:
  K3bAudioTrackDialog( Q3PtrList<K3bAudioTrack>&, QWidget *parent=0);
  ~K3bAudioTrackDialog();
	
 protected slots:
  void slotOk();
  void slotApply();

  void updateTrackLengthDisplay();

 private:
  Q3PtrList<K3bAudioTrack> m_tracks;

  K3bAudioTrackWidget* m_audioTrackWidget;

  void setupGui();
  void setupConnections();
};

#endif
