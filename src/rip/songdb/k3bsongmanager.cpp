/***************************************************************************
                          k3bsongmanager.cpp  -  description
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

#include "k3bsongmanager.h"
#include "k3bsongcontainer.h"
#include "k3bsong.h"
#include "k3bsonglistparser.h"

#include <qfile.h>
#include <qtextstream.h>

#include <iostream.h>

K3bSongManager::K3bSongManager( const QString& filename )
  : m_filename( filename ){
}

K3bSongManager::~K3bSongManager(){
}

QValueList<K3bSongContainer> K3bSongManager::getContainers(){
    return m_containers;
}
void K3bSongManager::save(){
    QFile f( m_filename );
    if ( f.open(IO_WriteOnly) ) {    // file opened successfully
        QTextStream t( &f );        // use a text stream
        t << "<k3b-CDDB-Database version=\"1.0\">" << endl;
        ContainerList::Iterator con;
        QString insertTab_1 = "    "; // 4 spaces
        for( con = m_containers.begin(); con != m_containers.end(); ++con ){
            t << insertTab_1 << "<cddbtree basepath=\"" << (*con).getPath() << "\">" << "\n";
            typedef QValueList<K3bSong> SongList;
            SongList list;
            list = (*con).getSongs();
            if( list.isEmpty() )
                qDebug("(K3bSongManager) No songs in " + (*con).getPath() );
            SongList::Iterator it;
            for( it = list.begin(); it != list.end(); ++it ){
                QString insertTab_2 = "        "; // 8 spaces
                QString insertTab_3 = "            "; // 12 spaces
                t << insertTab_2 << "<song filename=\"" << (*it).getFilename() << "\" tracknumber=\"";
                t << (*it).getTrackNumber() << "\" discid=\"" << (*it).getDiscId() << "\">\n";
                t << insertTab_3 << "<" << CONTENT_TITLE<< ">" << (*it).getTitle() << "</" << CONTENT_TITLE<< ">\n";
                t << insertTab_3 << "<" << CONTENT_ARTIST<< ">" << (*it).getArtist() << "</" << CONTENT_ARTIST<< ">\n";
                t << insertTab_3 << "<" << CONTENT_ALBUM<< ">" << (*it).getAlbum() << "</" << CONTENT_ALBUM<< ">\n";
                t << insertTab_2 << "</song>\n";
            }
            t << insertTab_1  <<"</cddbtree>" << "\n";
        }
        t << "</k3b-CDDB-Database>" << endl;
        f.close();
    } else {
        qDebug("(K3bSongManager) Can't open file " + m_filename);
    }
}

void K3bSongManager::load(){
    K3bSongListParser handler( this );
    QFile xmlFile( m_filename );
    QXmlInputSource source( xmlFile );
    QXmlSimpleReader reader;
    reader.setContentHandler( &handler );
    reader.parse( source );
        cerr << "<k3b-CDDB-Database version=\"1.0\">" << endl;
        ContainerList::Iterator con;
        QString insertTab_1 = "    "; // 4 spaces
        for( con = m_containers.begin(); con != m_containers.end(); ++con ){
            cerr << insertTab_1 << "<cddbtree basepath=\"" << (*con).getPath() << "\">" << "\n";
            typedef QValueList<K3bSong> SongList;
            SongList list;
            list = (*con).getSongs();
            if( list.isEmpty() )
                qDebug("(K3bSongManager) No songs in " + (*con).getPath() );
            SongList::Iterator it;
            for( it = list.begin(); it != list.end(); ++it ){
                QString insertTab_2 = "        "; // 8 spaces
                QString insertTab_3 = "            "; // 12 spaces
                cerr << insertTab_2 << "<song filename=\"" << (*it).getFilename() << "\" tracknumber=\"";
                cerr << (*it).getTrackNumber() << "\" discid=\"" << (*it).getDiscId() << "\">\n";
                cerr << insertTab_3 << "<" << CONTENT_TITLE<< ">" << (*it).getTitle() << "</" << CONTENT_TITLE<< ">\n";
                cerr << insertTab_3 << "<" << CONTENT_ARTIST<< ">" << (*it).getArtist() << "</" << CONTENT_ARTIST<< ">\n";
                cerr << insertTab_3 << "<" << CONTENT_ALBUM<< ">" << (*it).getAlbum() << "</" << CONTENT_ALBUM<< ">\n";
                cerr << insertTab_2 << "</song>\n";
            }
            cerr << insertTab_1  <<"</cddbtree>" << "\n";
        }
        cerr << "</k3b-CDDB-Database>" << endl;

}

K3bSong* K3bSongManager::findSong( const QString& index ){
    QString path = index.left( index.findRev("/") );
    qDebug("(K3bSongManager) Search container: " + path);
    QString file = index.right( index.length() - 1 - index.findRev("/") );
    qDebug("(K3bSongManager) Search song: " + file);
    K3bSongContainer *con = findContainer( path );
    if( con != 0 ) {
      qDebug("Found container " + con->getPath() );
      K3bSong *song = findSong( file, *con );
      return song;
    }
    else {
      qDebug( "No container found!" );
      return 0;
    }
}
void K3bSongManager::addSong( const QString& path, K3bSong& song){
    K3bSongContainer *con = getContainer( path );
    con->addSong( K3bSong(song) );
}

K3bSongContainer* K3bSongManager::getContainer( const QString& path ){
    K3bSongContainer *resultCon = findContainer( path );
    if( resultCon == 0 ){
        ContainerList::Iterator it;
        qDebug("(K3bSongManager) Container doesn't exist, create one. " + path);
        resultCon = new K3bSongContainer( path );
        it = m_containers.append( *resultCon );
        return &(*it);
    }
    return resultCon;
}

K3bSongContainer* K3bSongManager::findContainer( const QString& path ){
    bool successful = false;
    ContainerList::Iterator it;
    for( it = m_containers.begin(); it != m_containers.end(); ++it ){
        QString tmpPath = (*it).getPath();
        if( tmpPath == path ){
            successful = true;
            break;
        }
    }
    if( !successful )
        return 0;
    return &(*it);
}

K3bSong* K3bSongManager::findSong( const QString& filename, const K3bSongContainer& con ){
    bool successful = false;
    typedef QValueList<K3bSong> SongList;
    SongList list;
    list = con.getSongs();
    if( list.isEmpty() )
        qDebug("(K3bSongManager) Found no song list");
    SongList::Iterator it;
    for( it = list.begin(); it != list.end(); ++it ){
        QString tmp = (*it).getFilename();
        if( tmp == filename ){
            successful = true;
            break;
        }
    }
    if( !successful )
        return 0;
    return &(*it);
}


