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

#ifndef _K3B_AUDIO_TITLE_METAINFO_H_
#define _K3B_AUDIO_TITLE_METAINFO_H_

#include <device/k3bmsf.h>


class K3bAudioTitleMetaInfo
{
 public:
  K3bAudioTitleMetaInfo()
    : m_trackNumber(0) {
  }

  enum FileStatus { 
    OK, 
    RECOVERABLE, 
    CORRUPT
  };

  const QString& artist() const { return m_artist; }
  const QString& title() const { return m_title; }
  const QString& albumArtist() const { return m_albumArtist; }
  const QString& albumTitle() const { return m_albumTitle; }
  const QString& arranger() const { return m_arranger; }
  const QString& songwriter() const { return m_songwriter; }
  const QString& composer() const { return m_composer; }
  const QString& isrc() const { return m_isrc; }

  /** 0 means: not set */
  int trackNumber() const { return m_trackNumber; }
  const K3b::Msf& length() const { return m_length; }

  void setArtist( const QString& a ) { m_artist = a; }
  void setTitle( const QString& a ) { m_title = a; }
  void setAlbumArtist( const QString& a ) { m_albumArtist = a; }
  void setAlbumTitle( const QString& a ) { m_albumTitle = a; }
  void setArranger( const QString& t ) { m_arranger = t; }
  void setSongwriter( const QString& t ) { m_songwriter = t; }
  void setComposer( const QString& t ) { m_composer = t; }
  void setIsrc( const QString& t ) { m_isrc = t; }

  void setTrackNumber( int i ) { m_trackNumber = i; }
  void setLength( const K3b::Msf& msf ) { m_length = msf; }

  // to be expanded as needed

 private:
  QString m_artist;
  QString m_title;
  QString m_albumArtist;
  QString m_albumTitle;
  QString m_arranger;
  QString m_songwriter;
  QString m_composer;
  QString m_isrc;

  int m_trackNumber;
  K3b::Msf m_length;
};

#endif
