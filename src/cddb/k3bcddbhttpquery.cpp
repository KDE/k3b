/* 
 *
 * $Id$
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

  emit infoMessage( i18n("Searching %1 on port %2").arg(m_server).arg(m_port) );

  if( !connectToServer() ) {
    setError( CONNECTION_ERROR );
    emit infoMessage( i18n("Could not connect to host %1").arg(m_server) );
    emitQueryFinished();
  }
}


void K3bCddbHttpQuery::doMatchQuery()
{
  setError( WORKING );
  m_state = READ;

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
    stream << query << endl;
    m_socket->flush();
  }
  else if( m_state == READ ) {
    // send read command for first entry
    QString query = createHttpUrl();
    query.append( QString( "?cmd=cddb+read+%1+%2").arg( header().category ).arg( header().discid ) );
    query.append( "&hello=" );
    query.append( handshakeString().replace( QRegExp( "\\s" ), "+" ) );
    query.append( "&proto=5" );
      
    query.prepend( "GET " );

    m_parsingBuffer = "";
      
    kdDebug() <<  "(K3bCddbHttpQuery) Read: " << query << endl;

    QTextStream stream( m_socket );
    stream << query << endl;
    m_socket->flush();
  }
  else 
    kdDebug() << "(K3bCddbHttpQuery) Wrong state in http mode" << endl;
}


void K3bCddbHttpQuery::slotConnectionClosed()
{
  emit infoMessage( i18n("Connection closed") );

  if( m_state != FINISHED ) {
    // some error occurred
    setError( FAILURE );
    emitQueryFinished();
  }
}


void K3bCddbHttpQuery::slotReadyRead()
{
  QTextStream stream( m_socket );

  while( m_socket->canReadLine() ) {
    QString line = stream.readLine();

    //    kdDebug() << "(K3bCddbHttpQuery) line: " << line << endl;

    switch( m_state ) {

    case QUERY:
      if( getCode( line ) == 200 ) {
	// parse exact match and send a read command
	K3bCddbResultHeader header;
	parseMatchHeader( line.mid(4), header );

	queryMatch( header );
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
	m_state = FINISHED;
	emitQueryFinished();
	m_socket->close();
	return;
      }

      else {
	kdDebug() << "(K3bCddbHttpQuery) Error while querying: " << line << endl;
	emit infoMessage( i18n("Error while querying") );
	setError(QUERY_ERROR);
	m_state = FINISHED;
	emitQueryFinished();
	m_socket->close();
	return;
      }
      break;

    case QUERY_DATA:
      if( line.startsWith( "." ) ) {
	// finished query
	// go on reading


	// here we have the inexact matches headers and should emit the
	// inexactMatches signal
	emit inexactMatches( this );
      }
      else {
	kdDebug() << "(K3bCddbHttpQuery) inexact match: " << line << endl;

	// create a new resultHeader
	K3bCddbResultHeader header;
	parseMatchHeader( line, header );
	m_inexactMatches.append(header);
      }
      break;

    case READ:
      if( getCode( line ) == 210 ) {

	// we just start parsing the read data
	m_state = READ_DATA;
      }

      else {
	emit infoMessage( i18n("Could not read match") );
	setError(READ_ERROR);
	m_state = FINISHED;
	emitQueryFinished();
	m_socket->close();
	return;
      }
      break;
    

    case READ_DATA:

      //      kdDebug() << "parsing line: " << line << endl;

      if( line.startsWith( "." ) ) {
	
	kdDebug() << "(K3bCddbHttpQuery query finished." << endl;

	QTextStream strStream( m_parsingBuffer, IO_ReadOnly );
	parseEntry( strStream, result() );

	setError(SUCCESS);
	m_state = FINISHED;
	emitQueryFinished();
	m_socket->close();
	return;
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
  //  return ( m_socket->startAsyncConnect() == 0 );
  return true;
}


void K3bCddbHttpQuery::setTimeout( int )
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
