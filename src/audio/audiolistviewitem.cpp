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


AudioListViewItem::AudioListViewItem( K3bAudioTrack* track, QListView* parent )
  : QListViewItem( parent )
{
  m_track = track;
}

AudioListViewItem::AudioListViewItem( K3bAudioTrack* track, QListView* parent, QListViewItem* after )
  : QListViewItem( parent, after )
{
  m_track = track;
}

AudioListViewItem::~AudioListViewItem()
{
}

QString AudioListViewItem::text(int i) const
{
  // TODO: think about a really nice solution!
  QString _num = QString::number( m_track->index() +1 );
  if( _num.length() == 1 )
    _num = "0" + _num;
	
  switch( i )
    {
    case 0:
      return _num;
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
      return "";
    }
}

void AudioListViewItem::setText(int col, const QString& text )
{
  if( col == 1 ) {
    // this is the cd-text artist field
    m_track->setArtist( text );
  }
  else if( col == 2 ) {
    // this is the cd-text title field
    m_track->setTitle( text );
  }
//   else if(col == 5) {
//     // track pregap
//     // TODO: find out the max pregap!
// 	int value;
//     if( K3b::parseHundredth( text, value ) )
//       m_track->setPregap(value*75/100);   // frames
//   }
}

	
QString AudioListViewItem::key( int, bool ) const
{
  // The tracks should be sorted according to their track-number :-)
  return text(0);
}
