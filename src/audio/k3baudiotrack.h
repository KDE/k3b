/***************************************************************************
                          k3baudiotrack.h  -  description
                             -------------------
    begin                : Wed Mar 28 2001
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

#ifndef K3BAUDIOTRACK_H
#define K3BAUDIOTRACK_H

#include <qstring.h>
#include <qfileinfo.h>
#include <qfile.h>
#include <qlist.h>


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

  QString fileName() const { return QFileInfo(m_file).fileName(); }
  QString absPath() const { return QFileInfo(m_file).absFilePath(); }

  QString bufferFile() const { return m_bufferFile; }

  int pregap() const { return m_pregap; }

  /** returns length of track in frames **/
  int length() const { return m_length; }
  bool isAccurateLength() const { return m_isAccurateLength; }
	
  const QString& artist() const { return m_artist; }
  const QString& title() const { return m_title; }
  const QString& arranger() const { return m_arranger; }
  const QString& songwriter() const { return m_songwriter; }
  const QString& isrc() const { return m_isrc; }
  const QString& cdTextMessage() const { return m_cdTextMessage; }
  const QString& album() const { return m_album; }
	
  bool copyProtection() const { return m_copy; }
  bool preEmp() const { return m_preEmp; }
	
  void setPregap( int p ) { m_pregap = p; }

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
  void setIsrc( const QString& t ) { m_isrc = t; }
  void setCdTextMessage( const QString& t ) { m_cdTextMessage = t; }
  void setPreEmp( bool b ) { m_preEmp = b; }
  void setCopyProtection( bool b ) { m_copy = b; }
	
  void setAlbum( const QString& t ) { m_album = t; }
	
  void setLength( int time, bool accurate = false ) { m_length = time; m_isAccurateLength = accurate; }
	
  void setBufferFile( const QString& file );
  /** returns the filesize of the track */
  uint size() const;
  /** returns the index in the list */
  int index() const;

 protected:
  QList<K3bAudioTrack>* m_parent;
  int m_filetype;
  QFile m_file;

  /** the module that does all the work (decoding and shit!) */
  K3bAudioModule* m_module;

 private:	
  QString m_bufferFile;

  /** length of track in frames (1/75sec) **/
  int m_length;
  bool m_isAccurateLength;

  /** frames: 75 frames are one second **/
  int m_pregap;
	
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
  QString m_cdTextMessage;
  QString m_isrc;
};


#endif
