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
 
#ifndef K3BCDDACOPY_H
#define K3BCDDACOPY_H


#include <qobject.h>
#include <qstringlist.h>

#include <k3bjob.h>
#include <tools/k3bwavefilewriter.h>
#include <device/k3bdiskinfo.h>
#include <device/k3bdiskinfodetector.h>

#include "k3baudioripthread.h"

#include <cddb/k3bcddbquery.h>

typedef Q_INT16 size16;
typedef Q_INT32 size32;


class QStringList;
class QFile;
class K3bCdDevice::CdDevice;
class QCustomEvent;

/**
  *@author Sebastian Trueg
  */
class K3bCddaCopy : public K3bJob 
{
  Q_OBJECT

 public:
  K3bCddaCopy( QObject* parent = 0 );
  ~K3bCddaCopy();

  QString jobDescription() const;
  QString jobDetails() const;

 public slots:
  void setDevice( K3bDevice* dev ) { m_device = dev; }
  void setCddbEntry( const K3bCddbResultEntry& e ) { m_cddbEntry = e; }

  /**
   * 1 is the first track
   */
  void setCopyTracks( const QValueList<int>& t ) { m_tracksToCopy = t; }
  void setUsePattern( bool b ) { m_bUsePattern = b; }
  void setBaseDirectory( const QString& path ) { m_baseDirectory = path; }

  void setParanoiaMode( int mode ) { m_audioRip->setParanoiaMode(mode); }
  void setMaxRetries( int r ) { m_audioRip->setMaxRetries(r); }
  void setNeverSkip( bool b ) { m_audioRip->setNeverSkip(b); }
  void setSingleFile( bool b ) { m_singleFile = b; }

  void start();
  void cancel();

 private slots:
  void slotDiskInfoReady( const K3bCdDevice::DiskInfo& );
  void slotTrackOutput( const char*, int );
  void slotTrackFinished( bool );
  void slotTrackPercent( int p );

  void slotCheckIfThreadStillRunning();

 private:
  void createFilenames();
  bool createDirectory( const QString& dir );
  bool startRip( unsigned int i );

  K3bCddbResultEntry m_cddbEntry;
  K3bDiskInfo m_diskInfo;
  K3bDevice* m_device;

  K3bAudioRipThread* m_audioRip;

  bool m_bUsePattern;
  bool m_singleFile;

  K3bDiskInfoDetector* m_diskInfoDetector;

  QString m_baseDirectory;

  QStringList m_list;

  K3bWaveFileWriter m_waveFileWriter;
  QString m_currentWrittenFile;

  unsigned int m_currentTrackIndex;
  unsigned long m_byteCount;
  int m_lastOverallPercent;

  int m_currentRippedTrackNumber;

  bool m_interrupt;

  QValueList<int> m_tracksToCopy;
  long m_bytesToCopy;
  long m_bytesAll;
};

#endif
