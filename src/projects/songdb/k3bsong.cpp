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


#include "k3bsong.h"
#include "k3bsongmanager.h"


K3bSong::K3bSong( const QString& filename, 
		  const QString& album, 
		  const QString& artist, 
		  const QString& title, 
		  const QString& discId, 
		  int trackNumber)
  : m_filename( filename ),
    m_album( album ),
    m_artist( artist ),
    m_title( title ),
    m_discId( discId ),
    m_trackNumber( trackNumber ) 
{
}

K3bSong::K3bSong()
{
}

K3bSong::~K3bSong()
{
}


void K3bSong::addContent( const QString& tag, const QString& content )
{
  if( tag == CONTENT_TITLE ) {
    m_title = content;
  } 
  else if( tag == CONTENT_ARTIST ) {
    m_artist = content;
  }
  else if( tag == CONTENT_ALBUM ) {
    m_album = content;
  }
}
