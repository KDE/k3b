/***************************************************************************
                          k3bsong.cpp  -  description
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

#include "k3bsong.h"
#include "k3bsongmanager.h"

K3bSong::K3bSong( const QString& filename, const QString& album, const QString& artist, const QString& title, unsigned int discId, int trackNumber)
   : m_filename( filename ),
    m_album( album ),
    m_artist( artist ),
    m_title( title ),
    m_discId( discId ),
    m_trackNumber( trackNumber ) {
}

K3bSong::K3bSong(){
}

K3bSong::~K3bSong(){
}

void K3bSong::setFilename( const QString& filename ){
    m_filename = filename;
}

void K3bSong::setDiscId( unsigned int discId ){
    m_discId = discId;
}

void K3bSong::setTrackNumber( int tn ){
    m_trackNumber = tn;
}

void K3bSong::addContent( const QString& tag, const QString& content ){
    if( tag == CONTENT_TITLE ){
        m_title = QString(content);
    } else if( tag == CONTENT_ARTIST ){
        m_artist = content;
    } else if( tag == CONTENT_ALBUM ){
        m_album = content;
    }
}