/***************************************************************************
                          k3bsongmanager.h  -  description
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

#ifndef K3BSONGMANAGER_H
#define K3BSONGMANAGER_H

#define CONTENT_TITLE     "title"
#define CONTENT_ALBUM  "album"
#define CONTENT_ARTIST  "artist"
#include <qstring.h>
#include <qvaluelist.h>

class K3bSong;
class K3bSongContainer;
/**
  *@author Sebastian Trueg
  */

class K3bSongManager {
public: 
    K3bSongManager( const QString& filename);
    ~K3bSongManager();
    QValueList<K3bSongContainer> getContainers();
    void save();
    void load();
    K3bSong* findSong( const QString& index );
    K3bSong* findSong( const QString& filename, const K3bSongContainer& con );
    void addSong( const QString&path, K3bSong& song);
    // creates an container if no one is found
    K3bSongContainer* getContainer( const QString& path);
private:
    QString m_filename;
    typedef QValueList<K3bSongContainer> ContainerList;
    ContainerList m_containers;

    K3bSongContainer* findContainer( const QString& path);
};

#endif
