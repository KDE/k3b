/***************************************************************************
                          k3bcdcopyjob.h  -  description
                             -------------------
    begin                : Sun Mar 17 2002
    copyright            : (C) 2002 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef K3BCDCOPYJOB_H
#define K3BCDCOPYJOB_H

#include "../k3bjob.h"
#include <qsocketnotifier.h>

class K3bProcess;
class K3bDevice;
class K3bDiskInfo;
class K3bDiskInfoDetector;

/**
  *@author Sebastian Trueg
  */
class K3bCdCopyJob : public K3bBurnJob
{
  Q_OBJECT

 public: 
  K3bCdCopyJob( QObject* parent = 0 );
  ~K3bCdCopyJob();

  K3bDevice* writer() const;

 public slots:
  void start();
  void cancel();

  void setWriter( K3bDevice* dev ) { m_writer = dev; }
  void setReader( K3bDevice* dev ) { m_reader = dev; }
  void setSpeed( int s ) { m_speed = s; }
  void setOnTheFly( bool b ) { m_onTheFly = b; }
  void setKeepImage( bool b ) { m_keepImage = b; }
  void setOnlyCreateImage( bool b ) { m_onlyCreateImage = b; }
  void setDummy( bool b ) { m_dummy = b; }
  /** not usable for cdrdao (enabled by default) */
  void setBurnProof( bool b ) { m_burnProof = b; }
  void setTempPath( const QString& path ) { m_tempPath= path; }
  void setCopies( int c ) { m_copies = c; }
  void setFastToc( bool b ) { m_fastToc = b; }
  void setReadRaw( bool b );
  void setParanoiaMode( int i ) { m_paranoiaMode = i; }

 private slots:
  void diskInfoReady( const K3bDiskInfo& info );

  void cdrdaoCopy();
  void cdrdaoCopyFinished();

  void cdrdaoRead();
  void cdrdaoReadFinished();

  void cdrdaoWrite();
  void cdrdaoWriteFinished();

  void finishAll();
  void cancelAll();

  void addCdrdaoWriteArguments();

  /** reimplemented from K3bBurnJob */
  void parseCdrdaoSpecialLine( const QString& line );
  void getCdrdaoMessage();

 private:
  void startNewCdrdaoTrack();

  int m_copies;
  int m_finishedCopies;
  unsigned long m_blocksToCopy;

  K3bDevice* m_writer;
  K3bDevice* m_reader;
  int m_speed;

  bool m_burnProof;
  bool m_keepImage;
  bool m_onlyCreateImage;
  bool m_onTheFly;
  bool m_dummy;
  bool m_fastToc;
  bool m_readRaw;

  int m_paranoiaMode;

  QString m_tempPath;
  QString m_tocFile;

  K3bProcess* m_process;

  K3bDiskInfoDetector* m_diskInfoDetector;
  // cdrdao communication
  int cdrdaoComm[2];
  QSocketNotifier *qsn;
  int m_currentTrack;
};

#endif
