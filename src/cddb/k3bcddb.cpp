/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */



#include <qstring.h>
#include <qvaluelist.h>
#include <qstringlist.h>
#include <qtimer.h>

#include <klocale.h>
#include <kconfig.h>
#include <kdebug.h>

#include "k3bcddb.h"
#include "k3bcddbhttpquery.h"
#include "k3bcddbpquery.h"
#include "k3bcddblocalquery.h"
#include "k3bcddblocalsubmit.h"

#include <k3btoc.h>
#include <k3btrack.h>
#include <k3bcddbmultientriesdialog.h>


K3bCddb::K3bCddb( QObject* parent, const char* name )
  : QObject( parent, name )
{
  m_httpQuery = 0;
  m_cddbpQuery = 0;
  m_localQuery = 0;
  m_localSubmit = 0;

  m_lastUsedQuery = 0;
}


K3bCddb::~K3bCddb()
{
}


void K3bCddb::readConfig( KConfig* c )
{
  c->setGroup( "Cddb" );

  m_bRemoteCddbQuery = c->readBoolEntry( "use remote cddb", false );
  m_bLocalCddbQuery = c->readBoolEntry( "use local cddb query", false );

  // old config <= 0.7.3
  QStringList cddbpServer = c->readListEntry( "cddbp server" );
  QStringList httpServer = c->readListEntry( "http server" );

  // new config
  m_cddbServer = c->readListEntry( "cddb server" );

  m_localCddbDirs = c->readPathListEntry( "local cddb dirs" );
  m_proxyServer = c->readEntry( "proxy server" );
  m_proxyPort = c->readNumEntry( "proxy port" );
  m_bUseProxyServer = c->readBoolEntry( "use proxy server", false );
  m_bUseManualCgiPath = c->readBoolEntry( "use manual cgi path", false );
  m_cgiPath = c->readEntry( "cgi path", "~cddb/cddb.cgi" );
  m_bUseKdeSettings = ( c->readEntry( "proxy settings type", "kde" ) == "kde" );

  if( m_localCddbDirs.isEmpty() )
    m_localCddbDirs.append( "~/.cddb/" );

  // old config <= 0.7.3
  if( !httpServer.isEmpty() ) {
    for( QStringList::iterator it = httpServer.begin(); it != httpServer.end(); ++it ) {
      m_cddbServer.append( "Http " + *it );
    }
  }
  if( !cddbpServer.isEmpty() ) {
    for( QStringList::iterator it = cddbpServer.begin(); it != cddbpServer.end(); ++it ) {
      m_cddbServer.append( "Cddbp " + *it );
    }
  }

  if( m_cddbServer.isEmpty() )
    m_cddbServer.append( "Http freedb.org:80" );
}


void K3bCddb::query( const K3bToc& toc )
{
  m_toc = toc;

  // make sure we have a valid discId
  m_toc.calculateDiscId();

  if( m_bLocalCddbQuery ) {
    m_iCurrentQueriedLocalDir = 0;
    localQuery();
  }
  else if( m_bRemoteCddbQuery ) {
    m_iCurrentQueriedServer = 0;
    remoteQuery();
  }
}


void K3bCddb::remoteQuery()
{
  K3bCddbQuery* q = getQuery( m_cddbServer[m_iCurrentQueriedServer] );
  q->query(m_toc);
}


void K3bCddb::slotMultibleMatches( K3bCddbQuery* query )
{
  query->queryMatch( K3bCddbMultiEntriesDialog::selectCddbEntry( query, 0 ) );
}


void K3bCddb::slotQueryFinished( K3bCddbQuery* query )
{
  m_lastUsedQuery = query;

  if( query->error() == K3bCddbQuery::SUCCESS ) {
    m_lastResult = m_lastUsedQuery->result();

    // make sure the result has the requested discid since otherwise local saving does not make much sense
    m_lastResult.discid = QString::number( m_toc.discId(), 16 );

    emit queryFinished( K3bCddbQuery::SUCCESS );
  }
  else if( query == m_localQuery ) {
    m_iCurrentQueriedLocalDir++;
    if( m_iCurrentQueriedLocalDir < m_localCddbDirs.size() )
      localQuery();
    else if( m_bRemoteCddbQuery ) {
      m_iCurrentQueriedServer = 0;
      remoteQuery();
    }
    else {
      emit queryFinished( query->error() );
    }
  }
  else {
    m_iCurrentQueriedServer++;
    if( m_iCurrentQueriedServer < m_cddbServer.size() ) {
      remoteQuery();
    }
    else {
      emit queryFinished( query->error() );
    }
  }
}


K3bCddbQuery* K3bCddb::getQuery( const QString& s )
{
  QStringList buf = QStringList::split( ":", s.mid( s.find(" ")+1 ) );
  QString server = buf[0];
  int port = buf[1].toInt();

  if( s.startsWith("Http") ) {
    if( !m_httpQuery ) {
      m_httpQuery = new K3bCddbHttpQuery( this );
      connect( m_httpQuery, SIGNAL(infoMessage(const QString&)),
	       this, SIGNAL(infoMessage(const QString&)) );
      connect( m_httpQuery, SIGNAL(queryFinished(K3bCddbQuery*)),
	       this, SLOT(slotQueryFinished(K3bCddbQuery*)) );
      connect( m_httpQuery, SIGNAL(inexactMatches(K3bCddbQuery*)),
	       this, SLOT(slotMultibleMatches(K3bCddbQuery*)) );
    }

    m_httpQuery->setServer( server, port );
    m_httpQuery->setUseProxy( m_bUseProxyServer );
    m_httpQuery->setProxy( m_proxyServer, m_proxyPort );
    m_httpQuery->setUseKdeProxySettings( m_bUseKdeSettings );
    m_httpQuery->setCgiPath( m_bUseManualCgiPath ? m_cgiPath : QString::fromLatin1("~cddb/cddb.cgi") );
    //    m_httpQuery->setTimeout( m_timeout );

    return m_httpQuery;
  }
  else {
    if( !m_cddbpQuery ) {
      m_cddbpQuery = new K3bCddbpQuery( this );
      connect( m_cddbpQuery, SIGNAL(infoMessage(const QString&)),
	       this, SIGNAL(infoMessage(const QString&)) );
      connect( m_cddbpQuery, SIGNAL(queryFinished(K3bCddbQuery*)),
	       this, SLOT(slotQueryFinished(K3bCddbQuery*)) );
      connect( m_cddbpQuery, SIGNAL(inexactMatches(K3bCddbQuery*)),
	       this, SLOT(slotMultibleMatches(K3bCddbQuery*)) );
    }

    m_cddbpQuery->setServer( server, port );

    return m_cddbpQuery;
  }
}


void K3bCddb::localQuery()
{
  if( !m_localQuery ) {
    m_localQuery = new K3bCddbLocalQuery( this );
    connect( m_localQuery, SIGNAL(infoMessage(const QString&)),
	     this, SIGNAL(infoMessage(const QString&)) );
    connect( m_localQuery, SIGNAL(queryFinished(K3bCddbQuery*)),
	     this, SLOT(slotQueryFinished(K3bCddbQuery*)) );
    connect( m_localQuery, SIGNAL(inexactMatches(K3bCddbQuery*)),
	     this, SLOT(slotMultibleMatches(K3bCddbQuery*)) );
  }
  
  m_localQuery->setCddbDir( m_localCddbDirs[m_iCurrentQueriedLocalDir] );
  
  m_localQuery->query( m_toc );
}


QString K3bCddb::errorString() const
{
  if( !m_lastUsedQuery )
    return "no query";

  switch( m_lastUsedQuery->error() ) {
  case K3bCddbQuery::SUCCESS:
    return i18n("Found freedb entry.");
  case K3bCddbQuery::NO_ENTRY_FOUND:
    return i18n("No entry found");
  case K3bCddbQuery::CONNECTION_ERROR:
    return i18n("Error while connecting to host.");
  case K3bCddbQuery::WORKING:
    return i18n("Working...");
  case K3bCddbQuery::QUERY_ERROR:
  case K3bCddbQuery::READ_ERROR:
  case K3bCddbQuery::FAILURE:
  default:
    return i18n("Communication error.");
  }
}


const K3bCddbResultEntry& K3bCddb::result() const
{
  //  return m_lastUsedQuery->result();
  return m_lastResult;
}


void K3bCddb::saveEntry( const K3bCddbResultEntry& entry )
{
  if( !m_localSubmit ) {
    m_localSubmit = new K3bCddbLocalSubmit( this );
    connect( m_localSubmit, SIGNAL(submitFinished(K3bCddbSubmit*)),
	     this, SLOT(slotSubmitFinished(K3bCddbSubmit*)) );
  }
  
  m_localSubmit->setCddbDir( m_localCddbDirs[0] );
  
  m_localSubmit->submit( entry );
}


void K3bCddb::slotSubmitFinished( K3bCddbSubmit* s )
{
  emit submitFinished( s->error() == K3bCddbSubmit::SUCCESS );
}

#include "k3bcddb.moc"

