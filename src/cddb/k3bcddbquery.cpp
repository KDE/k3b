/***************************************************************************
                          k3bcddblookup.cpp  -  description
                             -------------------
    begin                : Sun Oct 7 2001
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


#include "k3bcddbquery.h"

#include "k3bcddbresult.h"

#include <kdebug.h>
#include <kapplication.h>
#include <kaboutdata.h>
#include <klocale.h>


#include <qtextstream.h>
#include <qstringlist.h>
#include <qregexp.h>
#include <qtimer.h>

#include <stdlib.h>


K3bCddbQuery::K3bCddbQuery( QObject* parent, const char* name )
  : QObject(parent, name)
{

}


K3bCddbQuery::~K3bCddbQuery()
{
}


void K3bCddbQuery::query( const K3bToc& toc )
{
  m_toc = toc;
  m_queryResult.clear();
  QTimer::singleShot( 0, this, SLOT(doQuery()) );
}


const QStringList& K3bCddbQuery::categories()
{
  static QStringList s_cat = QStringList::split( ",", "rock,blues,misc,classical,"
						 "country,data,folk,jazz,newage,reggea,soundtrack" );
  return s_cat;
}


bool K3bCddbQuery::parseEntry( QTextStream& stream, K3bCddbResultEntry& entry )
{
  entry.rawData = "";

  // parse data
  QString line;
  while( !(line = stream.readLine()).isNull() ) {
    entry.rawData.append(line + "\n");

    // !all fields my be splitted into several lines!
    // TODO: parse DGENRE, DYEAR
  
    if( line.startsWith( "DISCID" ) ) {
      // TODO: this could be several discids seperated by comma!
    }
    
    else if( line.startsWith( "DTITLE" ) ) {
      entry.cdTitle += line.mid( 7 );
    }
    
    else if( line.startsWith( "TTITLE" ) ) {
      int eqSgnPos = line.find( "=" );
      bool ok;
      uint trackNum = (uint)line.mid( 6, eqSgnPos - 6 ).toInt( &ok );
      if( !ok )
	kdDebug() << "(K3bCddbQuery) !!! PARSE ERROR: " << line << endl;
      else {
	kdDebug() << "(K3bCddbQuery) Track title for track " << trackNum << endl;
	
	// make sure the list is big enough
	while( entry.titles.count() <= trackNum )
	  entry.titles.append( "" );
	
	entry.titles[trackNum] += line.mid( eqSgnPos+1 );
      }
    }
    
    else if( line.startsWith( "EXTD" ) ) {
      entry.cdExtInfo += line.mid( 5 );
    }
    
    else if( line.startsWith( "EXTT" ) ) {
      int eqSgnPos = line.find( "=" );
      bool ok;
      uint trackNum = (uint)line.mid( 4, eqSgnPos - 4 ).toInt( &ok );
      if( !ok )
	kdDebug() << "(K3bCddbQuery) !!! PARSE ERROR: " << line << endl;
      else {
	kdDebug() << "(K3bCddbQuery) Track extr track " << trackNum << endl;

	// make sure the list is big enough
	while( entry.extInfos.count() <= trackNum )
	  entry.extInfos.append( "" );
	
	entry.extInfos[trackNum] += line.mid( eqSgnPos+1 );
      }
    }
    
    else if( line.startsWith( "#" ) ) {
      kdDebug() <<  "(K3bCddbQuery) comment: " << line << endl;
    }
    
    else {
      kdDebug() <<  "(K3bCddbQuery) Unknown field: " << line << endl;
    }
  }

  // now split the titles in the last added match 
  // if no " / " delimiter is present title and artist are the same
  // -------------------------------------------------------------------
  QString fullTitle = entry.cdTitle;
  int splitPos = fullTitle.find( " / " );
  if( splitPos < 0 )
    entry.cdArtist = fullTitle;
  else {
    // split
    entry.cdTitle = fullTitle.mid( splitPos + 3 );
    entry.cdArtist = fullTitle.left( splitPos );
  }


  for( QStringList::iterator it = entry.titles.begin();
       it != entry.titles.end(); ++it ) {
    QString fullTitle = *it;
    int splitPos = fullTitle.find( " / " );
    if( splitPos < 0 )
      entry.artists.append( entry.cdArtist );
    else {
      // split
      *it = fullTitle.mid( splitPos + 3 );
      entry.artists.append( fullTitle.left( splitPos ) );
    }
  }


  // replace all "\\n" with "\n"
  for( QStringList::iterator it = entry.titles.begin();
       it != entry.titles.end(); ++it ) {
    (*it).replace( QRegExp("\\\\\\\\n"), "\\n" );
  }

  for( QStringList::iterator it = entry.artists.begin();
       it != entry.artists.end(); ++it ) {
    (*it).replace( QRegExp("\\\\\\\\n"), "\\n" );
  }

  for( QStringList::iterator it = entry.extInfos.begin();
       it != entry.extInfos.end(); ++it ) {
    (*it).replace( QRegExp("\\\\\\\\n"), "\\n" );
  }

  entry.cdTitle.replace( QRegExp("\\\\\\\\n"), "\\n" );
  entry.cdArtist.replace( QRegExp("\\\\\\\\n"), "\\n" );
  entry.cdExtInfo.replace( QRegExp("\\\\\\\\n"), "\\n" );

  return true;
}


int K3bCddbQuery::getCode( const QString& line )
{
  bool ok;
  int code = line.left( 3 ).toInt( &ok );
  if( !ok )
    code = -1;
  return code;
}


QString K3bCddbQuery::handshakeString() const
{
  QString user( getenv("USER") );
  QString host( getenv("HOST") );
  if( user.isEmpty() )
    user = "kde-user";
  if( host.isEmpty() )
    host = "kde-host";
  
  return QString("%1 %2 K3b %3").arg(user).arg(host).arg(kapp->aboutData()->version());
}


QString K3bCddbQuery::queryString() const
{
  QString query;
  query.sprintf( "cddb query %08x %d", m_toc.discId(), m_toc.count() );
  
  for( K3bToc::const_iterator it = m_toc.begin(); it != m_toc.end(); ++it ) {
    query.append( QString( " %1" ).arg( (*it).firstSector() ) );
  }
  
  query.append( QString( " %1" ).arg( m_toc.length() / 75 ) );
  
  return query;
}


bool K3bCddbQuery::parseExactMatch( const QString &line, K3bCddbResultEntry& entry )
{
  QStringList buffer = QStringList::split( " ", line.mid(4) );
  if( buffer.size() < 3 )
    return false;
  QString cat = buffer[0];
  QString discid = buffer[1];
  QString title = buffer[2];

  kdDebug() << "(K3bCddbQuery) Found exact match: '" << cat << "' '" << discid << "' '" << title << "'" << endl;

  emit infoMessage( i18n("Found exact match") );

  entry.category = cat;
  entry.discid = discid;

  return true;
}

#include "k3bcddbquery.moc"
