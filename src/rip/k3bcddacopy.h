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
#include "../cdinfo/k3bdiskinfo.h"

#include <cddb/k3bcddbquery.h>

typedef Q_INT16 size16;
typedef Q_INT32 size32;

extern "C" {
#include <cdda_interface.h>
#include <cdda_paranoia.h>
}


class QStringList;
class QFile;
class K3bDevice;
class K3bDiskInfoDetector;
class K3bAudioRip;


/**
  *@author Sebastian Trueg
  */
class K3bCddaCopy : public K3bJob 
{
  Q_OBJECT

 public:
  K3bCddaCopy( QObject* parent = 0 );
  ~K3bCddaCopy();

 public slots:
  void setDevice( K3bDevice* dev ) { m_device = dev; }
  void setCddbEntry( const K3bCddbResultEntry& e ) { m_cddbEntry = e; }
  void setCopyTracks( const QValueList<int>& t ) { m_tracksToCopy = t; }
  void setUsePattern( bool b ) { m_bUsePattern = b; }
  void setBaseDirectory( const QString& path ) { m_baseDirectory = path; }
  void setParanoiaMode( int mode ) { m_paranoiaMode = mode; }
  void setMaxRetries( int r ) { m_paranoiaRetries = r; }

  void start();
  void cancel();

 private slots:
  void slotDiskInfoReady( const K3bDiskInfo& );
  void slotTrackOutput( const QByteArray& );
  void slotTrackFinished( bool );

 private:
  void createFilenames();
  bool createDirectory( const QString& dir );
  bool startRip( unsigned int i );

  K3bCddbResultEntry m_cddbEntry;
  K3bDiskInfo m_diskInfo;
  K3bDevice* m_device;

  K3bAudioRip* m_audioRip;

  bool m_bUsePattern;

  K3bDiskInfoDetector* m_diskInfoDetector;

  QString m_baseDirectory;

  QStringList m_list;

  K3bWaveFileWriter m_waveFileWriter;
  QString m_currentWrittenFile;

  unsigned int m_currentTrackIndex;
  unsigned long m_byteCount;
  int m_lastOverallPercent;

  bool m_interrupt;

  QValueList<int> m_tracksToCopy;
  long m_bytes;
  long m_bytesAll;

  int m_paranoiaMode;
  int m_paranoiaRetries;
};

#endif
