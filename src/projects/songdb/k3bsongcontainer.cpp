/* 
 *
 * $Id$
 * Copyright (C) 2002 Thomas Froescher <tfroescher@k3b.org>
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


#include "k3bsongcontainer.h"
#include "k3bsong.h"

#include <kdebug.h>


K3bSongContainer::K3bSongContainer( const QString& path)
  : m_path( path )
{
  m_songs.setAutoDelete(true);
}

K3bSongContainer::K3bSongContainer()
{
}

K3bSongContainer::~K3bSongContainer()
{
}

void K3bSongContainer::deleteSong( const QString& filename )
{
  for( QPtrListIterator<K3bSong> it( m_songs ); *it; ++it ){
    if( filename == it.current()->getFilename() ) {
      m_songs.removeRef( it.current() );
      break;
    }
  }
}

K3bSong* K3bSongContainer::addSong( K3bSong* song )
{
  deleteSong( song->getFilename() );
  m_songs.append( song );
  return song;
}


bool K3bSongContainer::isEmpty() const
{
  return m_path.isEmpty();
}

K3bSong* K3bSongContainer::findSong( const QString& filename ) const
{
  for( QPtrListIterator<K3bSong> it( m_songs ); *it; ++it )
    if( filename == it.current()->getFilename() ) {
      kdDebug() << "(K3bSongContainer) found song: " << it.current()->getFilename() << endl;
      return it.current();
    }
  
  return 0;
}
