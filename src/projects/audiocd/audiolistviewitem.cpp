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



#include "audiolistviewitem.h"
#include "k3baudiotrack.h"
#include <k3baudiodecoder.h>


K3bAudioListViewItem::K3bAudioListViewItem( K3bAudioTrack* track, K3bListView* parent )
  : K3bListViewItem( parent ), m_track(track)
{
  init();
}

K3bAudioListViewItem::K3bAudioListViewItem( K3bAudioTrack* track, K3bListView* parent, QListViewItem* after )
  : K3bListViewItem( parent, after ), m_track(track)
{
  init();
}


void K3bAudioListViewItem::init()
{
  animationIconNumber = 1;
  setEditor( 1, LINE );
  setEditor( 2, LINE );
  setEditor( 4, MSF );
}


K3bAudioListViewItem::~K3bAudioListViewItem()
{
}

QString K3bAudioListViewItem::text(int i) const
{
  //
  // We add two spaces after all strings (except the once renamable)
  // to increase readability
  //

  switch( i )
    {
    case 0:
      return QString::number( m_track->index() +1 ).rightJustify( 2, ' ' );
    case 1:
      return m_track->artist();
    case 2:
      return m_track->title();
    case 3:
      return m_track->length().toString() + "  ";
    case 4:
      return m_track->pregap().toString();
    case 5:
      return m_track->module()->fileType() + "  ";
    case 6:
      return m_track->filename();
    default:
      return KListViewItem::text(i);
    }
}

void K3bAudioListViewItem::setText(int col, const QString& text )
{
  if( col == 1 ) {
    // this is the cd-text artist field
    m_track->setArtist( text );
  }
  else if( col == 2 ) {
    // this is the cd-text title field
    m_track->setTitle( text );
  }
  else if( col == 4 ) {
    bool ok;
    int f = text.toInt(&ok);
    if( ok )
      m_track->setPregap( f );
  }

  KListViewItem::setText( col, text );
}

	
QString K3bAudioListViewItem::key( int, bool ) const
{
  // The tracks should be sorted according to their track-number :-)
  // although Red book only supports 99 tracks this goes up to 999 - it does not hurt ;-)

  QString num = QString::number( m_track->index() );
  if( num.length() == 1 )
    return "00" + num;
  else if( num.length() == 2 )
    return "0" + num;
  
  return num;
}
