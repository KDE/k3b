/* 
 *
 * $Id$
 * Copyright (C) 2004 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bartsaudioserver.h"
#include "k3baudioclient.h"

#include <artsc/artsc.h>
#include <kdebug.h>
#include <qthread.h>



class K3bArtsAudioServer::Private : public QThread
{
public:
  Private( K3bAudioClient* c )
    : QThread(),
      client( c ),
      canceled(false) {
  }

  void run() {
    stream = arts_play_stream(44100, 16, 2, "K3bArtsAudioServer");
    char buffer[2352];
    bool error = false;
    int read = 0;

    while( !error && !canceled ) {
      read = client->read( buffer, 2352 );
      if( read > 0 ) {
	int errorcode = arts_write( stream, buffer, 2352 );
	if( errorcode < 0 ) {
	  kdDebug() << "arts_write failed: " << arts_error_text(errorcode) << endl;
	  error = true;
	}
      }
      else if( read == 0 )
	break;
      else {
	error = true;
	break;
      }
    }

    if( error )
      kdDebug() << "(K3bArtsAudioServer) An error occured." << endl;

    arts_close_stream(stream);
  }

  arts_stream_t stream;
  K3bAudioClient* client;
  bool canceled;
};

K3bArtsAudioServer::K3bArtsAudioServer( QObject* parent, const char* name )
  : K3bAudioServer( parent, name ),
    m_initialized(false)
{
}


K3bArtsAudioServer::~K3bArtsAudioServer()
{
  cleanup();
}


void K3bArtsAudioServer::attachClient( K3bAudioClient* c )
{
  if( !arts_init() ) {
    m_initialized = true;
    Private* p = new Private( c );
    m_clientMap[c] = p;
    p->start();
  }
  else
    kdDebug() << "(K3bArtsAudioServer::attachClient) unable to connect to arts sound server." << endl;
}


void K3bArtsAudioServer::detachClient( K3bAudioClient* c )
{
  QMap<K3bAudioClient*, Private*>::iterator it = m_clientMap.find( c );
  if( it != m_clientMap.end() ) {
    Private* p = it.data();
    p->canceled = true;
    p->wait();
    delete p;
    m_clientMap.remove(it);
  }

  if( m_clientMap.empty() )
    cleanup();
}


void K3bArtsAudioServer::cleanup()
{
  if( m_initialized ) {
    // TODO: cleanup client map
    arts_free();
    m_initialized = false;
  }
}


bool K3bArtsAudioServer::artsAvailable()
{
  int e = arts_init();
  if( e == 0 )
    arts_free();
  return ( e == 0 );
}
