/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#ifndef K3BAUDIOTRACKDIALOG_H
#define K3BAUDIOTRACKDIALOG_H


#include <kdialogbase.h>

#include <k3bmsf.h>

#include <qptrlist.h>

class K3bAudioTrack;
class K3bAudioTrackWidget;

/**
  *@author Sebastian Trueg
  */

class K3bAudioTrackDialog : public KDialogBase
{
  Q_OBJECT

 public:
  K3bAudioTrackDialog( QPtrList<K3bAudioTrack>&, QWidget *parent=0, const char *name=0);
  ~K3bAudioTrackDialog();
	
 protected slots:
  void slotOk();
  void slotApply();

  void updateTrackLengthDisplay();

 private:
  QPtrList<K3bAudioTrack> m_tracks;

  K3bAudioTrackWidget* m_audioTrackWidget;

  void setupGui();
  void setupConnections();
};

#endif
