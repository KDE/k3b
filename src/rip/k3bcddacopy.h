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
class QTimer;
class KProgress;
class K3bDevice;


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

  void setDevice( K3bDevice* dev ) { m_device = dev; }
  void setCddbEntry( const K3bCddbEntry& e ) { m_cddbEntry = e; }
  void setCopyTracks( const QValueList<int>& t ) { m_tracksToCopy = t; }


 public slots:
  void start();
  void cancel();

 private slots:
  void slotReadData();

 private:
 K3bCddbEntry m_cddbEntry;
 K3bDevice* m_device;

  QStringList m_list;
  //QStringList m_directories;

  K3bWaveFileWriter m_waveFileWriter;
  QString m_currentWrittenFile;

  int m_count;
  int m_progressBarValue;
  int m_currentTrackIndex;
  long m_byteCount;
  long m_trackBytesAll;

  long m_currentSector;
  long m_lastSector;
  bool m_interrupt;
  QValueList<int> m_tracksToCopy;
  struct cdrom_drive *m_drive;
  K3bCdda *m_cdda;
  long m_bytes;
  long m_bytesAll;
  bool m_finished;
  //KProgress *m_progress;
  QTimer *t;
  cdrom_paranoia *m_paranoia;
  bool paranoiaRead(struct cdrom_drive *drive, int track, QString dest);
  void readDataFinished();
  bool startRip(int i);
  void finishedRip();
};

#endif
