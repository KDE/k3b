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
#include "../k3bcdrdaowriter.h"
#include "../k3bcdrdaoreader.h"
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
  K3bDevice* writer() const { return m_cdrdaowriter->burnDevice(); };

 public slots:
  void start();
  void cancel();

  void setWriter( K3bDevice* dev ) { m_cdrdaowriter->setBurnDevice(dev); }
  void setReader( K3bDevice* dev ) { m_cdrdaoreader->setReadDevice(dev); }
  void setSpeed( int s ) { m_cdrdaowriter->setBurnSpeed(s); }
  void setOnTheFly( bool b ) { m_onTheFly = b; }
  void setKeepImage( bool b ) { m_keepImage = b; }
  void setOnlyCreateImage( bool b ) { m_onlyCreateImage = b; }
  void setDummy( bool b ) { m_cdrdaowriter->setSimulate(b); }
  /** not usable for cdrdao (enabled by default) */
  void setTempPath( const QString& path ) { m_tempPath= path; }
  void setCopies( int c ) { m_copies = c; }
  void setFastToc( bool b ) { m_fastToc = b; }
  void setReadRaw( bool b ) { m_readRaw = b; }
  void setParanoiaMode( int i ) { m_paranoiaMode = i; }

 private slots:
  void diskInfoReady( const K3bDiskInfo& info );

  void cdrdaoDirectCopy();
  void cdrdaoRead(); 
  void cdrdaoWrite();  
  void readFinished(bool);
  void cdrdaoFinished(bool);
  void finishAll();
  void cancelAll();

  void addCdrdaoReadArguments();

  void copyPercent(int p);
  void copySubPercent(int p);
  void slotReaderNextTrack( int, int );
  void slotWriterNextTrack( int, int );

 private:
  int m_copies;
  int m_finishedCopies;

  bool m_keepImage;
  bool m_onlyCreateImage;
  bool m_onTheFly;
  bool m_fastToc;
  bool m_readRaw;
  int m_paranoiaMode;

  QString m_tempPath;
  QString m_tocFile;

  K3bCdrdaoWriter *m_cdrdaowriter;
  K3bCdrdaoReader *m_cdrdaoreader;
  K3bDiskInfoDetector* m_diskInfoDetector;
};

#endif
