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
#include "k3baudioproject.h"
#include "k3baudiotrack.h"


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
	switch( i )
	{
		case 0:
			// TODO: think about a really nice solution!
			return QString::number( m_track->index() +1 );
		case 1:
			// track title
			if( m_track->title().isEmpty() || m_track->artist().isEmpty() )
				return m_track->fileName();
			else
				return m_track->artist() + " - " + m_track->title();
		case 2:
			// track length
			return m_track->length().toString();
		case 3:
			// track pregap
			return QString::number( m_track->pregap() );
		case 4:
			// start time
			// TODO: fix it, fix it, fix it....
			return "START";
		case 5:
			// end time
			// TODO: fix it, fix it, fix it....
			return "END";
		default:
			return "xxx";
	}
}

QString AudioListViewItem::key( int column, bool ascending ) const
{
	// The tracks should be sorted according to their track-number :-)
	return text(0);
}
