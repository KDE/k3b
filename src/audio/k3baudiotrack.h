/* 
 *
 * $Id: $
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


#ifndef K3BAUDIOTRACK_H
#define K3BAUDIOTRACK_H

#include <qstring.h>
#include <qfileinfo.h>
#include <qfile.h>
#include <qptrlist.h>


class K3bAudioModule;


/**
  *@author Sebastian Trueg
  */

class K3bAudioTrack
{
 public:
  K3bAudioTrack( QList<K3bAudioTrack>* parent, const QString& filename );
  ~K3bAudioTrack();

  /** returns true if K3b is able to handle the file
   *  which means that it can calculate the length of the track
   *  and the file can be written to a cd.
   **/
  //  bool isValid();

  K3bAudioModule* module() const { return m_module; }

  // TODO: this should only be accessable by K3bAudioDoc
  void setModule( K3bAudioModule* module ) { m_module = module; }

  QString fileName() const { return QFileInfo(m_file).fileName(); }
  QString absPath() const { return QFileInfo(m_file).absFilePath(); }

  QString bufferFile() const { return m_bufferFile; }

  int pregap() const { return m_pregap; }

  /** returns length of track in frames **/
  unsigned long length() const { return m_length; }
	
  const QString& artist() const { return m_artist; }
  const QString& title() const { return m_title; }
  const QString& arranger() const { return m_arranger; }
  const QString& songwriter() const { return m_songwriter; }
  const QString& composer() const { return m_composer; }
  const QString& isrc() const { return m_isrc; }
  const QString& cdTextMessage() const { return m_cdTextMessage; }
  const QString& album() const { return m_album; }
	
  bool copyProtection() const { return m_copy; }
  bool preEmp() const { return m_preEmp; }
	
  void setPregap( int p );

  /**
   * If the file is a mp3-file, it's mp3-tag is used
   **/
  void setArtist( const QString& a ) { m_artist = a; }

  /**
   * If the file is a mp3-file, it's mp3-tag is used
   **/
  void setTitle( const QString& t ) { m_title = t; }
  void setArranger( const QString& t ) { m_arranger = t; }
  void setSongwriter( const QString& t ) { m_songwriter = t; }
  void setComposer( const QString& t ) { m_composer = t; }
  void setIsrc( const QString& t ) { m_isrc = t; }
  void setCdTextMessage( const QString& t ) { m_cdTextMessage = t; }
  void setPreEmp( bool b ) { m_preEmp = b; }
  void setCopyProtection( bool b ) { m_copy = b; }
	
  void setAlbum( const QString& t ) { m_album = t; }
	
  void setLength( unsigned long time ) { m_length = time; }
	
  void setBufferFile( const QString& file );

  /** returns the raw size of the track (16bit, 44800 kHz, stereo) */
  unsigned long size() const;

  /** returns the index in the list */
  int index() const;

  int status() const { return m_status; }
  void setStatus( int status ) { m_status = status; }

  bool isWave() const { return (m_module == 0); }

  /**
   * status of the file.<b>
   * RECOVERABLE means that there are errors in the file that K3b is able to fix.
   * CORRUPT means K3b is not able to handle the file and it needs to be removed from the project.
   */
  enum file_status { OK, RECOVERABLE, CORRUPT };

 protected:
  QList<K3bAudioTrack>* m_parent;
  int m_filetype;
  QFile m_file;

  /** the module that does all the work (decoding and shit!) */
  K3bAudioModule* m_module;

 private:	
  QString m_bufferFile;

  /** length of track in frames (1/75sec) **/
  unsigned long m_length;

  /** frames: 75 frames are one second **/
  int m_pregap;
  
  /** Status of the file. see file_status */
  int m_status;

  /** CD-Text: copy protection */
  bool m_copy;
  bool m_preEmp;
  /** CD-Text: PERFORMER */
  QString m_artist;
  /** CD-Text: TITLE (track) */
  QString m_title;
  /** CD-Text: TITLE (cd) */
  QString m_album;
  QString m_arranger;
  QString m_songwriter;
  QString m_composer;
  QString m_cdTextMessage;
  QString m_isrc;
};


#endif
