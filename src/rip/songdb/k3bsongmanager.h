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


#ifndef K3BSONGMANAGER_H
#define K3BSONGMANAGER_H

#define CONTENT_TITLE     "title"
#define CONTENT_ALBUM  "album"
#define CONTENT_ARTIST  "artist"

#include <qobject.h>

#include <qstring.h>
#include <qptrlist.h>
#include <qstringlist.h>

class K3bSong;
class K3bSongContainer;


class K3bSongManager : public QObject 
{
  Q_OBJECT

 public: 
  K3bSongManager( QObject* parent = 0, const char* name = 0 );
  ~K3bSongManager();

  const QPtrList<K3bSongContainer>& getContainers() const { return m_containers; }
  void save();
  void load( const QString& filename );
  K3bSong* findSong( const QString& index ) const;
  void addSong( const QString& path, K3bSong* song );
  void deleteSong( const QString& );

  /**
   * creates an container if no one is found
   */
  K3bSongContainer* getContainer( const QString& path);
  const QStringList& verify();

 private:
  QString m_filename;
  QPtrList<K3bSongContainer> m_containers;
  QStringList m_missingSongList;

  K3bSongContainer* findContainer( const QString& path) const;
  void debug() const;
};

#endif
