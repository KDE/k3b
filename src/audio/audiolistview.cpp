/* 
 *
 * $Id$
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


#include "audiolistview.h"
#include "audiolistviewitem.h"
#include "k3baudiotrack.h"
#include "../k3b.h"
#include "../k3baudioplayer.h"
#include "k3baudiotrackdialog.h"
#include "k3baudiodoc.h"
#include "../k3bview.h"

#include <qheader.h>
#include <qtimer.h>
#include <qdragobject.h>
#include <qpoint.h>
#include <qptrlist.h>
#include <qstringlist.h>
#include <qevent.h>
#include <qpainter.h>
#include <qfontmetrics.h>

#include <kurl.h>
#include <kurldrag.h>
#include <klocale.h>
#include <kaction.h>
#include <kpopupmenu.h>
#include <kiconloader.h>
#include <kdialog.h>


K3bAudioListView::K3bAudioListView( K3bView* view, K3bAudioDoc* doc, QWidget *parent, const char *name )
  : K3bListView(parent,name), m_doc(doc), m_view(view)
{
  setAcceptDrops( true );
  setDropVisualizer( true );
  setAllColumnsShowFocus( true );
  setDragEnabled( true );
  //  setSelectionModeExt( KListView::Konqueror ); // FileManager in KDE3
  setSelectionModeExt( KListView::Extended );
  setItemsMovable( false );

  setNoItemText( i18n("Use drag'n'drop to add audio files to the project.") + "\n"
		 + i18n("After that press the burn button to write the CD." ) );

  setSorting( 0 );

  setupActions();
  setupPopupMenu();

  setupColumns();
  header()->setClickEnabled( false );

  m_animationTimer = new QTimer( this );
  connect( m_animationTimer, SIGNAL(timeout()), this, SLOT(slotAnimation()) );

  connect( this, SIGNAL(dropped(KListView*, QDropEvent*, QListViewItem*)),
	   this, SLOT(slotDropped(KListView*, QDropEvent*, QListViewItem*)) );
  connect( this, SIGNAL(contextMenu(KListView*, QListViewItem*, const QPoint&)),
	   this, SLOT(showPopupMenu(KListView*, QListViewItem*, const QPoint&)) );
  connect( this, SIGNAL(doubleClicked(QListViewItem*, const QPoint&, int)),
	   this, SLOT(showPropertiesDialog()) );

  connect( m_doc, SIGNAL(newTracks()), this, SLOT(slotUpdateItems()) );


  slotUpdateItems();
}

K3bAudioListView::~K3bAudioListView(){
}

void K3bAudioListView::setupColumns(){
  addColumn( i18n("No.") );
  addColumn( i18n("Artist (CD-Text)") );
  addColumn( i18n("Title (CD-Text)") );
  addColumn( i18n("Length") );
  addColumn( i18n("Pregap") );
  addColumn( i18n("Filename") );
}


void K3bAudioListView::setupActions()
{
  m_actionCollection = new KActionCollection( this );

  m_actionProperties = new KAction( i18n("Properties..."), "misc",
				  0, this, SLOT(showPropertiesDialog()), 
				    actionCollection(), "audio_properties" );
  m_actionRemove = new KAction( i18n( "Remove" ), "editdelete",
			      Key_Delete, this, SLOT(slotRemoveTracks()), 
				actionCollection(), "audio_remove" );
  m_actionPlay = new KAction( i18n( "Play" ), "1rightarrow",
			      0, this, SLOT(slotPlaySelected()), 
			      actionCollection(), "audio_play" );

  m_actionPlayAll = new KAction( i18n( "Play all" ), "1rightarrow",
				 0, this, SLOT(slotPlayAll()), 
				 actionCollection(), "audio_play_all" );

  // disabled by default
  m_actionRemove->setEnabled(false);
  m_actionPlay->setEnabled(false);
}


void K3bAudioListView::setupPopupMenu()
{
  m_popupMenu = new KPopupMenu( this, "AudioViewPopupMenu" );
  m_actionPlayAll->plug( m_popupMenu );
  m_popupMenu->insertSeparator();
  m_actionPlay->plug( m_popupMenu );
  m_actionRemove->plug( m_popupMenu );
  m_popupMenu->insertSeparator();
  m_actionProperties->plug( m_popupMenu );
  m_popupMenu->insertSeparator();
  k3bMain()->actionCollection()->action("file_burn")->plug( m_popupMenu );
}


bool K3bAudioListView::acceptDrag(QDropEvent* e) const
{
  // the first is for built-in item moving, the second for dropping urls
  return ( KListView::acceptDrag(e) || QUriDrag::canDecode(e) );
}


QDragObject* K3bAudioListView::dragObject()
{
  QPtrList<QListViewItem> list = selectedItems();

  if( list.isEmpty() )
    return 0;

  QPtrListIterator<QListViewItem> it(list);
  KURL::List urls;

  for( ; it.current(); ++it )
    urls.append( KURL( ((K3bAudioListViewItem*)it.current())->audioTrack()->absPath() ) );

  return KURLDrag::newDrag( urls, viewport() );
}


void K3bAudioListView::slotDropped( KListView*, QDropEvent* e, QListViewItem* after )
{
  if( !e->isAccepted() )
    return;

  int pos;
  if( after == 0L )
    pos = 0;
  else
    pos = ((K3bAudioListViewItem*)after)->audioTrack()->index() + 1;

  if( e->source() == viewport() ) {
    QPtrList<QListViewItem> sel = selectedItems();
    QPtrListIterator<QListViewItem> it(sel);
    K3bAudioTrack* trackAfter = ( after ? ((K3bAudioListViewItem*)after)->audioTrack() : 0 );
    while( it.current() ) {
      K3bAudioTrack* track = ((K3bAudioListViewItem*)it.current())->audioTrack();
      m_doc->moveTrack( track, trackAfter );
      trackAfter = track;
      ++it;
    }

    sort();  // This is so lame!
  }
  else {
    KURL::List urls;
    KURLDrag::decode( e, urls );

    m_doc->addTracks( urls, pos );
  }
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
	if( item->audioTrack()->length() > 0
	    || item->audioTrack()->status() == K3bAudioTrack::CORRUPT ) {
	  // set status icon
	  switch( item->audioTrack()->status() ) {
	  case K3bAudioTrack::OK:
	    item->setPixmap( 3, SmallIcon( "greenled" ) );
	    break;
	  case K3bAudioTrack::RECOVERABLE:
	    item->setPixmap( 3, SmallIcon( "yellowled" ) );
	    break;
	  case K3bAudioTrack::CORRUPT:
	    item->setPixmap( 3, SmallIcon( "redled" ) );
	    break;
	  }

	  item->animationIconNumber = 0;
	}
	else {
	  int& iconNumber = item->animationIconNumber;
	  QString icon = QString( "kde%1" ).arg( iconNumber );
	  item->setPixmap( 3, SmallIcon( icon ) );
	  iconNumber++;
	  if ( iconNumber > 6 )
	    iconNumber = 1;

	  animate = true;
	}
      }
    }

  if( !animate ) {
    m_animationTimer->stop();
    emit lengthReady();
  }
}


void K3bAudioListView::showPopupMenu( KListView*, QListViewItem* _item, const QPoint& _point )
{
  if( _item ) {
     m_actionRemove->setEnabled(true);
     m_actionPlay->setEnabled(true);
   }
   else {
     m_actionRemove->setEnabled(false);
     m_actionPlay->setEnabled(false);
   }

  m_popupMenu->popup( _point );
}


void K3bAudioListView::showPropertiesDialog()
{
  QPtrList<K3bAudioTrack> selected = selectedTracks();
  if( !selected.isEmpty() ) {
    K3bAudioTrackDialog d( selected, this );
    if( d.exec() ) {
      repaint();
    }
  }
  else {
    m_view->burnDialog( false );
  }
}


QPtrList<K3bAudioTrack> K3bAudioListView::selectedTracks()
{
  QPtrList<K3bAudioTrack> selectedTracks;
  QPtrList<QListViewItem> selectedVI( selectedItems() );
  for( QListViewItem* item = selectedVI.first(); item != 0; item = selectedVI.next() ) {
    K3bAudioListViewItem* audioItem = dynamic_cast<K3bAudioListViewItem*>(item);
    if( audioItem ) {
      selectedTracks.append( audioItem->audioTrack() );
    }
  }

  return selectedTracks;
}


void K3bAudioListView::slotRemoveTracks()
{
  QPtrList<K3bAudioTrack> selected = selectedTracks();
  if( !selected.isEmpty() ) {

    for( K3bAudioTrack* track = selected.first(); track != 0; track = selected.next() ) {
      m_doc->removeTrack( track );

      // not best, I think we should connect to doc.removedTrack (but since there is only one view this is not important!)
      QListViewItem* viewItem = m_itemMap[track];
      m_itemMap.remove( track );
      delete viewItem;
    }
  }

  if( m_doc->numOfTracks() == 0 ) {
    m_actionRemove->setEnabled(false);
    m_actionPlay->setEnabled(false);
  }
}


void K3bAudioListView::slotUpdateItems()
{
  // iterate through all viewItems and check if the track is still there
//   QListViewItemIterator it( m_songlist );
//   for( ; it.current(); ++it ) {
//     K3bAudioListViewItem* item = (K3bAudioListViewItem*)it.current();
//     bool stillThere = false;

//     for( K3bAudioTrack* track = m_doc->first(); track != 0; track = m_doc->next() ) {
//       if( track == item->audioTrack() ) {
// 	stillThere = true;
// 	break;
//       }
//     }

//     if( !stillThere ) {
//       m_itemMap.remove( item->audioTrack() );
//       delete item;
//     }
//   }


  // iterate through all doc-tracks and test if we have a listItem, if not, create one
  K3bAudioTrack* track = m_doc->first();
  K3bAudioTrack* lastTrack = 0;
  while( track != 0 ) {
    if( !m_itemMap.contains( track ) )
      m_itemMap.insert( track, new K3bAudioListViewItem( track, this, m_itemMap[lastTrack] ) );

    lastTrack = track;
    track = m_doc->next();
  }

  if( m_doc->numOfTracks() > 0 ) {
    m_actionRemove->setEnabled(true);
    m_actionPlay->setEnabled(true);
  }
  else {
    m_actionRemove->setEnabled(false);
    m_actionPlay->setEnabled(false);
  }
}


void K3bAudioListView::slotPlaySelected()
{
  QPtrList<K3bAudioTrack> selected = selectedTracks();

  if( !selected.isEmpty() ) {
    QListIterator<K3bAudioTrack> it( selected );
    k3bMain()->audioPlayer()->playFile( it.current()->absPath() );

    for( ++it; it.current(); ++it ) {
      k3bMain()->audioPlayer()->enqueueFile( it.current()->absPath() );
    }
  }
}


void K3bAudioListView::slotPlayAll()
{
  if( !m_doc->tracks()->isEmpty() ) {
    QListIterator<K3bAudioTrack> it( *m_doc->tracks() );
    k3bMain()->audioPlayer()->playFile( it.current()->absPath() );

    for( ++it; it.current(); ++it ) {
      k3bMain()->audioPlayer()->enqueueFile( it.current()->absPath() );
    }
  }
}


#include "audiolistview.moc"
