/***************************************************************************
                          k3bsongcontainer.h  -  description
                             -------------------
    begin                : Sat Dec 29 2001
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

#ifndef K3BSONGCONTAINER_H
#define K3BSONGCONTAINER_H

#include <qstring.h>
#include <qvaluelist.h>

class K3bSong;

/**
  *@author Sebastian Trueg
  */

class K3bSongContainer {
public: 
    K3bSongContainer( const QString& path );
    K3bSongContainer();
    ~K3bSongContainer();
    K3bSong* addSong( const K3bSong& song);
    QValueList<K3bSong> getSongs() const;
    const QString& getPath() const;
    bool isEmpty();
private:
    QString m_path;
    typedef QValueList<K3bSong> SongList;
    SongList m_songs;
};

#endif
