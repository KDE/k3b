/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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


#ifndef K3B_AUDIO_RIP_THREAD_H
#define K3B_AUDIO_RIP_THREAD_H

#include <k3bthread.h>
#include <qcstring.h>

#include "../device/k3bdevice.h"

class QTimer;
class K3bCdparanoiaLib;


class K3bAudioRipThread : public K3bThread
{
 public:
  K3bAudioRipThread();
  ~K3bAudioRipThread();

  // paranoia settings
  void setParanoiaMode( int mode ) { m_paranoiaMode = mode; }
  void setMaxRetries( int r ) { m_paranoiaRetries = r; }
  void setNeverSkip( bool b ) { m_neverSkip = b; }

  void setDevice( K3bDevice* dev ) { m_device = dev; }
  void setTrackToRip( unsigned int track ) { m_track = track; }

  void cancel();

  void resume();

 private:
  /** reimplemented from QThread. Does the work */
  void run();

  K3bCdparanoiaLib* m_paranoiaLib;
  K3bDevice* m_device;

  long m_currentSector;
  long m_lastSector;
  unsigned long m_sectorsRead;
  unsigned long m_sectorsAll;

  int m_paranoiaMode;
  int m_paranoiaRetries;
  bool m_neverSkip;

  unsigned int m_track;

  bool m_bInterrupt;
  bool m_bError;
  bool m_bSuspended;

  void createStatus(long, int);

  // status variables
  long m_lastReadSector;
  long m_overlap;
  long m_readSectors;

  // this friend function will call createStatus(long,int)
  friend void paranoiaCallback(long, int);
};

#endif
