/***************************************************************************
                          audiolistview.cpp  -  description
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

#include "audiolistview.h"

#include <qevent.h>
#include <qdragobject.h>


AudioListView::AudioListView(QWidget *parent, const char *name )
 : KListView(parent,name)
{
 	setAcceptDrops( true );
	setDropVisualizer( true );
	setAllColumnsShowFocus( true );
	
	setupColumns();
}

AudioListView::~AudioListView(){
}

void AudioListView::setupColumns(){
	addColumn( "No" );
	addColumn( "Trackname" );
	addColumn( "Length" );
	addColumn( "Pregap" );
	addColumn( "Start" );
	addColumn( "End" );
}

bool AudioListView::acceptDrag(QDropEvent* e) const{
	return QTextDrag::canDecode(e);
}
