/***************************************************************************
                          k3baudiotrack.cpp  -  description
                             -------------------
    begin                : Wed Mar 28 2001
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

#include "k3baudiotrack.h"
#include "../k3bglobals.h"

#include <qstring.h>
#include <qfileinfo.h>


K3bAudioTrack::K3bAudioTrack( QList<K3bAudioTrack>* parent, const QString& filename )
: m_file(filename), m_bufferFile(), m_length()
{
	m_parent = parent;
	m_pregap = 2;
	
	if( QFileInfo( m_file ).extension(false).contains("mp3", false) )
		m_filetype = K3b::MP3;
	else
		m_filetype = K3b::WAV;
}

K3bAudioTrack::K3bAudioTrack( const K3bAudioTrack& track )
: m_file( track.absPath() ),	m_bufferFile(), m_artist( track.artist() ),
	m_title( track.title() ), m_length( track.length() )
{
	m_parent = track.m_parent;
	m_pregap  = track.pregap();
	m_filetype = track.filetype();
}

K3bAudioTrack::~K3bAudioTrack()
{
	// delete the buffer file
	qDebug( "Deleting buffered file" + m_bufferFile );
	
	if( !m_bufferFile.isEmpty() )
		QFile::remove( m_bufferFile );
}

uint K3bAudioTrack::size() const{
	return m_file.size();
}

int K3bAudioTrack::index() const
{
	int i = m_parent->find( this );
	if( i < 0 )
		qDebug( "(K3bAudioTrack) I'm not part of my parent!");
	return i;
}
