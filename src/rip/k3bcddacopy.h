/***************************************************************************
                          k3bcddacopy.h  -  description
                             -------------------
    begin                : Sun Nov 4 2001
    copyright            : (C) 2001 by Sebastian Trueg
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

#ifndef K3BCDDACOPY_H
#define K3BCDDACOPY_H


#include <qobject.h>
#include <qstringlist.h>
#include <qtimer.h>

#include "../k3bjob.h"
#include "../tools/k3bwavefilewriter.h"
#include "k3bcdview.h"
#include "../k3bcddb.h"
#include "../cdinfo/k3bdiskinfo.h"


typedef Q_INT16 size16;
typedef Q_INT32 size32;

extern "C" {
#include <cdda_interface.h>
#include <cdda_paranoia.h>
}

struct cdrom_drive;
class QStringList;
class QFile;
class QDataStream;
class KProgress;
class K3bDevice;
class K3bDiskInfoDetector;


/**
  *@author Sebastian Trueg
  */
class K3bCddaCopy : public K3bJob 
{
  Q_OBJECT

 public:
  K3bCddaCopy( QObject* parent = 0 );
  ~K3bCddaCopy();
  //void start();
/*   void setDrive(QString device); */
/*   void setCopyTracks( QArray<int> tracks ); */
/*   void setCopyFiles( QStringList list ); */
  //void setCopyDirs( QStringList list );
/*   void setCopyCount( int ); */
/*   void setFinish(bool stop); */
/*   void setBytes( long); */
  //bool isFinished() { return m_finished; };

 public slots:
  void setDevice( K3bDevice* dev ) { m_device = dev; }
  void setCddbEntry( const K3bCddbEntry& e ) { m_cddbEntry = e; }
  void setCopyTracks( const QValueList<int>& t ) { m_tracksToCopy = t; }
  void setUsePattern( bool b ) { m_bUsePattern = b; }
  void setBaseDirectory( const QString& path ) { m_baseDirectory = path; }

  void start();
  void cancel();

 private slots:
  void slotReadData();
  void slotDiskInfoReady( const K3bDiskInfo& );

 private:
  void createFilenames();
  bool createDirectory( const QString& dir );

  K3bCddbEntry m_cddbEntry;
  K3bDiskInfo m_diskInfo;
  K3bDevice* m_device;

  bool m_bUsePattern;

  K3bDiskInfoDetector* m_diskInfoDetector;

  QString m_baseDirectory;

  QStringList m_list;
  //QStringList m_directories;

  K3bWaveFileWriter m_waveFileWriter;
  QString m_currentWrittenFile;

  QTimer* m_rippingTimer;

  unsigned int m_currentTrackIndex;
  unsigned long m_byteCount;
  unsigned long m_trackBytesAll;
  int m_lastOverallPercent;
  int m_lastTrackPercent;

  unsigned long m_currentSector;
  unsigned long m_lastSector;
  bool m_interrupt;
  QValueList<int> m_tracksToCopy;
  struct cdrom_drive *m_drive;
  long m_bytes;
  long m_bytesAll;
  bool m_finished;
  //KProgress *m_progress;
  cdrom_paranoia *m_paranoia;
  bool paranoiaRead(struct cdrom_drive *drive, int track, QString dest);
  void readDataFinished();
  bool startRip( unsigned int i );
  void finishedRip();
};

#endif
