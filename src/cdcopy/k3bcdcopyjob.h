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

class K3bProcess;
class K3bDevice;
class K3bDiskInfo;
class K3bDiskInfoDetector;
class QUrlOperator;


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

 public:
  void setWriter( K3bDevice* dev ) { m_cdrdaowriter->setBurnDevice(dev); }
  void setReader( K3bDevice* dev ) { m_cdrdaowriter->setSourceDevice(dev); }
  void setSpeed( int s ) { m_cdrdaowriter->setBurnSpeed(s); }
  void setOnTheFly( bool b ) { m_onTheFly = b; }
  void setKeepImage( bool b ) { m_keepImage = b; }
  void setOnlyCreateImage( bool b ) { m_onlyCreateImage = b; }
  void setDummy( bool b ) { m_cdrdaowriter->setSimulate(b); }
  /** not usable for cdrdao (enabled by default) */
  void setTempPath( const QString& path ) { m_tempPath= path; }
  void setCopies( int c ) { m_copies = c; }

  void setFastToc( bool b ) { m_cdrdaowriter->setFastToc(b); }
  void setReadRaw( bool b ) { m_cdrdaowriter->setReadRaw(b); }
  void setParanoiaMode( int i ) { m_cdrdaowriter->setParanoiaMode(i); }
  void setReadSubchan(K3bCdrdaoWriter::SubMode m) { m_cdrdaowriter->setReadSubchan(m); };
  void setTaoSource(bool b) { m_cdrdaowriter->setTaoSource(b); };
  void setTaoSourceAdjust(int a) { m_cdrdaowriter->setTaoSourceAdjust(a); };
  void setForce(bool b) { m_cdrdaowriter->setForce(b); };
  
 private slots:
  void diskInfoReady( const K3bDiskInfo& info );

  void cdrdaoFinished(bool);

  void copyPercent(int p);
  void copySubPercent(int p);
  void slotNextTrack( int, int );

 private:
  void cdrdaoDirectCopy();
  void cdrdaoRead(); 
  void cdrdaoWrite();  
  void removeImages();
  void finishAll();
  void cancelAll();
  void fixTocFile(QString &);

 private:
  int m_copies;
  int m_finishedCopies;

  int m_sessions;
  int m_finishedSessions;

  bool m_keepImage;
  bool m_onlyCreateImage;
  bool m_onTheFly;

  QString m_tempPath;
  QString m_tocFile;

  enum Task { READING, WRITING };

  int m_job; // Task

  K3bCdrdaoWriter *m_cdrdaowriter;
  K3bDiskInfoDetector* m_diskInfoDetector;
};

#endif
