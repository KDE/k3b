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
#include <qstring.h>
#include <qevent.h>
#include <qdragobject.h>

// KDE-includes


K3bAudioView::K3bAudioView( K3bAudioDoc* pDoc, QWidget* parent, const char *name, int wflags )
 : K3bView( pDoc, parent, name, wflags )
{
	QGridLayout* grid = new QGridLayout( this );
	m_songlist = new AudioListView( this );
	
	grid->addWidget( m_songlist, 0, 0 );
	
	// TODO: create slot dropped that calculates the position where was dropped and passes it to the signal dropped( KURL&, int)
	connect( m_songlist, SIGNAL(dropped(KListView*, QDropEvent*, QListViewItem*)), this, SLOT(slotDropped(KListView*, QDropEvent*, QListViewItem*)) );
	connect( m_songlist, SIGNAL(moved(QListViewItem*,QListViewItem*,QListViewItem*)), this, SLOT(slotItemMoved( QListViewItem*, QListViewItem*, QListViewItem* )) );
}

K3bAudioView::~K3bAudioView(){
}

void K3bAudioView::addItem( K3bAudioTrack* _track )
{
	qDebug( "(K3bAudioView) adding new item to list: " + _track->fileName() );
	(void)new AudioListViewItem( _track, m_songlist );
}

void K3bAudioView::slotDropped( KListView* listView, QDropEvent* e, QListViewItem* after )
{
	if( !e->isAccepted() )
		return;

	QString url;
	QTextDrag::decode( e, url );
	// TODO: parse multible urls
	url.truncate( url.find( '\r') );
	AudioListViewItem* _item = (AudioListViewItem*)after;
	uint _pos;
	if( _item == 0L )
		_pos = 0;
	else
		_pos = _item->text(0).toInt();
		
	emit dropped( url, _pos );
}

void K3bAudioView::slotItemMoved( QListViewItem* item, QListViewItem* afterFirst, QListViewItem* afterNow )
{
	if( !item)
		return;
		
	AudioListViewItem *_item, *_first, *_now;
	_item = (AudioListViewItem*)item;
	//_first = (AudioListViewItem*)afterFirst;
	_now = (AudioListViewItem*)afterNow;
	
	uint before, after;
	// text starts at 1 but QList starts at 0
	before = _item->text(0).toInt()-1;
	if( _now ) {
		after = _now->text(0).toInt()-1;
		if( before > after )
			after++;
	}
	else
		after = 0;
	
	emit itemMoved( before, after );
}
