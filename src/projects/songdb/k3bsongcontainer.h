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


#ifndef K3BSONGCONTAINER_H
#define K3BSONGCONTAINER_H

#include <qstring.h>
#include <qptrlist.h>

class K3bSong;

class K3bSongContainer 
{
 public: 
  K3bSongContainer( const QString& path );
  K3bSongContainer();
  ~K3bSongContainer();

  K3bSong* addSong( K3bSong* song);
  const QPtrList<K3bSong>& getSongs() const { return m_songs; }
  const QString& getPath() const { return m_path; }
  bool isEmpty() const;
  void deleteSong( const QString& filename );

  K3bSong* findSong( const QString& filename ) const;

 private:
  QString m_path;
  QPtrList<K3bSong> m_songs;
};

#endif
