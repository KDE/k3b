/***************************************************************************
                          k3baudioview.cpp  -  description
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

#include "k3baudioview.h"
#include "k3baudiodoc.h"
#include "audiolistview.h"
#include "audiolistviewitem.h"
#include "k3baudiotrack.h"

// QT-includes
#include <qlayout.h>

// KDE-includes


K3bAudioView::K3bAudioView( K3bAudioDoc* pDoc, QWidget* parent, const char *name, int wflags )
 : K3bView( pDoc, parent, name, wflags )
{
	QGridLayout* grid = new QGridLayout( this );
	m_songlist = new AudioListView( this );
	
	grid->addWidget( m_songlist, 0, 0 );
	
	// TODO: create slot dropped that calculates the position where was dropped and passes it to the signal dropped( KURL&, int)
	connect( m_songlist, SIGNAL(dropped(QDropEvent*, QListViewItem*)), this, SIGNAL(dropped(QDropEvent*)) );
}

K3bAudioView::~K3bAudioView(){
}

void K3bAudioView::addItem( K3bAudioTrack* _track )
{
	qDebug( "(K3bAudioView) adding new item to list: " + _track->fileName() );
	(void)new AudioListViewItem( _track, m_songlist );
}
