/* 
 *
 * $Id$
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_AUDIOTRACK_TRM_LOOKUP_DIALOG_H_
#define _K3B_AUDIOTRACK_TRM_LOOKUP_DIALOG_H_

#include <kdialog.h>
#include <qptrlist.h>

class QLabel;
class K3bAudioTrack;
class K3bTRMLookup;
class K3bBusyWidget;

class K3bAudioTrackTRMLookupDialog : public KDialog
{
  Q_OBJECT

 public:
  K3bAudioTrackTRMLookupDialog( QWidget* parent = 0, const char* name = 0 );
  ~K3bAudioTrackTRMLookupDialog();

  /**
   * This will show the dialog and start the lookup
   */
  int lookup( const QPtrList<K3bAudioTrack>& tracks );

 private slots:
  void slotLookupFinished( K3bTRMLookup* );

 private:
  void lookup( K3bAudioTrack* track );

  QLabel* m_infoLabel;
  K3bBusyWidget* m_busyWidget;

  QPtrList<K3bAudioTrack> m_tracks;
};

#endif
