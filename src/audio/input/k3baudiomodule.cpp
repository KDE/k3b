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


#include "k3baudiomodule.h"

#include <kdebug.h>


K3bAudioModule::K3bAudioModule( QObject* parent, const char* name )
  : QObject( parent, name )
{
}


K3bAudioModule::~K3bAudioModule()
{
}


bool K3bAudioModule::initDecoding( const QString& filename, unsigned long trackSize )
{
  m_alreadyDecoded = 0;
  m_size = trackSize;

  return initDecodingInternal( filename );
}


int K3bAudioModule::decode( const char** _data )
{
  if( m_alreadyDecoded >= m_size )
    return 0;

  int len = decodeInternal( _data );
  if( len < 0 )
    return -1;

  else if( len == 0 ) {
    // check if we need to pad
    int bytesToPad = m_size - m_alreadyDecoded;
    if( bytesToPad > 0 ) {
      kdDebug() << "(K3bAudioModule) track length: " << m_size
		<< "; decoded module data: " << m_alreadyDecoded
		<< "; we need to pad " << bytesToPad << " bytes." << endl;
      m_data.resize( bytesToPad );
      m_data.fill(0);
      *_data = m_data.data();
      m_alreadyDecoded += bytesToPad;
      return bytesToPad;
    }
    else
      return 0;
  }
  else {
    // check if we decoded too much
    if( m_alreadyDecoded + len > m_size ) {
      kdDebug() << "(K3bAudioModule) we decoded too much. Cutting output." << endl;
      len = m_size - m_alreadyDecoded;
    }

    m_alreadyDecoded += len;
    return len;
  }
}


void K3bAudioModule::cleanup()
{
}

#include "k3baudiomodule.moc"
