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
#include <qobject.h>
#include <qvaluevector.h>
#include <qpair.h>

#include <cddb/k3bcddbquery.h>


class K3bAudioEncoderFactory;
class K3bCdparanoiaLib;
namespace K3bCdDevice {
  class CdDevice;
}


class K3bAudioRipThread : public QObject, public K3bThread
{
  Q_OBJECT

 public:
  K3bAudioRipThread();
  ~K3bAudioRipThread();

  QString jobDescription() const;
  QString jobDetails() const;

  // paranoia settings
  void setParanoiaMode( int mode );
  void setMaxRetries( int r );
  void setNeverSkip( bool b );

  void setSingleFile( bool b ) { m_singleFile = b; }

  void setDevice( K3bCdDevice::CdDevice* dev ) { m_device = dev; }

  void setCddbEntry( const K3bCddbResultEntry& e ) { m_cddbEntry = e; }

  // file naming
  void setUsePattern( bool b ) { m_bUsePattern = b; }
  void setBaseDirectory( const QString& path ) { m_baseDirectory = path; }
  void setDirectoryPattern( const QString& s ) { m_dirPattern = s; }
  void setFilenamePattern( const QString& s ) { m_filenamePattern = s; }
  void setDirectoryReplaceString( const QString& s ) { m_dirReplaceString = s; }
  void setFilenameReplaceString( const QString& s ) { m_filenameReplaceString = s; }
  void setReplaceBlanksInDir( bool b ) { m_replaceBlanksInDir = b; }
  void setReplaceBlanksInFilename( bool b ) { m_replaceBlanksInFilename = b; }

  // if 0 (default) wave files are created
  void setEncoderFactory( K3bAudioEncoderFactory* f );

  /**
   * 1 is the first track
   */
  void setTracksToRip( const QValueVector<QPair<int, QString> >& t ) { m_tracks = t; }

  void cancel();

 private slots:
  void slotCheckIfThreadStillRunning();

 private:
  /** reimplemented from QThread. Does the work */
  void run();

  bool ripTrack( int track, const QString& filename );
  void cleanupAfterCancellation();
  //  QString createFileName( int track );

  K3bCddbResultEntry m_cddbEntry;
  K3bCdDevice::CdDevice* m_device;

  bool m_bUsePattern;
  bool m_singleFile;

  QString m_baseDirectory;
  QString m_dirPattern;
  QString m_filenamePattern;
  QString m_dirReplaceString;
  QString m_filenameReplaceString;
  bool m_replaceBlanksInDir;
  bool m_replaceBlanksInFilename;

  QValueVector<QPair<int, QString> > m_tracks;

  class Private;
  Private* d;
};

#endif
