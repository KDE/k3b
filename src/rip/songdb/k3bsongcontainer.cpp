/***************************************************************************
                          k3bsongcontainer.cpp  -  description
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

#include "k3bsongcontainer.h"
#include "k3bsong.h"

K3bSongContainer::K3bSongContainer( const QString& path)
  : m_path( path ){
}

K3bSongContainer::K3bSongContainer(){
}

K3bSongContainer::~K3bSongContainer(){
}

K3bSong* K3bSongContainer::addSong( const K3bSong& song){
    QString filename = song.getFilename();
    SongList::Iterator it;
    for( it = m_songs.begin(); it != m_songs.end(); ++it ){
        if( filename == (*it).getFilename() ){
            m_songs.remove( it );
            break;
        }
    }
    return &(*m_songs.append( song ));
}
QValueList<K3bSong> K3bSongContainer::getSongs() const{
    return m_songs;
}
const QString& K3bSongContainer::getPath() const {
    return m_path;
}

bool K3bSongContainer::isEmpty(){
    if( m_path.isEmpty() )
        return true;
    return false;
}
