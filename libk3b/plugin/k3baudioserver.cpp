/*
 *
 * Copyright (C) 2004 Sebastian Trueg <trueg@k3b.org>
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

#include <k3bcore.h>
#include <k3bthread.h>
#include <k3bprogressinfoevent.h>
#include "k3baudioserver.h"
#include "k3baudioclient.h"
#include "k3bpluginmanager.h"
#include "k3baudiooutputplugin.h"

#include <kdebug.h>
#include <kapplication.h>
#include <klocale.h>
#include <kmessagebox.h>

#include <k3bthread.h>
//Added by qt3to4:
#include <Q3CString>
#include <QCustomEvent>
#include <Q3PtrList>



K3bAudioServer* K3bAudioServer::s_instance = 0;



class K3bAudioServer::Private : public K3bThread
{
public:
  Private( K3bAudioServer* s )
    : m_streaming(false),
      m_server(s) {
    setProgressInfoEventHandler( m_server );
  }


  void stop() {
    m_streaming = false;
  }

protected:
  void run() {
    m_streaming = true;

    char buffer[2048*10];

    while( m_streaming ) {
      int len = m_server->m_client->read( buffer, 2048*10 );
      if( len > 0 ) {
	if( m_server->m_pluginInitialized ) {
	  if( write( buffer, len ) < 0 ) {
	    kDebug() << "Audio Streaming failed: " << m_server->m_usedOutputPlugin->lastErrorMessage();
	    emitInfoMessage( m_server->m_usedOutputPlugin->lastErrorMessage(), 0 );
	    return;
	  }
	}
	// else just drop the data into space...
      }
      else {
	// FIXME: no data or error... what to do?
      }
    }
  }

  int write( char* buffer, int len ) {
    int written = m_server->m_usedOutputPlugin->write( buffer, len );
    return written;
//     if( written < 0 )
//       return -1;
//     else if( written < len )
//       return write( buffer+written, len-written ) + written;
//     else
//       return len;
  }

private:
  bool m_streaming;
  K3bAudioServer* m_server;
};


K3bAudioServer::K3bAudioServer( QObject* parent, const char* name )
  : QObject( parent, name ),
    m_usedOutputPlugin(0),
    m_pluginInitialized(false),
    m_client(0)
{
  s_instance = this;
  d = new Private( this );
}


K3bAudioServer::~K3bAudioServer()
{
  delete d;
  s_instance = 0;
}


bool K3bAudioServer::setOutputMethod( const Q3CString& name )
{
  if( K3bAudioOutputPlugin* p = findOutputPlugin( name ) ) {
    setOutputPlugin( p );
    return true;
  }
  else
    return false;
}


void K3bAudioServer::setOutputPlugin( K3bAudioOutputPlugin* p )
{
  if( p != m_usedOutputPlugin ) {
    bool restart = d->running();
    if( restart ) {
      d->stop();
      d->wait();
    }

    if( m_usedOutputPlugin ) {
      m_usedOutputPlugin->cleanup();
      m_pluginInitialized = false;
    }

    m_usedOutputPlugin = p;

    if( restart )
      d->start();
  }
}


void K3bAudioServer::attachClient( K3bAudioClient* c )
{
  // for now we simply allow only one client and stop the old one
  if( m_client ) {
    kDebug() << "(K3bAudioServer) leaving old client hanging. :(";
    detachClient( m_client );
  }

  m_client = c;

  if( m_usedOutputPlugin && !m_pluginInitialized ) {
    if( !m_usedOutputPlugin->init() ) {
      emit error( i18n("Could not initialize Audio Output plugin %1 (%2)")
		  .arg(m_usedOutputPlugin->pluginInfo().name())
		  .arg(m_usedOutputPlugin->lastErrorMessage()) );
    }
    else
      m_pluginInitialized = true;
  }
  else
    kDebug() << "(K3bAudioServer::attachClient) no output plugin selected. Using null output.";

  // start the streaming
  d->start();
}


void K3bAudioServer::detachClient( K3bAudioClient* c )
{
  if( m_client == c ) {
    m_client = 0;

    // stop the streaming
    d->stop();
    d->wait();

    if( m_usedOutputPlugin && m_pluginInitialized ) {
      m_usedOutputPlugin->cleanup();
      m_pluginInitialized = false;
    }
  }
}


K3bAudioOutputPlugin* K3bAudioServer::findOutputPlugin( const Q3CString& name )
{
  QList<K3bPlugin*> fl = k3bcore->pluginManager()->plugins( "AudioOutput" );

  Q_FOREACH( K3bPlugin* plugin, fl ) {
    K3bAudioOutputPlugin* f = dynamic_cast<K3bAudioOutputPlugin*>( plugin );

    if( f && f->soundSystem() == name ) {
      return f;
    }
  }

  kDebug() << "(K3bAudioServer::findOutputPlugin) could not find output plugin " << name;

  return 0;
}


void K3bAudioServer::customEvent( QCustomEvent* e )
{
  if( K3bProgressInfoEvent* be = dynamic_cast<K3bProgressInfoEvent*>(e) ) {
    if( be->type() == K3bProgressInfoEvent::InfoMessage ) {
      emit error( be->firstString() );
    }
  }
}

#include "k3baudioserver.moc"
