/* 
 *
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3bcddbpquery.h"

#include <qstringlist.h>
#include <q3socket.h>
#include <q3textstream.h>

#include <klocale.h>
#include <kdebug.h>




K3bCddbpQuery::K3bCddbpQuery( QObject* parent, const char* name )
  :K3bCddbQuery( parent, name )
{
  m_socket = new Q3Socket( this );
  m_stream.setDevice( m_socket );
  m_stream.setEncoding( Q3TextStream::UnicodeUTF8 );

  connect( m_socket, SIGNAL(connected()), this, SLOT(slotConnected()) );
  connect( m_socket, SIGNAL(hostFound()), this, SLOT(slotHostFound()) );
  connect( m_socket, SIGNAL(connectionClosed()), this, SLOT(slotConnectionClosed()) );
  connect( m_socket, SIGNAL(error(int)), this, SLOT(slotError(int)) );
  connect( m_socket, SIGNAL(readyRead()), this, SLOT(slotReadyRead()) );
}


K3bCddbpQuery::~K3bCddbpQuery()
{
  delete m_socket;
}

void K3bCddbpQuery::doQuery()
{
  setError( WORKING );

  m_state = GREETING;

  // connect to the server

  m_socket->connectToHost( m_server, m_port );
  emit infoMessage( i18n("Searching %1 on port %2",m_server,m_port) );
}


void K3bCddbpQuery::doMatchQuery()
{
  // we should still be connected
  // TODO: check this

  QString read = QString( "cddb read %1 %2").arg( header().category ).arg( header().discid );
  
  m_state = READ;
  m_parsingBuffer = "";
  
  kDebug() <<  "(K3bCddbpQuery) Read: " << read;

  m_stream << read << endl << flush;
}


void K3bCddbpQuery::slotHostFound()
{
  emit infoMessage( i18n("Host found") );
}


void K3bCddbpQuery::slotConnected()
{
  emit infoMessage( i18n("Connected") );
}


void K3bCddbpQuery::slotConnectionClosed()
{
  emit infoMessage( i18n("Connection closed") );
  emitQueryFinished();
}


void K3bCddbpQuery::cddbpQuit()
{
  m_state = QUIT;
  m_stream << "quit" << endl << flush;
}


void K3bCddbpQuery::slotReadyRead()
{
  while( m_socket->canReadLine() ) {
    QString line = m_stream.readLine();

    //    kDebug() << "(K3bCddbpQuery) line: " << line;

    switch( m_state ) {
    case GREETING:
      if( getCode( line ) == 200 || getCode( line ) == 201) {
	emit infoMessage( i18n("OK, read access") );
	m_state = HANDSHAKE;

	m_stream << "cddb hello " << handshakeString() << endl << flush;
      }

      else {
	emit infoMessage( i18n("Connection refused") );
	setError( CONNECTION_ERROR );
	m_socket->close();
      }
      break;

    case HANDSHAKE:
      if( getCode( line ) == 200 ) {
	emit infoMessage( i18n("Handshake successful") );

	m_state = PROTO;

	m_stream << "proto 6" << endl << flush;
      }

      else {
	emit infoMessage( i18n("Handshake failed") );  // server closes connection
	setError( CONNECTION_ERROR );
	m_socket->close(); // just to be sure
      }
      break;

    case PROTO:
      {
	if( getCode( line ) == 501 ) {
	  kDebug() << "(K3bCddbpQuery) illigal protocol level!";
	}
	
	// just ignore the reply since it's not important for the functionality
	m_state = QUERY;
	
	m_stream << queryString() << endl << flush;
	break;
      }

    case QUERY:
      if( getCode( line ) == 200 ) {
	// parse exact match and send a read command
	K3bCddbResultHeader header;
	parseMatchHeader( line.mid( 4 ), header );

	emit infoMessage( i18n("Found exact match") );

	queryMatch( header );
      }

      else if( getCode( line ) == 210 ) {
	// TODO: perhaps add an "exact" field to K3bCddbEntry
	kDebug() << "(K3bCddbpQuery) Found multiple exact matches";

	emit infoMessage( i18n("Found multiple exact matches") );

	m_state = QUERY_DATA;
      }

      else if( getCode( line ) == 211 ) {
	kDebug() << "(K3bCddbpQuery) Found inexact matches";

	emit infoMessage( i18n("Found inexact matches") );

	m_state = QUERY_DATA;
      }

      else if( getCode( line ) == 202 ) {
	kDebug() << "(K3bCddbpQuery) no match found";
	emit infoMessage( i18n("No match found") );
	setError( NO_ENTRY_FOUND );
	cddbpQuit();
      }

      else {
	kDebug() << "(K3bCddbpQuery) Error while querying: " << line;
	emit infoMessage( i18n("Error while querying") );
	setError( QUERY_ERROR );
	cddbpQuit();
      }
      break;

    case QUERY_DATA:
      if( line.startsWith( "." ) ) {
	// finished query
	// go on reading

	emit inexactMatches( this );
	return;
      }
      else {
	kDebug() << "(K3bCddbpQuery) inexact match: " << line;
	K3bCddbResultHeader header;
	parseMatchHeader( line, header );
	m_inexactMatches.append( header );
      }
      break;

    case READ:
      if( getCode( line ) == 210 ) {

	// we just start parsing the read data
	m_state = READ_DATA;
      }

      else {
	emit infoMessage( i18n("Could not read match") );
	setError( READ_ERROR );
	cddbpQuit();
      }
      break;
    

    case READ_DATA:

      //      kDebug() << "(K3bCddbpQuery) parsing line: " << line;

      if( line.startsWith( "." ) ) {
	
	kDebug() << "(K3bCddbpQuery) query finished.";

	Q3TextStream strStream( m_parsingBuffer, QIODevice::ReadOnly );
	parseEntry( strStream, result() );

	setError( SUCCESS );
	cddbpQuit();
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


void K3bCddbpQuery::slotError( int e )
{
  switch(e) {
  case Q3Socket::ErrConnectionRefused:
    kDebug() <<  i18n("Connection to %1 refused", m_server );
    emit infoMessage( i18n("Connection to %1 refused", m_server ) );
    break;
  case Q3Socket::ErrHostNotFound:
    kDebug() <<  i18n("Could not find host %1", m_server );
    emit infoMessage( i18n("Could not find host %1", m_server ) );
    break;
  case Q3Socket::ErrSocketRead:
    kDebug() <<  i18n("Error while reading from %1", m_server );
    emit infoMessage( i18n("Error while reading from %1", m_server ) );
    break;
  }

  m_socket->close();
  emitQueryFinished();
}

#include "k3bcddbpquery.moc"
