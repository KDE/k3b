/* 
 *
 * $Id$
 * Copyright (C) 2002 Thomas Froescher <tfroescher@k3b.org>
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


#ifndef K3BSONG_H
#define K3BSONG_H

#include <qstring.h>

// TODO: use K3bAudioTitleMetaInfo

class K3bSong 
{
 public: 
  K3bSong( const QString& filename, 
	   const QString& album, 
	   const QString& artist, 
	   const QString& title, 
	   const QString& discId, 
	   int trackNumber);
  K3bSong();
  ~K3bSong();

  const QString& getFilename() const { return m_filename; };
  const QString& getTitle() const { return m_title; };
  const QString& getArtist() const { return m_artist; };
  const QString& getAlbum() const { return m_album; };
  const QString& getDiscId() const { return m_discId; };
  const int getTrackNumber() const { return m_trackNumber; };
  
  void setDiscId( const QString& discid ) { m_discId = discid; }
  void setTrackNumber( int tn ) { m_trackNumber = tn; }
  void setFilename( const QString& filename ) { m_filename = filename; }
  void addContent( const QString& tag, const QString& content );
  
 private:
  QString m_filename;
  QString m_album;
  QString m_artist;
  QString m_title;
  QString m_discId;
  int m_trackNumber;
};

#endif
