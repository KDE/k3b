/***************************************************************************
                          audiolistviewitem.cpp  -  description
                             -------------------
    begin                : Tue Mar 27 2001
    copyright            : (C) 2001 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

// QT-includes

#include "audiolistviewitem.h"
#include "k3baudiotrack.h"
#include "../k3bglobals.h"


K3bAudioListViewItem::K3bAudioListViewItem( K3bAudioTrack* track, QListView* parent )
  : KListViewItem( parent )
{
  m_track = track;
  animationIconNumber = 1;
}

K3bAudioListViewItem::K3bAudioListViewItem( K3bAudioTrack* track, QListView* parent, QListViewItem* after )
  : KListViewItem( parent, after )
{
  m_track = track;
  animationIconNumber = 1;
}

K3bAudioListViewItem::~K3bAudioListViewItem()
{
}

QString K3bAudioListViewItem::text(int i) const
{
  switch( i )
    {
    case 0:
      return QString::number( m_track->index() +1 );
    case 1:
      return m_track->artist();
    case 2:
      return m_track->title();
    case 3:
      // track length
      return K3b::framesToString( m_track->length() );
    case 4:
      // track pregap
      return K3b::framesToString( m_track->pregap() );
    case 5:
      return m_track->fileName();
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
