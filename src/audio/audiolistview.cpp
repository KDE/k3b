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
#include "audiolistviewitem.h"
#include "k3baudiotrack.h"

#include <qevent.h>
#include <qdragobject.h>
#include <qheader.h>
#include <qtimer.h>

#include <kiconloader.h>


K3bAudioListView::K3bAudioListView(QWidget *parent, const char *name )
  : KListView(parent,name)
{
  setAcceptDrops( true );
  setDropVisualizer( true );
  setAllColumnsShowFocus( true );
  setDragEnabled( true );
  setSelectionModeExt( KListView::Konqueror );
		
  setupColumns();
  header()->setClickEnabled( false );

  m_animationTimer = new QTimer( this );
  connect( m_animationTimer, SIGNAL(timeout()), this, SLOT(slotAnimation()) );
}

K3bAudioListView::~K3bAudioListView(){
}

void K3bAudioListView::setupColumns(){
  addColumn( "No" );
  addColumn( "Artist (CD-Text)" );
  addColumn( "Title (CD-Text)" );
  addColumn( "Length" );
  addColumn( "Pregap" );
  addColumn( "Filename" );
	
  setItemsRenameable( true );
  setRenameable( 0, false );
  setRenameable( 1 );
  setRenameable( 2 );
}

bool K3bAudioListView::acceptDrag(QDropEvent* e) const{
  return ( e->source() == viewport() || QTextDrag::canDecode(e) );
}


void K3bAudioListView::insertItem( QListViewItem* item )
{
  KListView::insertItem( item );

  // make sure at least one item is selected
  if( selectedItems().isEmpty() ) {
    setSelected( firstChild(), true );
  }

  if( !m_animationTimer->isActive() )
    m_animationTimer->start( 50 );
}



void K3bAudioListView::slotAnimation()
{
  QListViewItemIterator it( this );

  bool animate = false;

  for (; it.current(); ++it )
    {
      K3bAudioListViewItem* item = (K3bAudioListViewItem*)it.current();

      if( item->animationIconNumber > 0 ) {
	if( item->audioTrack()->length() > 0 ) {
	  // set status icon
	  switch( item->audioTrack()->status() ) {
	  case K3bAudioTrack::OK:
	    item->setPixmap( 3, SmallIcon( "ok" ) );
	    break;
	  case K3bAudioTrack::RECOVERABLE:
	    item->setPixmap( 3, SmallIcon( "undo" ) );
	    break;
	  case K3bAudioTrack::CORRUPT:
	    item->setPixmap( 3, SmallIcon( "stop" ) );
	    break;
	  }
	  
	  item->animationIconNumber = 0;
	}
	else {
	  int& iconNumber = item->animationIconNumber;
	  QString icon = QString( "kiotreework%1" ).arg( iconNumber );
	  item->setPixmap( 3, SmallIcon( icon ) );
	  iconNumber++;
	  if ( iconNumber > 6 )
	    iconNumber = 1;

	  animate = true;
	}
      }
    }

  if( !animate )
    m_animationTimer->stop();
}


#include "audiolistview.moc"
