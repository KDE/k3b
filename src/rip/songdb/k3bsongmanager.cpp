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


#include "k3bsongmanager.h"
#include "k3bsongcontainer.h"
#include "k3bsong.h"
#include "k3bsonglistparser.h"

#include <kdebug.h>

#include <qfile.h>
#include <qtextstream.h>
#include <qstringlist.h>


K3bSongManager::K3bSongManager( QObject* parent, const char* name )
  : QObject( parent, name )
{
  m_containers.setAutoDelete(true);
}

K3bSongManager::~K3bSongManager()
{
}


void K3bSongManager::save()
{
  QFile f( m_filename );
  if ( f.open(IO_WriteOnly) ) {    // file opened successfully
    QTextStream t( &f );        // use a text stream
    t << "<?xml version = \"1.0\" encoding = \"ISO 8859-1\" ?>" << endl;  // needed for german umlauts
    t << "<k3b-CDDB-Database version=\"1.0\">" << endl;

    QString insertTab_1 = "    "; // 4 spaces

    for( QPtrListIterator<K3bSongContainer> con( m_containers ); *con; ++con ) {
      t << insertTab_1 
	<< "<cddbtree basepath=\"" 
	<< con.current()->getPath() 
	<< "\">" << "\n";

      const QPtrList<K3bSong>& list = con.current()->getSongs();

      if( list.isEmpty() )
	kdDebug() << "(K3bSongManager) No songs in " << con.current()->getPath() << endl;

      for( QPtrListIterator<K3bSong> it( list ); *it; ++it ) {
	QString insertTab_2 = "        "; // 8 spaces
	QString insertTab_3 = "            "; // 12 spaces
	t << insertTab_2 << "<song filename=\"" << it.current()->getFilename() << "\" tracknumber=\"";
	t << it.current()->getTrackNumber() << "\" discid=\"" << it.current()->getDiscId() << "\">\n";
	t << insertTab_3 << "<" << CONTENT_TITLE<< ">" << it.current()->getTitle() << "</" << CONTENT_TITLE<< ">\n";
	t << insertTab_3 << "<" << CONTENT_ARTIST<< ">" << it.current()->getArtist() << "</" << CONTENT_ARTIST<< ">\n";
	t << insertTab_3 << "<" << CONTENT_ALBUM<< ">" << it.current()->getAlbum() << "</" << CONTENT_ALBUM<< ">\n";
	t << insertTab_2 << "</song>\n";
      }
      t << insertTab_1  <<"</cddbtree>" << "\n";
    }
    t << "</k3b-CDDB-Database>" << endl;
    f.close();
  } 
  else {
    kdError() << "(K3bSongManager) Can't open file " << m_filename << endl;
  }
}

void K3bSongManager::load( const QString& filename )
{
  m_containers.clear();
  m_filename = filename;
  K3bSongListParser handler( this );
  QFile xmlFile( m_filename );
  QXmlInputSource source( xmlFile );
  QXmlSimpleReader reader;
  reader.setContentHandler( &handler );
  reader.parse( source );
  // debug output
  //  debug();
}

const QStringList& K3bSongManager::verify()
{
  m_missingSongList.clear();

  QString insertTab_1 = "    "; // 4 spaces

  for( QPtrListIterator<K3bSongContainer> con( m_containers ); *con; ++con ) {
    const QPtrList<K3bSong>& list = con.current()->getSongs();

    if( list.isEmpty() )
      kdDebug() << "(K3bSongManager) No songs in " << con.current()->getPath() << endl;
    else {
      for( QPtrListIterator<K3bSong> it( list ); *it; ++it ){
	QString findSong = con.current()->getPath() + "/" + it.current()->getFilename();
	kdDebug() << "(K3bSongManager) Search song: " << findSong << endl;

	if( QFile::exists(findSong) ) {
	  kdDebug() << "(K3bSongManager) Add song that are not found: " << findSong << endl;
	  m_missingSongList.append( findSong );
	}
      }
    }
  }

  return m_missingSongList;
}

K3bSong* K3bSongManager::findSong( const QString& index ) const
{
  QString path = index.left( index.findRev("/") );
  kdDebug() << "(K3bSongManager) Search container: " << path << endl;
  QString file = index.right( index.length() - 1 - index.findRev("/") );
  kdDebug() << "(K3bSongManager) Search song: " << file << endl;

  K3bSongContainer *con = findContainer( path );
  if( con != 0 ) {
    kdDebug() << "Found container " << con->getPath() << endl;
    return con->findSong(file);
  }
  else {
    kdDebug() << "No container found!" << endl;
    return 0;
  }
}


void K3bSongManager::addSong( const QString& path, K3bSong* song )
{
  K3bSongContainer *con = getContainer( path );
  con->addSong( song );
}

void K3bSongManager::deleteSong( const QString& index )
{
  QString path = index.left( index.findRev("/") );
  QString file = index.right( index.length() - 1 - index.findRev("/") );

  K3bSongContainer *con = findContainer( path );
  if( con != 0 ) {
    kdDebug() << "Found container " << con->getPath() << endl;
 
    con->deleteSong( file );
  }
  else {
    kdDebug() << "No container found!" << endl;
  }

  debug();
}

K3bSongContainer* K3bSongManager::getContainer( const QString& path )
{
  K3bSongContainer *resultCon = findContainer( path );
  if( resultCon == 0 ){
    kdDebug() << "(K3bSongManager) Container doesn't exist, create one. " << path << endl;

    resultCon = new K3bSongContainer( path );
    m_containers.append( resultCon );
  }
  return resultCon;
}

K3bSongContainer* K3bSongManager::findContainer( const QString& path ) const
{
  for( QPtrListIterator<K3bSongContainer> con( m_containers ); *con; ++con ) {
    if( con.current()->getPath() == path )
      return con.current();
  }
  
  return 0;
}


void K3bSongManager::debug() const
{
  kdDebug() << "<k3b-CDDB-Database version=\"1.0\">" << endl;

  QString insertTab_1 = "    "; // 4 spaces

  for( QPtrListIterator<K3bSongContainer> con( m_containers ); *con; ++con ) {
    kdDebug() << insertTab_1 << "<cddbtree basepath=\"" << con.current()->getPath() << "\">" << "\n";

    const QPtrList<K3bSong>& list = con.current()->getSongs();

    if( list.isEmpty() )
      kdDebug() << "(K3bSongManager) No songs in " << con.current()->getPath() << endl;

    for( QPtrListIterator<K3bSong> it( list ); *it; ++it ){
      QString insertTab_2 = "        "; // 8 spaces
      QString insertTab_3 = "            "; // 12 spaces
      kdDebug() << insertTab_2 << "<song filename=\"" << it.current()->getFilename() << "\" tracknumber=\""
		<< it.current()->getTrackNumber() << "\" discid=\"" << it.current()->getDiscId() << "\">\n";
      kdDebug() << insertTab_3 << "<" << CONTENT_TITLE<< ">" << it.current()->getTitle() << "</" << CONTENT_TITLE<< ">\n";
      kdDebug() << insertTab_3 << "<" << CONTENT_ARTIST<< ">" << it.current()->getArtist() << "</" << CONTENT_ARTIST<< ">\n";
      kdDebug() << insertTab_3 << "<" << CONTENT_ALBUM<< ">" << it.current()->getAlbum() << "</" << CONTENT_ALBUM<< ">\n";
      kdDebug() << insertTab_2 << "</song>\n";
    }
    kdDebug() << insertTab_1  <<"</cddbtree>" << "\n";
  }
  kdDebug() << "</k3b-CDDB-Database>" << endl;
}


