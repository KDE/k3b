/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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



#include "k3bcddbhttpquery.h"

#include "k3bcddbresult.h"

#include <qstringlist.h>
#include <qregexp.h>
#include <q3textstream.h>

#include <klocale.h>
#include <kdebug.h>
#include <kio/global.h>
#include <kio/job.h>


K3bCddbHttpQuery::K3bCddbHttpQuery( QObject* parent, const char* name )
  : K3bCddbQuery( parent, name )
{
  m_server = "freedb.org";
  m_port = 80;
  m_cgiPath = "/~cddb/cddb.cgi";
}


K3bCddbHttpQuery::~K3bCddbHttpQuery()
{
}


void K3bCddbHttpQuery::doQuery()
{
  setError( WORKING );
  m_state = QUERY;

  performCommand( queryString() );
}


void K3bCddbHttpQuery::doMatchQuery()
{
  setError( WORKING );
  m_state = READ;
  m_parsingBuffer.truncate(0);

  performCommand( QString( "cddb read %1 %2").arg( header().category ).arg( header().discid ) );
}


void K3bCddbHttpQuery::performCommand( const QString& cmd )
{
  KUrl url;
  url.setProtocol( "http" );
  url.setHost( m_server );
  url.setPort( m_port );
  url.setPath( m_cgiPath );

  url.addQueryItem( "cmd", cmd );
  url.addQueryItem( "hello", handshakeString() );
  url.addQueryItem( "proto", "6" );

  m_data.truncate(0);

  kDebug() << "(K3bCddbHttpQuery) getting url: " << url.prettyUrl() << endl;

  KIO::TransferJob* job = KIO::get( url, KIO::NoReload, KIO::HideProgressInfo );

  if( !job ) {
    setError( CONNECTION_ERROR );
    emit infoMessage( i18n("Could not connect to host %1").arg(m_server) );
    emitQueryFinished();
    return;
  }

  connect( job, SIGNAL(data(KIO::Job*, const QByteArray&)),
	   SLOT(slotData(KIO::Job*, const QByteArray&)) );
  connect( job, SIGNAL(result(KIO::Job*)),
	   SLOT(slotResult(KIO::Job*)) );
}



void K3bCddbHttpQuery::slotData( KIO::Job*, const QByteArray& data )
{
  if( data.size() ) {
    QDataStream stream( m_data, QIODevice::WriteOnly | QIODevice::Append );
    stream.writeRawBytes( data.data(), data.size() );
  }
}


void K3bCddbHttpQuery::slotResult( KIO::Job* job )
{
  if( job->error() ) {
    emit infoMessage( job->errorString() );
    setError( CONNECTION_ERROR );
    emitQueryFinished();
    return;
  }

  QStringList lines = QStringList::split( "\n", QString::fromUtf8( m_data.data(), m_data.size() ) );

  for( QStringList::const_iterator it = lines.begin(); it != lines.end(); ++it ) {
    QString line = *it;

    //    kDebug() << "(K3bCddbHttpQuery) line: " << line << endl;

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
	kDebug() << "(K3bCddbHttpQuery) Found multiple exact matches" << endl;

	emit infoMessage( i18n("Found multiple exact matches") );

	m_state = QUERY_DATA;
      }

      else if( getCode( line ) == 211 ) {
	kDebug() << "(K3bCddbHttpQuery) Found inexact matches" << endl;

	emit infoMessage( i18n("Found inexact matches") );

	m_state = QUERY_DATA;
      }

      else if( getCode( line ) == 202 ) {
	kDebug() << "(K3bCddbHttpQuery) no match found" << endl;
	emit infoMessage( i18n("No match found") );
	setError(NO_ENTRY_FOUND);
	m_state = FINISHED;
	emitQueryFinished();
	return;
      }

      else {
	kDebug() << "(K3bCddbHttpQuery) Error while querying: " << line << endl;
	emit infoMessage( i18n("Error while querying") );
	setError(QUERY_ERROR);
	m_state = FINISHED;
	emitQueryFinished();
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
	kDebug() << "(K3bCddbHttpQuery) inexact match: " << line << endl;

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
	return;
      }
      break;
    

    case READ_DATA:

      //      kDebug() << "parsing line: " << line << endl;

      if( line.startsWith( "." ) ) {
	
	kDebug() << "(K3bCddbHttpQuery query finished." << endl;

	Q3TextStream strStream( m_parsingBuffer, QIODevice::ReadOnly );
	parseEntry( strStream, result() );

	setError(SUCCESS);
	m_state = FINISHED;
	emitQueryFinished();
	return;
      }

      else {
	m_parsingBuffer.append(line + "\n");
      }
      break;
    }
  }
}


#include "k3bcddbhttpquery.moc"
