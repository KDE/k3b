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


#ifndef _K3B_AUDIO_PROJECT_INTERFACE_H_
#define _K3B_AUDIO_PROJECT_INTERFACE_H_

#include "k3bprojectinterface.h"

namespace K3b {
    class AudioDoc;
}


namespace K3b {
class AudioProjectInterface : public ProjectInterface
{
  K_DCOP

 public:
  AudioProjectInterface( AudioDoc* );

 k_dcop:
  int trackCount() const;
  QString title() const;
  QString artist() const;
  QString trackTitle( int trackNum ) const;
  QString trackArtist( int trackNum ) const;
  
  /**
   * Set the global CD-Text title field.
   */
  void setTitle( const QString& title );

  /**
   * Set the global CD-Text artist field.
   */
  void setArtist( const QString& artist );

  /**
   * Set the track CD-Text title field.
   */
  void setTrackTitle( int trackNum, const QString& title );

  /**
   * Set the track CD-Text artist field.
   */
  void setTrackArtist( int trackNum, const QString& artist );

 private:
  AudioDoc* m_audioDoc;
};
}

#endif
