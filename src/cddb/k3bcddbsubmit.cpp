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


#include "k3bcddbsubmit.h"

#include <qtimer.h>


K3bCddbSubmit::K3bCddbSubmit( QObject* parent, const char* name )
  : QObject( parent, name )
{
}


K3bCddbSubmit::~K3bCddbSubmit()
{
}


void K3bCddbSubmit::submit( const K3bCddbResultEntry& entry )
{
  m_resultEntry = entry;

  if( m_resultEntry.rawData.isEmpty() )
    createDataStream( m_resultEntry );

  QTimer::singleShot( 0, this, SLOT(doSubmit()) );
}


void K3bCddbSubmit::createDataStream( K3bCddbResultEntry& entry )
{
  entry.rawData.truncate(0);
  
  QTextStream ts( &entry.rawData, IO_WriteOnly );

  ts << "#" << endl
     << "# Submitted via: K3b" << endl
     << "#" << endl;

  ts << "DISCID=" << entry.discid << endl
     << "DTITLE=" << entry.cdArtist << " / " << entry.cdTitle << endl
     << "DYEAR=";
  if( entry.year > 0 )
    ts << entry.year;
  ts << endl;
  ts << "DGENRE=" << entry.genre << endl;

  bool allEqualArtist = true;
  for( unsigned int i = 0; i < entry.artists.count(); ++i )
    if( entry.artists[i] != entry.cdArtist &&
	!entry.artists[i].isEmpty() ) {
      allEqualArtist = false;
      break;
    }

  for( unsigned int i = 0; i < entry.titles.count(); ++i ) {
    ts << "TTITLE" << i << "=";
    if( !allEqualArtist )
      ts << entry.artists[i] << " / ";
    ts << entry.titles[i] << endl;
  }

  ts << "EXTD=" << entry.cdExtInfo << endl;

  for( unsigned int i = 0; i < entry.titles.count(); ++i ) {
    ts << "EXTT" << i << "=" << entry.extInfos[i] << endl;
  }
}

#include "k3bcddbsubmit.moc"
