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


#ifndef K3B_AUDIO_RIP
#define K3B_AUDIO_RIP


#include <k3bjob.h>
#include <qcstring.h>



class K3bCdDevice::CdDevice;
class QTimer;
class K3bCdparanoiaLib;


class K3bAudioRip : public K3bJob
{
  Q_OBJECT

 public:
  K3bAudioRip( QObject* parent = 0 );
  ~K3bAudioRip();

 public slots:
  void start();
  void cancel();
  void setParanoiaMode( int mode ) { m_paranoiaMode = mode; }
  void setMaxRetries( int r ) { m_paranoiaRetries = r; }
  void setNeverSkip( bool b ) { m_neverSkip = b; }
  void setDevice( K3bDevice* dev ) { m_device = dev; }
  void setTrackToRip( unsigned int track ) { m_track = track; }

 signals:
  void output( const QByteArray& );

 protected slots:
  void slotParanoiaRead();
  void slotParanoiaFinished();

 private:
  K3bCdparanoiaLib* m_paranoiaLib;
  K3bDevice* m_device;
  QTimer* m_rippingTimer;

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

  void createStatus(long, int);

  // status variables
  long m_lastReadSector;
  long m_overlap;
  long m_readSectors;


  // this friend function will call createStatus(long,int)
  friend void paranoiaCallback(long, int);
};

#endif
