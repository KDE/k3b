/* 
 *
 * $Id$
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

#include <config.h>

#include "k3bartsoutputplugin.h"
#include <k3bpluginfactory.h>

#include <kdebug.h>


K_EXPORT_COMPONENT_FACTORY( libk3bartsoutputplugin, K3bPluginFactory<K3bArtsOutputPlugin>( "k3bartsoutputplugin" ) )


K3bArtsOutputPlugin::K3bArtsOutputPlugin( QObject* parent, const char* name )
  : K3bAudioOutputPlugin( parent, name ),
    m_initialized(false),
    m_lastErrorCode(0)
{
}


K3bArtsOutputPlugin::~K3bArtsOutputPlugin()
{
  cleanup();
}


int K3bArtsOutputPlugin::write( char* data, int len )
{
  for( int i = 0; i < len-1; i+=2 ) {
    char b = data[i];
    data[i] = data[i+1];
    data[i+1] = b;
  }

  m_lastErrorCode = arts_write( m_stream, data, len );

  if( m_lastErrorCode < 0 )
    return -1;
  else
    return len;
}


void K3bArtsOutputPlugin::cleanup()
{
  if( m_initialized ) {
    arts_close_stream( m_stream );
    kdDebug() << "(K3bArtsOutputPlugin::cleanup) arts_free()" << endl;
    arts_free();
    kdDebug() << "(K3bArtsOutputPlugin::cleanup) arts_free() done" << endl;
    m_initialized = false;
  }
}


bool K3bArtsOutputPlugin::init()
{
  kdDebug() << "(K3bArtsOutputPlugin::init)" << endl;
  if( !m_initialized ) {
    kdDebug() << "(K3bArtsOutputPlugin::init) arts_init()" << endl;
    m_lastErrorCode = arts_init();
    m_initialized = ( m_lastErrorCode == 0 );
    kdDebug() << "(K3bArtsOutputPlugin::init) arts_init() done: " << m_lastErrorCode << endl;
    if( m_initialized )
      m_stream = arts_play_stream( 44100, 16, 2, "K3bArtsOutputPlugin" );
  }
  
  return m_initialized;
}


QString K3bArtsOutputPlugin::lastErrorMessage() const
{
  return QString::fromLocal8Bit( arts_error_text(m_lastErrorCode) );
}

