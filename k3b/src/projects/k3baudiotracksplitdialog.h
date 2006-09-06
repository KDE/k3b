/* 
 *
 * $Id$
 * Copyright (C) 2004-2006 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2006 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_AUDIO_TRACK_SPLIT_DIALOG_H_
#define _K3B_AUDIO_TRACK_SPLIT_DIALOG_H_

#include <kdialogbase.h>

namespace K3b {
  class Msf;
}
class K3bAudioTrack;
class K3bAudioEditorWidget;
class K3bMsfEdit;


/**
 * Internally used by K3bAudioTrackView to get an msf value from the user.
 */
class K3bAudioTrackSplitDialog : public KDialogBase
{
  Q_OBJECT

 public:
  K3bAudioTrackSplitDialog( K3bAudioTrack*, QWidget* parent = 0, const char* name = 0 );
  ~K3bAudioTrackSplitDialog();

  K3b::Msf currentSplitPos() const;

  /**
   * if this method returns true val is filled with the user selected value.
   */
  static bool getSplitPos( K3bAudioTrack* track, K3b::Msf& val, QWidget* parent = 0, const char* name = 0 );

 private slots:
  void slotRangeModified( int, const K3b::Msf& start, const K3b::Msf& );
  void slotMsfChanged( const K3b::Msf& msf );

 private:
  K3bAudioEditorWidget* m_editorWidget;
  K3bMsfEdit* m_msfEdit;
  int m_firstRange;
  K3bAudioTrack* m_track;
};

#endif
