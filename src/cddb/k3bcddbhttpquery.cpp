/***************************************************************************
                          k3bcddbhttpquery.cpp  -  description
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


#include "k3bcddbhttpquery.h"

#include "k3bcddbresult.h"

#include <qstringlist.h>
#include <qregexp.h>
#include <qtextstream.h>
#include <qsocket.h>

#include <kextsock.h>
#include <klocale.h>
#include <kdebug.h>
#include <kprotocolmanager.h>


K3bCddbHttpQuery::K3bCddbHttpQuery( QObject* parent, const char* name )
  : K3bCddbQuery( parent, name )
{
  m_socket = new QSocket(this);
//   m_socket = new KExtendedSocket();
//   m_socket->enableRead(true);
//   m_socket->setSocketFlags( KExtendedSocket::inetSocket|KExtendedSocket::bufferedSocket );

//   connect( m_socket, SIGNAL(connectionSuccess()), this, SLOT(slotConnected()) );
//   connect( m_socket, SIGNAL(closed(int)), this, SLOT(slotConnectionClosed()) );
//   connect( m_socket, SIGNAL(connectionFailed(int)), this, SLOT(slotConnectionFailed(int)) );
  connect( m_socket, SIGNAL(readyRead()), this, SLOT(slotReadyRead()) );
  connect( m_socket, SIGNAL(error(int)), this, SLOT(slotError(int)) );
  connect( m_socket, SIGNAL(connectionClosed()), this, SLOT(slotConnectionClosed()) );
  connect( m_socket, SIGNAL(connected()), this, SLOT(slotConnected()) );

  m_server = "freedb.org";
  m_port = 80;
  m_cgiPath = "~cddb/cddb.cgi";
  m_bUseProxyServer = false;
  m_bUseKdeSettings = true;
}


K3bCddbHttpQuery::~K3bCddbHttpQuery()
{
  delete m_socket;
}


void K3bCddbHttpQuery::doQuery()
{
  setError( WORKING );
  m_state = QUERY;

  m_matches.clear();

  if( !connectToServer() ) {
    setError( CONNECTION_ERROR );
    emit infoMessage( i18n("Could not connect to host %1").arg(m_server) );
    emitQueryFinished();
  }
}


void K3bCddbHttpQuery::slotConnected()
{
  emit infoMessage( i18n("Connected") );

  if( m_state == QUERY ) {
    // set query
    QString query = createHttpUrl();
    query.append( "?cmd=" );
    query.append( queryString().replace( QRegExp( "\\s" ), "+" ) );
    query.append( "&hello=" );
    query.append( handshakeString().replace( QRegExp( "\\s" ), "+" ) );
    query.append( "&proto=5" );

    query.prepend( "GET " );

    kdDebug() << "(K3bCddbHttpQuery) Query: " + query << endl;

    QTextStream stream( m_socket );
    stream << query << "\n";
    //    m_socket->flush();
  }
  else if( m_state == READ ) {
    // send read command for first entry
    QString query = createHttpUrl();
    query.append( QString( "?cmd=cddb+read+%1+%2").arg( m_matches.first().category ).arg( m_matches.first().discid ) );
    query.append( "&hello=" );
    query.append( handshakeString().replace( QRegExp( "\\s" ), "+" ) );
    query.append( "&proto=5" );
      
    query.prepend( "GET " );

    m_parsingBuffer = "";
      
    kdDebug() <<  "(K3bCddbHttpQuery) Read: " << query << endl;

    QTextStream stream( m_socket );
    stream << query << "\n";
    //    m_socket->flush();
  }
  else 
    kdDebug() << "(K3bCddbHttpQuery) Wrong state in http mode" << endl;
}


void K3bCddbHttpQuery::slotConnectionClosed()
{
  emit infoMessage( i18n("Connection closed") );

  if( m_state == QUERY ) {
    // the query was not successfull

    setError( QUERY_ERROR );
    emitQueryFinished();
  }
  else if( m_state == READ ) {
    // successfull query

    if( m_matches.isEmpty() ) {
      // all matches read.
      // finish
      if( error() != READ_ERROR )
	setError( SUCCESS );
      emitQueryFinished();
    }
    else {
      // connect to host to send the read command
      if( !connectToServer() ) {
	emit infoMessage( i18n("Could not connect to host %1").arg(m_currentlyConnectingServer) );
	setError( CONNECTION_ERROR );
	emitQueryFinished();
      }
    }
  }
}


void K3bCddbHttpQuery::slotReadyRead()
{
  QTextStream stream( m_socket );

  while( m_socket->canReadLine() ) {
    QString line = stream.readLine();

    kdDebug() << "(K3bCddbHttpQuery) line: " << line << endl;

    switch( m_state ) {

    case QUERY:
      if( getCode( line ) == 200 ) {
	// parse exact match and send a read command
	K3bCddbResultEntry entry;
	if( parseExactMatch( line, entry ) )
	  m_matches.append( entry );

	m_state = READ;
      }

      else if( getCode( line ) == 210 ) {
	// TODO: perhaps add an "exact" field to K3bCddbEntry
	kdDebug() << "(K3bCddbHttpQuery) Found multiple exact matches" << endl;

	emit infoMessage( i18n("Found multiple exact matches") );

	m_state = QUERY_DATA;
      }

      else if( getCode( line ) == 211 ) {
	kdDebug() << "(K3bCddbHttpQuery) Found inexact matches" << endl;

	emit infoMessage( i18n("Found inexact matches") );

	m_state = QUERY_DATA;
      }

      else if( getCode( line ) == 202 ) {
	kdDebug() << "(K3bCddbHttpQuery) no match found" << endl;
	emit infoMessage( i18n("No match found") );
	setError(NO_ENTRY_FOUND);
	emitQueryFinished();
	m_socket->close();
	return;
      }

      else {
	kdDebug() << "(K3bCddbHttpQuery) Error while querying: " << line << endl;
	emit infoMessage( i18n("Error while querying") );
	setError(QUERY_ERROR);
	emitQueryFinished();
	m_socket->close();
	return;
      }
      break;

    case QUERY_DATA:
      if( line.startsWith( "." ) ) {
	// finished query
	// go on reading

	m_state = READ;
      }
      else {
	QStringList match = QStringList::split( " ", line );

	kdDebug() << "(K3bCddbHttpQuery) inexact match: " << line << endl;

	K3bCddbResultEntry entry;
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
	setError(READ_ERROR);
	emitQueryFinished();
	m_socket->close();
	return;
      }
      break;
    

    case READ_DATA:

      kdDebug() << "parsing line: " << line << endl;

      if( line.startsWith( "." ) ) {
	
	kdDebug() << "(K3bCddbHttpQuery query finished." << endl;

	QTextStream strStream( m_parsingBuffer, IO_ReadOnly );
	K3bCddbResultEntry entry = *m_matches.begin();
	parseEntry( strStream, entry );

	queryResult().addEntry( entry );
	m_matches.erase( m_matches.begin() );
	
	m_state = READ;
      }

      else {
	m_parsingBuffer.append(line + "\n");
      }
      break;
    }
  }
}



QString K3bCddbHttpQuery::createHttpUrl()
{
  return QString( "http://%1/%2" ).arg(m_server).arg( m_cgiPath );
}


bool K3bCddbHttpQuery::connectToServer()
{
  QString server = m_server;
  int port = m_port;

  if( m_bUseProxyServer ) {
    if( m_bUseKdeSettings ) {
      if( KProtocolManager::useProxy() ) {
	KURL u( KProtocolManager::proxyFor( "http" ) );
	
	server = u.host();
	port = u.port();
      }
    }
    else {
      server = m_proxyServer;
      port = m_proxyPort;
    }
  }

  m_currentlyConnectingServer = server;

  m_socket->connectToHost( server, port );
  //  m_socket->setAddress( server, port );
  emit infoMessage( i18n("Searching %1 on port %2").arg(server).arg(port) );
  //  return ( m_socket->startAsyncConnect() == 0 );
  return true;
}


void K3bCddbHttpQuery::setTimeout( int t )
{
  //m_socket->setTimeout(t);
}


void K3bCddbHttpQuery::slotConnectionFailed( int )
{
//   switch(e) {
//   case QSocket::ErrConnectionRefused:
//     kdDebug() <<  i18n("Connection to %1 refused").arg( m_currentlyConnectingServer ) << endl;
//     emit infoMessage( i18n("Connection to %1 refused").arg( m_currentlyConnectingServer ) );
//     break;
//   case QSocket::ErrHostNotFound:
//     kdDebug() <<  i18n("Could not find host %1").arg( m_currentlyConnectingServer ) << endl;
//     emit infoMessage( i18n("Could not find host %1").arg( m_currentlyConnectingServer ) );
//     break;
//   case QSocket::ErrSocketRead:
//     kdDebug() <<  i18n("Error while reading from %1").arg( m_currentlyConnectingServer ) << endl;
//     emit infoMessage( i18n("Error while reading from %1").arg( m_currentlyConnectingServer ) );
//     break;
//   }

  setError( CONNECTION_ERROR );
  emitQueryFinished();
}


void K3bCddbHttpQuery::slotError( int e )
{
  switch(e) {
  case QSocket::ErrConnectionRefused:
    kdDebug() <<  i18n("Connection to %1 refused").arg( m_currentlyConnectingServer ) << endl;
    emit infoMessage( i18n("Connection to %1 refused").arg( m_currentlyConnectingServer ) );
    setError( CONNECTION_ERROR );
    break;
  case QSocket::ErrHostNotFound:
    kdDebug() <<  i18n("Could not find host %1").arg( m_currentlyConnectingServer ) << endl;
    emit infoMessage( i18n("Could not find host %1").arg( m_currentlyConnectingServer ) );
    setError( CONNECTION_ERROR );
    break;
  case QSocket::ErrSocketRead:
    kdDebug() <<  i18n("Error while reading from %1").arg( m_currentlyConnectingServer ) << endl;
    emit infoMessage( i18n("Error while reading from %1").arg( m_currentlyConnectingServer ) );
    break;
  }

  m_socket->close();
  emitQueryFinished();
}

#include "k3bcddbhttpquery.moc"
