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

#include <stdlib.h>

#include <qstring.h>
#include <qvaluelist.h>
#include <qstringlist.h>
#include <qsocket.h>
#include <qtextstream.h>
#include <qregexp.h>
#include <qfile.h>
#include <qdir.h>

#include <klocale.h>
#include <kconfig.h>
#include <kio/job.h>
#include <kio/global.h>

#include "k3bcddb.h"

#include "device/k3btoc.h"
#include "device/k3btrack.h"
#include <kdebug.h>



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

const K3bCddbEntry& K3bCddbQuery::entry( int number ) const
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
    m_bSaveCddbEntriesLocally( true )
{
  m_localCddbDirs.append( ".cddb/" );

  m_socket = new QSocket( this );

  connect( m_socket, SIGNAL(connected()), this, SLOT(slotConnected()) );
  connect( m_socket, SIGNAL(hostFound()), this, SLOT(slotHostFound()) );
  connect( m_socket, SIGNAL(connectionClosed()), this, SLOT(slotConnectionClosed()) );
  connect( m_socket, SIGNAL(error(int)), this, SLOT(slotError(int)) );
  connect( m_socket, SIGNAL(readyRead()), this, SLOT(slotReadyRead()) );
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
  m_bUseProxyServer = c->readBoolEntry( "use proxy server", false );
  m_bSaveCddbEntriesLocally = c->readBoolEntry( "save cddb entries locally", true );

  if( m_localCddbDirs.isEmpty() )
    m_localCddbDirs.append( ".cddb/" );
  if( m_cddbpServer.isEmpty() )
    m_cddbpServer.append( "freedb.org:8880" );
}


void K3bCddb::query( const K3bToc& toc )
{
  m_toc = toc;

  m_error = WORKING;
  m_queryType = CDDBP;

  m_iCurrentQueriedServer = 0;
  queryCdOnServer();
}


void K3bCddb::localQuery( const K3bToc& toc )
{
  m_toc = toc;

  m_error = WORKING;
  m_queryType = LOCAL;

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
  // TODO: add cddb over http

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
    m_socket->connectToHost( server, port );
    emit infoMessage( i18n("Searching %1 on port %2").arg(server).arg(port) );
  }
  else {
    kdDebug() << "(K3bCddb) parsing problem: " << m_cddbpServer[m_iCurrentQueriedServer] << endl;
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
}


void K3bCddb::slotConnectionClosed()
{
  emit infoMessage( i18n("Connection closed") );

  if( m_error == SUCCESS ) {
    emit queryFinished( this );
  }
  else {
    m_iCurrentQueriedServer++;
    queryCdOnServer();
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

	// handshake
	QString user( getenv("USER") );
	QString host( getenv("HOST") );
	if( user.isEmpty() )
	  user = "kde-user";
	if( host.isEmpty() )
	  host = "kde-host";

	QString handshake = QString("cddb hello %1 %2 k3b %3").arg(user).arg(host).arg(VERSION);

	kdDebug() << "(K3bCddb) handshake: " << handshake << endl;

	QTextStream stream( m_socket );
	stream << handshake << "\n";
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

	// query
	QString query;
	query.sprintf( "cddb query %08x %d", m_toc.discId(), m_toc.count() );

	for( K3bToc::const_iterator it = m_toc.begin(); it != m_toc.end(); ++it ) {
	  query.append( QString( " %1" ).arg( (*it).firstSector() ) );
	}

	query.append( QString( " %1" ).arg( m_toc.length() / 75 ) );

	kdDebug() << "(K3bCddb) Query: " << query << endl;

	m_state = QUERY;

	QTextStream stream( m_socket );
	stream << query << "\n";
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

	readFirstEntry();
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
	emit infoMessage( i18n("No match found") );
	cddbpQuit();
      }

      else {
	emit infoMessage( i18n("Error while querying") );
	cddbpQuit();
      }
      break;

    case QUERY_DATA:
      if( line.startsWith( "." ) ) {
	// finished query
	// go on reading

	readFirstEntry();
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
	cddbpQuit();
      }
      break;
    

    case READ_DATA:

      kdDebug() << "parsing line: " << line << endl;

      if( line.startsWith( "." ) ) {
	
	kdDebug() << "(K3bCddb) query finished." << endl;

	QTextStream strStream( m_parsingBuffer, IO_ReadOnly );
	m_query.addEntry( parseEntry( strStream ) );
	m_matches.erase( m_matches.begin() );
	
	
	if( !readFirstEntry() ) {
	  m_error = SUCCESS;
	  cddbpQuit();
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
  
  kdDebug() << "(K3bCddb) Read: " << read << endl;
  
  QTextStream stream( m_socket );
  stream << read << "\n";

  return true;
}


K3bCddbEntry K3bCddb::parseEntry( QTextStream& stream )
{
  K3bCddbEntry entry;

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
	kdDebug() << "set title to: " << line.mid( eqSgnPos+1 ) << " is now: " << entry.titles[trackNum] << endl;
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
      kdDebug() << "(K3bCddb) comment: " << line << endl;
    }
    
    else {
      kdDebug() << "(K3bCddb) Unknown field: " << line << endl;
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

  return entry;
}



void K3bCddb::slotError( int e )
{
  switch(e) {
  case QSocket::ErrConnectionRefused:
    kdDebug() << i18n("Connection to %1 refused").arg( m_cddbpServer[m_iCurrentQueriedServer] ).local8Bit().data() << endl;
    emit infoMessage( i18n("Connection to %1 refused").arg( m_cddbpServer[m_iCurrentQueriedServer] ) );
    break;
  case QSocket::ErrHostNotFound:
    kdDebug() << i18n("Could not find host %1").arg( m_cddbpServer[m_iCurrentQueriedServer] ).local8Bit().data() << endl;
    emit infoMessage( i18n("Could not find host %1").arg( m_cddbpServer[m_iCurrentQueriedServer] ) );
    break;
  case QSocket::ErrSocketRead:
    kdDebug() << i18n("Error while reading from %1").arg( m_cddbpServer[m_iCurrentQueriedServer] ).local8Bit().data() << endl;
    emit infoMessage( i18n("Error while reading from %1").arg( m_cddbpServer[m_iCurrentQueriedServer] ) );
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


  m_localCddbFile = m_localCddbDirs[m_iCurrentLocalDir];
  if( !m_localCddbFile.startsWith( "/" ) )
    m_localCddbFile.prepend( QDir::homeDirPath() + "/" );
  if( m_localCddbFile[m_localCddbFile.length()-1] != '/' )
    m_localCddbFile.append( "/" );

  m_localCddbFile.append( QString::number( m_toc.discId(), 16 ) );

  connect( KIO::stat( KURL(  m_localCddbFile  ), false ),
	   SIGNAL(result(KIO::Job*)),
	   this,
	   SLOT(statJobFinished(KIO::Job*)) );
}


void K3bCddb::statJobFinished( KIO::Job* job )
{
  KIO::StatJob* sj = (KIO::StatJob*)job;

  // Here we could do further KIO::Error checking
  // but that is not really important

  if( sj->error() == 0 ) {
    // found file

    QFile f( m_localCddbFile );
    if( !f.open( IO_ReadOnly ) ) {
      kdDebug() << "(K3bCddb) Could not open file" << endl;
      m_iCurrentLocalDir++;
      searchLocalDir();
    }
    else {
      QTextStream t( &f );
      
      m_query.addEntry( parseEntry( t ) );
      m_error = SUCCESS;
      emit queryFinished( this );
    }
  }
  else {
    kdDebug() << "(K3bCddb) Could not find local entry" << endl;
    m_iCurrentLocalDir++;
    searchLocalDir();
  }
}


#include "k3bcddb.moc"

