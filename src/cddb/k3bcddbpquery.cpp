/* 
 *
 * $Id: $
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


/***************************************************************************
                          k3bcddbpquery.cpp  -  description
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


#include "k3bcddbpquery.h"

#include <qstringlist.h>
#include <qsocket.h>
#include <qtextstream.h>

#include <klocale.h>
#include <kdebug.h>


K3bCddbpQuery::K3bCddbpQuery( QObject* parent, const char* name )
  :K3bCddbQuery( parent, name )
{
  m_socket = new QSocket( this );

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

  // TODO: set maximum cddb protocol level (5) with "cddb proto"

  m_state = GREETING;
  m_matches.clear();

  // connect to the server

  m_socket->connectToHost( m_server, m_port );
  emit infoMessage( i18n("Searching %1 on port %2").arg(m_server).arg(m_port) );
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
  QTextStream stream( m_socket );
  stream << "quit\n";
}


void K3bCddbpQuery::slotReadyRead()
{
  QTextStream stream( m_socket );

  while( m_socket->canReadLine() ) {
    QString line = stream.readLine();

    kdDebug() << "(K3bCddbpQuery) line: " << line << endl;

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
	setError( CONNECTION_ERROR );
	m_socket->close();
      }
      break;

    case HANDSHAKE:
      if( getCode( line ) == 200 ) {
	emit infoMessage( i18n("Handshake successful") );

	m_state = PROTO;

	QTextStream stream( m_socket );
	stream << "proto 5\n";
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
	  kdDebug() << "(K3bCddbpQuery) illigal protocol level!" << endl;
	}
	
	// just ignore the reply since it's not important for the functionality
	m_state = QUERY;
	
	QTextStream stream( m_socket );
	stream << queryString() << "\n";
	break;
      }

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

	kdDebug() << "(K3bCddbpQuery) Found exact match: '" << cat << "' '" << discid << "' '" << title << "'" << endl;

	emit infoMessage( i18n("Found exact match") );

	K3bCddbResultEntry entry;
	entry.category = cat;
	entry.discid = discid;
	m_matches.append( entry );

	readFirstEntry();
      }

      else if( getCode( line ) == 210 ) {
	// TODO: perhaps add an "exact" field to K3bCddbEntry
	kdDebug() << "(K3bCddbpQuery) Found multiple exact matches" << endl;

	emit infoMessage( i18n("Found multiple exact matches") );

	m_state = QUERY_DATA;
      }

      else if( getCode( line ) == 211 ) {
	kdDebug() << "(K3bCddbpQuery) Found inexact matches" << endl;

	emit infoMessage( i18n("Found inexact matches") );

	m_state = QUERY_DATA;
      }

      else if( getCode( line ) == 202 ) {
	kdDebug() << "(K3bCddbpQuery) no match found" << endl;
	emit infoMessage( i18n("No match found") );
	setError( NO_ENTRY_FOUND );
	cddbpQuit();
      }

      else {
	kdDebug() << "(K3bCddbpQuery) Error while querying: " << line << endl;
	emit infoMessage( i18n("Error while querying") );
	setError( QUERY_ERROR );
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

	kdDebug() << "(K3bCddbpQuery) inexact match: " << line << endl;

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
	setError( READ_ERROR );
	cddbpQuit();
      }
      break;
    

    case READ_DATA:

      kdDebug() << "(K3bCddbpQuery) parsing line: " << line << endl;

      if( line.startsWith( "." ) ) {
	
	kdDebug() << "(K3bCddbpQuery) query finished." << endl;

	QTextStream strStream( m_parsingBuffer, IO_ReadOnly );
	K3bCddbResultEntry entry = *m_matches.begin();
	parseEntry( strStream, entry );

	queryResult().addEntry( entry );
	m_matches.erase( m_matches.begin() );
	
	if( !readFirstEntry() ) {
	  setError( SUCCESS );
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


bool K3bCddbpQuery::readFirstEntry()
{
  if( m_matches.isEmpty() )
    return false;

  QString read = QString( "cddb read %1 %2").arg( m_matches.first().category ).arg( m_matches.first().discid );
  
  m_state = READ;
  m_parsingBuffer = "";
  
  kdDebug() <<  "(K3bCddbpQuery) Read: " << read << endl;

  QTextStream stream( m_socket );
  stream << read << "\n";

  return true;
}


void K3bCddbpQuery::slotError( int e )
{
  switch(e) {
  case QSocket::ErrConnectionRefused:
    kdDebug() <<  i18n("Connection to %1 refused").arg( m_server ) << endl;
    emit infoMessage( i18n("Connection to %1 refused").arg( m_server ) );
    break;
  case QSocket::ErrHostNotFound:
    kdDebug() <<  i18n("Could not find host %1").arg( m_server ) << endl;
    emit infoMessage( i18n("Could not find host %1").arg( m_server ) );
    break;
  case QSocket::ErrSocketRead:
    kdDebug() <<  i18n("Error while reading from %1").arg( m_server ) << endl;
    emit infoMessage( i18n("Error while reading from %1").arg( m_server ) );
    break;
  }

  m_socket->close();
  emitQueryFinished();
}

#include "k3bcddbpquery.moc"
