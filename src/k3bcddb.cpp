/***************************************************************************
                          k3bcddb.cpp  -  description
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif


#include <qstring.h>
#include <qvaluelist.h>
#include <qstringlist.h>
#include <qsocket.h>
#include <qtextstream.h>
#include <qregexp.h>
#include <qfile.h>
#include <qdir.h>
#include <qtimer.h>

#include <klocale.h>
#include <kconfig.h>
#include <kio/job.h>
#include <kio/global.h>
#include <kprotocolmanager.h>
#include <kdebug.h>

#include <stdlib.h>

#include "k3bcddb.h"

#include "device/k3btoc.h"
#include "device/k3btrack.h"



typedef Q_INT16 size16;
typedef Q_INT32 size32;

/* This is in support for the Mega Hack, if cdparanoia ever is fixed, or we
 * use another ripping library we can remove this.  */
// #include <linux/cdrom.h>
// #include <sys/ioctl.h>

// extern "C" {
// #include <cdda_interface.h>
// #include <cdda_paranoia.h>
// }
// #define MAX_IPC_SIZE (1024*32)
// #define DEFAULT_CDDB_SERVER "localhost:888"

// extern "C" {
//   int FixupTOC( cdrom_drive * d, int tracks );
// }
// int start_of_first_data_as_in_toc;
// int hack_track;

/* Mega hack.  This function comes from libcdda_interface, and is called by
 * it.  We need to override it, so we implement it ourself in the hope, that
 * shared lib semantics make the calls in libcdda_interface to FixupTOC end
 * up here, instead of it's own copy.  This usually works.
 * You don't want to know the reason for this.  */

// int FixupTOC( cdrom_drive * d, int tracks )
// {
//   int j;
//   for( j = 0; j < tracks; j++ ) {
//     if( d->disc_toc[j].dwStartSector < 0 )
//       d->disc_toc[j].dwStartSector = 0;
//     if( j < tracks - 1 && d->disc_toc[j].dwStartSector > d->disc_toc[j + 1].dwStartSector )
//       d->disc_toc[j].dwStartSector = 0;
//   }
//   long last = d->disc_toc[0].dwStartSector;
//   for( j = 1; j < tracks; j++ ) {
//     if( d->disc_toc[j].dwStartSector < last )
//       d->disc_toc[j].dwStartSector = last;
//   }
//   start_of_first_data_as_in_toc = -1;
//   hack_track = -1;
//   if( d->ioctl_fd != -1 ) {
//     struct cdrom_multisession ms_str;
//     ms_str.addr_format = CDROM_LBA;
//     if( ioctl( d->ioctl_fd, CDROMMULTISESSION, &ms_str ) == -1 )
//       return -1;
//     if( ms_str.addr.lba > 100 ) {
//       for( j = tracks - 1; j >= 0; j-- )
// 	if( j > 0 && !IS_AUDIO( d, j ) && IS_AUDIO( d, j - 1 ) ) {
// 	  if( d->disc_toc[j].dwStartSector > ms_str.addr.lba - 11400 ) {

// 	    /* The next two code lines are the purpose of duplicating this
// 	     * function, all others are an exact copy of paranoias FixupTOC().
// 	     * The gory details: CD-Extra consist of N audio-tracks in the
// 	     * first session and one data-track in the next session.  This
// 	     * means, the first sector of the data track is not right behind
// 	     * the last sector of the last audio track, so all length
// 	     * calculation for that last audio track would be wrong.  For this
// 	     * the start sector of the data track is adjusted (we don't need
// 	     * the real start sector, as we don't rip that track anyway), so
// 	     * that the last audio track end in the first session.  All well
// 	     * and good so far.  BUT: The CDDB disc-id is based on the real
// 	     * TOC entries so this adjustment would result in a wrong Disc-ID.
// 	     * We can only solve this conflict, when we save the old
// 	     * (toc-based) start sector of the data track.  Of course the
// 	     * correct solution would be, to only adjust the _length_ of the
// 	     * last audio track, not the start of the next track, but the
// 	     * internal structures of cdparanoia are as they are, so the
// 	     * length is only implicitely given.  Bloody sh*.  */

// 	    start_of_first_data_as_in_toc = d->disc_toc[j].dwStartSector;
// 	    hack_track = j + 1;
// 	    d->disc_toc[j].dwStartSector = ms_str.addr.lba - 11400;
// 	  }
// 	  break;
// 	}
//       return 1;
//     }
//   }
//   return 0;
// }

//  libcdda returns for cdda_disc_lastsector() the last sector of the last
//  _audio_ track.  How broken.  For CDDB Disc-ID we need the real last sector
//  to calculate the disc length.  */
// long my_last_sector( cdrom_drive * drive )
// {
//   return cdda_track_lastsector( drive, drive->tracks );
// }




// /////////////////////////////////////////////////////////
// NEW IMPLEMETATION
// /////////////////////////////////////////////////////////


K3bCddbQuery::K3bCddbQuery()
{
}


// K3bCddbQuery::K3bCddbQuery( const K3bCddbQuery& query )
//   : m_titleList( query.m_titleList ),
// {
// }

void K3bCddbQuery::clear()
{
  m_entries.clear();
}


int K3bCddbQuery::foundEntries() const
{
  return m_entries.count();
}

const K3bCddbEntry& K3bCddbQuery::entry( unsigned int number ) const
{
  if( number >= m_entries.count() )
    return m_emptyEntry;

  return m_entries[number];
}


void K3bCddbQuery::addEntry( const K3bCddbEntry& entry )
{
  m_entries.append( entry );
}


K3bCddb::K3bCddb( QObject* parent, const char* name )
  : QObject( parent, name ),
    m_bUseProxyServer( false ),
    m_bUseKdeSettings( true ),
    m_bSaveCddbEntriesLocally( true ),
    m_bCddbpQuery( false ),
    m_bSearchLocalDirs( true ),
    m_bUseManualCgiPath( false )
{
  m_localCddbDirs.append( ".cddb/" );

  m_socket = new QSocket( this );

  connect( m_socket, SIGNAL(connected()), this, SLOT(slotConnected()) );
  connect( m_socket, SIGNAL(hostFound()), this, SLOT(slotHostFound()) );
  connect( m_socket, SIGNAL(connectionClosed()), this, SLOT(slotConnectionClosed()) );
  connect( m_socket, SIGNAL(error(int)), this, SLOT(slotError(int)) );
  connect( m_socket, SIGNAL(readyRead()), this, SLOT(slotReadyRead()) );

  m_cddbCategories.append( "rock" );
  m_cddbCategories.append( "blues" );
  m_cddbCategories.append( "misc" );
  m_cddbCategories.append( "classical" );
  m_cddbCategories.append( "country" );
  m_cddbCategories.append( "data" );
  m_cddbCategories.append( "folk" );
  m_cddbCategories.append( "jazz" );
  m_cddbCategories.append( "newage" );
  m_cddbCategories.append( "reggea" );
  m_cddbCategories.append( "soundtrack" );
}


K3bCddb::~K3bCddb()
{
}


void K3bCddb::readConfig( KConfig* c )
{
  c->setGroup( "Cddb" );

  m_cddbpServer = c->readListEntry( "cddbp server" );
  m_httpServer = c->readListEntry( "http server" );
  m_localCddbDirs = c->readListEntry( "local cddb dirs" );
  m_proxyServer = c->readEntry( "proxy server" );
  m_proxyPort = c->readNumEntry( "proxy port" );
  m_bUseProxyServer = c->readBoolEntry( "use proxy server", false );
  m_bSaveCddbEntriesLocally = c->readBoolEntry( "save cddb entries locally", true );
  m_bCddbpQuery = c->readBoolEntry( "query via cddbp", false );
  m_bUseManualCgiPath = c->readBoolEntry( "use manual cgi path", false );
  m_cgiPath = c->readEntry( "cgi path", "~cddb/cddb.cgi" );
  m_bUseKdeSettings = ( c->readEntry( "proxy settings type", "kde" ) == "kde" );

  if( m_localCddbDirs.isEmpty() )
    m_localCddbDirs.append( "~/.cddb/" );
  if( m_cddbpServer.isEmpty() )
    m_cddbpServer.append( "freedb.org:8880" );
  if( m_httpServer.isEmpty() )
    m_httpServer.append( "freedb.org:80" );
}


void K3bCddb::query( const K3bToc& toc )
{
  if( m_bSearchLocalDirs )
    localQuery( toc );
  else if( m_bCddbpQuery )
    cddbpQuery( toc );
  else
    httpQuery( toc );
}


void K3bCddb::cddbpQuery( const K3bToc& toc )
{
  m_toc = toc;

  m_error = WORKING;
  m_queryType = CDDBP;

  m_iCurrentQueriedServer = 0;
  queryCdOnServer();
}


void K3bCddb::httpQuery( const K3bToc& toc )
{
  m_toc = toc;

  m_error = WORKING;
  m_queryType = HTTP;
  m_state = QUERY;

  m_iCurrentQueriedServer = 0;
  m_matches.clear();
  m_query.clear();

  if( !connectToHttpServer( 0 ) ) {
    kdDebug() << "(K3bCddb) Error: could not try first connect. REPORT!" << endl;
  }
}


void K3bCddb::localQuery( const K3bToc& toc )
{
  m_toc = toc;

  m_error = WORKING;
  m_queryType = LOCAL;
  m_matches.clear();
  m_query.clear();

  m_iCurrentLocalDir = 0;
  searchLocalDir();
}


bool K3bCddb::splitServerPort( const QString& str, QString& server, int& port )
{
  int colPos = str.find( ":" );
  if( colPos < 0 )
    return false;

  server = str.left( colPos );
  bool ok;
  port = str.mid( colPos + 1 ).toInt( &ok );

  return ok;
}


void K3bCddb::queryCdOnServer()
{
  // TODO: set maximum cddb protocol level (5) with "cddb proto"

  if( m_cddbpServer.count() <= m_iCurrentQueriedServer ) {
    kdDebug() << "(K3bCddb) all server queried." << endl;
    m_error = NO_ENTRY_FOUND;
    emit queryFinished( this );
    return;
  }

  m_state = GREETING;
  m_matches.clear();
  m_query.clear();

  QString server;
  int port;

  if( splitServerPort( m_cddbpServer[m_iCurrentQueriedServer], server, port ) ) {
    m_currentlyConnectingServer = server;
    m_socket->connectToHost( server, port );
    emit infoMessage( i18n("Searching %1 on port %2").arg(server).arg(port) );
  }
  else {
    kdDebug() <<  "(K3bCddb) parsing problem: " << m_cddbpServer[m_iCurrentQueriedServer] << endl;
    m_iCurrentQueriedServer++;
    queryCdOnServer();
  }
}

void K3bCddb::slotHostFound()
{
  emit infoMessage( i18n("Host found") );
}


void K3bCddb::slotConnected()
{
  emit infoMessage( i18n("Connected") );

  if( m_queryType == HTTP ) {
    if( m_state == QUERY ) {

      // set query
      QString query = createHttpUrl( m_iCurrentQueriedServer );
      query.append( "?cmd=" );
      query.append( queryString().replace( QRegExp( "\\s" ), "+" ) );
      query.append( "&hello=" );
      query.append( handshakeString().replace( QRegExp( "\\s" ), "+" ) );
      query.append( "&proto=5" );

      query.prepend( "GET " );

      QTextStream stream( m_socket );
      stream << query << "\n";
    }
    else if( m_state == READ ) {

      // send read command for first entry
      QString query = createHttpUrl( m_iCurrentQueriedServer );
      query.append( QString( "?cmd=cddb+read+%1+%2").arg( m_matches.first().category ).arg( m_matches.first().discid ) );
      query.append( "&hello=" );
      query.append( handshakeString().replace( QRegExp( "\\s" ), "+" ) );
      query.append( "&proto=5" );
      
      query.prepend( "GET " );

      m_parsingBuffer = "";
      
      kdDebug() <<  "(K3bCddb) Read: " << query << endl;

      QTextStream stream( m_socket );
      stream << query << "\n";
    }
    else 
      kdDebug() << "(K3bCddb) Wrong state in http mode" << endl;
  }
}


void K3bCddb::slotConnectionClosed()
{
  emit infoMessage( i18n("Connection closed") );

  if( m_queryType == CDDBP ) {
    if( m_error == SUCCESS ) {
      emit queryFinished( this );
    }
    else {
      m_iCurrentQueriedServer++;
      queryCdOnServer();
    }
  }
  else { // HTTP
    if( m_state == QUERY ) {
      // the query was not successfull
      // query next server
      m_iCurrentQueriedServer++;

      if( !connectToHttpServer( m_iCurrentQueriedServer ) ) {
	// no success
	//	m_error = NO_ENTRY_FOUND;
	emit queryFinished( this );
      }
    }
    else if( m_state == READ ) {
      // successfull query

      if( m_matches.isEmpty() ) {
	// all matches read.
	// finish
	if( m_error != READ_ERROR )
	  m_error = SUCCESS;
	emit queryFinished( this );
      }
      else {
	// connect to host to send the read command
	if( !connectToHttpServer( m_iCurrentQueriedServer ) ) {
	  // this should really not happen!
	  kdDebug() << "(K3bCddb) could not connect for reading. REPORT!" << endl;
	}
      }
    }
  }
}


void K3bCddb::cddbpQuit()
{
  m_state = QUIT;
  QTextStream stream( m_socket );
  stream << "quit\n";
}


void K3bCddb::slotReadyRead()
{
  QTextStream stream( m_socket );

  while( m_socket->canReadLine() ) {
    QString line = stream.readLine();

    switch( m_state ) {
    case GREETING:
      if( getCode( line ) == 200 || getCode( line ) == 201) {
	emit infoMessage( i18n("OK, read access") );
	m_state = HANDSHAKE;

	QTextStream stream( m_socket );
	stream << "cddb hello " << handshakeString() << "\n";
      }

      else {
	emit infoMessage( i18n("Connection refused") );
	m_iCurrentQueriedServer++;
	queryCdOnServer(); // try next one
      }
      break;

    case HANDSHAKE:
      if( getCode( line ) == 200 ) {
	emit infoMessage( i18n("Handshake successful") );

	m_state = QUERY;

	QTextStream stream( m_socket );
	stream << queryString() << "\n";
      }

      else {
	emit infoMessage( i18n("Handshake failed") );  // server closes connection
	m_iCurrentQueriedServer++;
	queryCdOnServer(); // try next one
      }
      break;

    case QUERY:
      if( getCode( line ) == 200 ) {
	// parse exact match and send a read command
	QString buffer = line.mid( 4 );
	int pos = buffer.find( " " );
	QString cat = buffer.left( pos );
	buffer = buffer.mid( pos + 1 );
	pos = buffer.find( " " );
	QString discid = buffer.left( pos );
	buffer = buffer.mid( pos + 1 );
	QString title = buffer.stripWhiteSpace();

	kdDebug() << "(K3bCddb) Found exact match: '" << cat << "' '" << discid << "' '" << title << "'" << endl;

	emit infoMessage( i18n("Found exact match") );

	K3bCddbEntry entry;
	entry.category = cat;
	entry.discid = discid;
	m_matches.append( entry );

	if( m_queryType == CDDBP )
	  readFirstEntry();
	else
	  m_state = READ;
      }

      else if( getCode( line ) == 210 ) {
	// TODO: perhaps add an "exact" field to K3bCddbEntry
	kdDebug() << "(K3bCddb) Found multiple exact matches" << endl;

	emit infoMessage( i18n("Found multiple exact matches") );

	m_state = QUERY_DATA;
      }

      else if( getCode( line ) == 211 ) {
	kdDebug() << "(K3bCddb) Found inexact matches" << endl;

	emit infoMessage( i18n("Found inexact matches") );

	m_state = QUERY_DATA;
      }

      else if( getCode( line ) == 202 ) {
	kdDebug() << "(K3bCddb) no match found" << endl;
	emit infoMessage( i18n("No match found") );
	m_error = NO_ENTRY_FOUND;
	if( m_queryType == CDDBP )
	  cddbpQuit();
      }

      else {
	kdDebug() << "(K3bCddb) Error while querying: " << line << endl;
	emit infoMessage( i18n("Error while querying") );
	m_error = QUERY_ERROR;
	if( m_queryType == CDDBP )
	  cddbpQuit();
      }
      break;

    case QUERY_DATA:
      if( line.startsWith( "." ) ) {
	// finished query
	// go on reading

	if( m_queryType == CDDBP )
	  readFirstEntry();
	else
	  m_state = READ;
      }
      else {
	QStringList match = QStringList::split( " ", line );

	kdDebug() << "(K3bCddb) inexact match: " << line << endl;

	K3bCddbEntry entry;
	entry.category = match[0];
	entry.discid = match[1];
	m_matches.append( entry );
      }
      break;

    case READ:
      if( getCode( line ) == 210 ) {

	// we just start parsing the read data
	m_state = READ_DATA;
      }

      else {
	emit infoMessage( i18n("Could not read match") );
	m_matches.erase( m_matches.begin() );  // remove the unreadable match
	m_error = READ_ERROR;
	if( m_queryType == CDDBP )
	  cddbpQuit();
      }
      break;
    

    case READ_DATA:

      kdDebug() << "parsing line: " << line << endl;

      if( line.startsWith( "." ) ) {
	
	kdDebug() << "(K3bCddb) query finished." << endl;

	QTextStream strStream( m_parsingBuffer, IO_ReadOnly );
	K3bCddbEntry entry = *m_matches.begin();
	parseEntry( strStream, entry );

	m_query.addEntry( entry );
	m_matches.erase( m_matches.begin() );
	
	// write entry to local cddb dir
	if( m_bSearchLocalDirs && m_bSaveCddbEntriesLocally ) {
	  saveLocalEntry( entry, m_parsingBuffer );
	}

	if( m_queryType == CDDBP ) {	
	  if( !readFirstEntry() ) {
	    m_error = SUCCESS;
	    if( m_queryType == CDDBP )
	      cddbpQuit();
	  }
	}
	else {
	  m_state = READ;
	}
      }

      else {
	m_parsingBuffer.append(line + "\n");
      }
      break;

    case QUIT:
      // no parsing needed
      break;
    }
  }
}


bool K3bCddb::readFirstEntry()
{
  if( m_matches.isEmpty() )
    return false;

  QString read = QString( "cddb read %1 %2").arg( m_matches.first().category ).arg( m_matches.first().discid );
  
  m_state = READ;
  m_parsingBuffer = "";
  
  kdDebug() <<  "(K3bCddb) Read: " << read << endl;

  QTextStream stream( m_socket );
  stream << read << "\n";

  return true;
}


bool K3bCddb::parseEntry( QTextStream& stream, K3bCddbEntry& entry )
{
  // parse data
  QString line;
  while( !(line = stream.readLine()).isNull() ) {
    // !all fields my be splitted into several lines!
    // TODO: parse DGENRE, DYEAR
  
    if( line.startsWith( "DISCID" ) ) {
      // TODO: this could me several discids seperated by comma!
    }
    
    else if( line.startsWith( "DTITLE" ) ) {
      entry.cdTitle += line.mid( 7 );
    }
    
    else if( line.startsWith( "TTITLE" ) ) {
      int eqSgnPos = line.find( "=" );
      bool ok;
      int trackNum = line.mid( 6, eqSgnPos - 6 ).toInt( &ok );
      if( !ok )
	kdDebug() << "(K3bCddb) !!! PARSE ERROR: " << line << endl;
      else {
	kdDebug() << "(K3bCddb) Track title for track " << trackNum << endl;
	
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
      int trackNum = line.mid( 4, eqSgnPos - 4 ).toInt( &ok );
      if( !ok )
	kdDebug() << "(K3bCddb) !!! PARSE ERROR: " << line << endl;
      else {
	kdDebug() << "(K3bCddb) Track extr track " << trackNum << endl;

	// make sure the list is big enough
	while( entry.extInfos.count() <= trackNum )
	  entry.extInfos.append( "" );
	
	entry.extInfos[trackNum] += line.mid( eqSgnPos+1 );
      }
    }
    
    else if( line.startsWith( "#" ) ) {
      kdDebug() <<  "(K3bCddb) comment: " << line << endl;
    }
    
    else {
      kdDebug() <<  "(K3bCddb) Unknown field: " << line << endl;
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



void K3bCddb::slotError( int e )
{
  switch(e) {
  case QSocket::ErrConnectionRefused:
    kdDebug() <<  i18n("Connection to %1 refused").arg( m_currentlyConnectingServer ) << endl;
    emit infoMessage( i18n("Connection to %1 refused").arg( m_currentlyConnectingServer ) );
    break;
  case QSocket::ErrHostNotFound:
    kdDebug() <<  i18n("Could not find host %1").arg( m_currentlyConnectingServer ) << endl;
    emit infoMessage( i18n("Could not find host %1").arg( m_currentlyConnectingServer ) );
    break;
  case QSocket::ErrSocketRead:
    kdDebug() <<  i18n("Error while reading from %1").arg( m_currentlyConnectingServer ) << endl;
    emit infoMessage( i18n("Error while reading from %1").arg( m_currentlyConnectingServer ) );
    break;
  }

  m_iCurrentQueriedServer++;
  queryCdOnServer();
}


int K3bCddb::getCode( const QString& line )
{
  bool ok;
  int code = line.left( 3 ).toInt( &ok );
  if( !ok )
    code = -1;
  return code;
}


void K3bCddb::searchLocalDir()
{
  if( m_localCddbDirs.count() <= m_iCurrentLocalDir ) {
    kdDebug() << "(K3bCddb) all local dirs searched" << endl;
    m_error = NO_ENTRY_FOUND;
    emit queryFinished( this );
    return;
  }


  emit infoMessage( i18n("Searching entry in %1").arg( m_localCddbDirs[m_iCurrentLocalDir] ) );
  kapp->processEvents(); //BAD!

  QString path = m_localCddbDirs[m_iCurrentLocalDir];
  if( path.startsWith( "~" ) )
    path.replace( 0, 1, QDir::homeDirPath() + "/" );
  else if( !path.startsWith( "/" ) )
    path.prepend( QDir::homeDirPath() + "/" );
  if( path[path.length()-1] != '/' )
    path.append( "/" );

  for( QStringList::const_iterator it = m_cddbCategories.begin();
       it != m_cddbCategories.end(); ++it ) {

    QString file = path + *it + "/" +  QString::number( m_toc.discId(), 16 );

    if( QFile::exists( file ) ) {
      // found file
      
      QFile f( file );
      if( !f.open( IO_ReadOnly ) ) {
	kdDebug() << "(K3bCddb) Could not open file" << endl;
      }
      else {
	QTextStream t( &f );
	
	K3bCddbEntry entry;
	entry.category = *it;
	entry.discid = QString::number( m_toc.discId(), 16 );
	parseEntry( t, entry );
	m_query.addEntry( entry );
      }
    }
    else {
      kdDebug() << "(K3bCddb) Could not find local entry" << endl;
    }
  }

  if( m_query.foundEntries() > 0 ) {
    m_error = SUCCESS;
    emit queryFinished( this );
  }
  else {
    m_iCurrentLocalDir++;
    QTimer::singleShot( 0, this, SLOT(searchLocalDir()) );
  }
}


QString K3bCddb::queryString() const
{
  QString query;
  query.sprintf( "cddb query %08x %d", m_toc.discId(), m_toc.count() );
  
  for( K3bToc::const_iterator it = m_toc.begin(); it != m_toc.end(); ++it ) {
    query.append( QString( " %1" ).arg( (*it).firstSector() ) );
  }
  
  query.append( QString( " %1" ).arg( m_toc.length() / 75 ) );
  
  kdDebug() << "(K3bCddb) Query: " + query << endl;

  return query;
}


QString K3bCddb::handshakeString() const
{
  QString user( getenv("USER") );
  QString host( getenv("HOST") );
  if( user.isEmpty() )
    user = "kde-user";
  if( host.isEmpty() )
    host = "kde-host";
  
  return QString("%1 %2 k3b %3").arg(user).arg(host).arg(VERSION);
}


QString K3bCddb::createHttpUrl( unsigned int i )
{
  if( i >= m_httpServer.count() )
    return "";

  QString server;
  int port;

  if( !splitServerPort( m_httpServer[i], server, port ) )
    return "";

  if( !server.startsWith( "http://" ) )
    server.prepend( "http://" );
  
  return QString( "%1/%2" ).arg(server).arg( m_bUseManualCgiPath ? m_cgiPath : QString("~cddb/cddb.cgi") );
}


bool K3bCddb::connectToHttpServer( unsigned int i )
{
  if( i >= m_httpServer.count() )
    return false;

  QString server;
  int port;

  if( !splitServerPort( m_httpServer[i], server, port ) )
    return false;

  if( m_bUseKdeSettings ) {
    if( KProtocolManager::useProxy() ) {
      KURL u( KProtocolManager::proxyFor( "http" ) );

      server = u.host();
      port = u.port();
    }
  }
  else {
    if( m_bUseProxyServer ) {
      server = m_proxyServer;
      port = m_proxyPort;
    }
  }

  m_currentlyConnectingServer = server;

  m_socket->connectToHost( server, port );
  emit infoMessage( i18n("Searching %1 on port %2").arg(server).arg(port) );

  return true;
}


bool K3bCddb::saveLocalEntry( const K3bCddbEntry& entry, const QString& data )
{
  QString path = m_localCddbDirs[0];
  if( path.startsWith( "~" ) )
    path.replace( 0, 1, QDir::homeDirPath() + "/" );
  else if( !path.startsWith( "/" ) )
    path.prepend( QDir::homeDirPath() + "/" );
  if( path[path.length()-1] != '/' )
    path.append( "/" );

  if( QFile::exists( path ) ) {
    // if the category dir does not exists
    // create it

    path += entry.category;

    if( !QFile::exists( path ) ) {
      if( !QDir().mkdir( path ) ) {
	kdDebug() << "(K3bCddb) could not create directory: " << path << endl;
	return false;
      }
    }

    // we always overwrite existing entries
    path += "/" + entry.discid;
    QFile entryFile( path );
    if( !entryFile.open( IO_WriteOnly ) ) {
      kdDebug() << "(K3bCddb) could not create file: " << path << endl;
      return false;
    }
    else {
      kdDebug() << "(K3bCddb) creating file: " << path << endl;
      QTextStream entryStream( &entryFile );
      entryStream << data;
      entryFile.close();
      return true;
    }
  }
  else {
    kdDebug() << "(K3bCddb) could not find directory: " << m_localCddbDirs[0] << endl;
    return false;
  }
}


#include "k3bcddb.moc"

