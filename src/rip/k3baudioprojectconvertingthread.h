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


#ifndef K3B_AUDIO_PROJECT_CONVERTING_THREAD_H
#define K3B_AUDIO_PROJECT_CONVERTING_THREAD_H

#include <k3bthread.h>
#include <qobject.h>
#include <qvaluevector.h>
#include <qpair.h>

#include <k3bcddbresult.h>


class K3bAudioEncoder;
class K3bAudioDoc;
class K3bAudioTrack;


class K3bAudioProjectConvertingThread : public K3bThread
{
 public:
  K3bAudioProjectConvertingThread( K3bAudioDoc* );
  ~K3bAudioProjectConvertingThread();

  QString jobDescription() const;
  QString jobDetails() const;

  void setSingleFile( bool b ) { m_singleFile = b; }

  void setCddbEntry( const K3bCddbResultEntry& e ) { m_cddbEntry = e; }

  // if 0 (default) wave files are created
  void setEncoder( K3bAudioEncoder* f );

  /**
   * Used for encoders that support multiple formats
   */
  void setFileType( const QString& );

  /**
   * 1 is the first track
   */
  void setTracksToRip( const QValueVector<QPair<int, QString> >& t ) { m_tracks = t; }

  void setWritePlaylist( bool b ) { m_writePlaylist = b; }
  void setPlaylistFilename( const QString& s ) { m_playlistFilename = s; }
  void setUseRelativePathInPlaylist( bool b ) { m_relativePathInPlaylist = b; }
  void setWriteCueFile( bool b ) { m_writeCueFile = b; }

  void cancel();

 private:
  /** reimplemented from QThread. Does the work */
  void run();

  bool convertTrack( K3bAudioTrack*, const QString& filename );
  bool writePlaylist();
  bool writeCueFile();

  /**
   * Finds a relative path from baseDir to absPath
   */
  QString findRelativePath( const QString& absPath, const QString& baseDir );

  K3bCddbResultEntry m_cddbEntry;

  bool m_singleFile;
  bool m_writePlaylist;
  bool m_relativePathInPlaylist;
  QString m_playlistFilename;

  bool m_writeCueFile;

  QValueVector<QPair<int, QString> > m_tracks;

  K3bAudioDoc* m_doc;

  class Private;
  Private* d;
};

#endif
