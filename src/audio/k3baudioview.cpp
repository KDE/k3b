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

#include "../k3b.h"
#include "k3baudioview.h"
#include "k3baudiodoc.h"
#include "audiolistview.h"
#include "audiolistviewitem.h"
#include "k3baudiotrack.h"
#include "k3baudiotrackdialog.h"
#include "k3baudioburndialog.h"
#include "../k3bfillstatusdisplay.h"

// QT-includes
#include <qlayout.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qevent.h>
#include <qdragobject.h>
#include <qpoint.h>
#include <qtimer.h>
#include <qlist.h>

// KDE-includes
#include <kpopupmenu.h>
#include <kaction.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kapp.h>


K3bAudioView::K3bAudioView( K3bAudioDoc* pDoc, QWidget* parent, const char *name )
  : K3bView( pDoc, parent, name )
{
  m_doc = pDoc;

  QGridLayout* grid = new QGridLayout( this );
  grid->setSpacing( 5 );
  grid->setMargin( 2 );
	
  m_songlist = new K3bAudioListView( this );
  m_fillStatusDisplay = new K3bFillStatusDisplay( doc, this );
  m_fillStatusDisplay->showTime();
  m_burnDialog = 0;
	
  grid->addWidget( m_songlist, 0, 0 );
  grid->addWidget( m_fillStatusDisplay, 1, 0 );

  setupActions();
  setupPopupMenu();

  // TODO: create slot dropped that calculates the position where was dropped and passes it to the signal dropped( KURL&, int)
  connect( m_songlist, SIGNAL(dropped(KListView*, QDropEvent*, QListViewItem*)),
	   this, SLOT(slotDropped(KListView*, QDropEvent*, QListViewItem*)) );
  connect( m_songlist, SIGNAL(moved(QListViewItem*,QListViewItem*,QListViewItem*)),
	   this, SLOT(slotItemMoved( QListViewItem*, QListViewItem*, QListViewItem* )) );
	
  connect( m_songlist, SIGNAL(rightButtonClicked(QListViewItem*, const QPoint&, int)),
	   this, SLOT(showPopupMenu(QListViewItem*, const QPoint&)) );
  connect( m_songlist, SIGNAL(doubleClicked(QListViewItem*, const QPoint&, int)),
	   this, SLOT(showPropertiesDialog()) );


  connect( pDoc, SIGNAL(newTracks()), this, SLOT(slotUpdateItems()) );


  slotUpdateItems();
}

K3bAudioView::~K3bAudioView(){
}


K3bProjectBurnDialog* K3bAudioView::burnDialog()
{
  if( !m_burnDialog )
    m_burnDialog = new K3bAudioBurnDialog( (K3bAudioDoc*)getDocument(), k3bMain(), "audioburndialog", true );
		
  return m_burnDialog;
}


void K3bAudioView::setupActions()
{
  m_actionProperties = new KAction( i18n("Properties"), "misc", 
				  0, this, SLOT(showPropertiesDialog()), actionCollection() );
  m_actionRemove = new KAction( i18n( "Remove" ), "editdelete", 
			      Key_Delete, this, SLOT(slotRemoveTracks()), actionCollection() );

  // disabled by default
  m_actionRemove->setEnabled(false);
  m_actionProperties->setEnabled(false);
}


void K3bAudioView::setupPopupMenu()
{
  m_popupMenu = new KPopupMenu( m_songlist, "AudioViewPopupMenu" );
  //  m_popupMenu->insertTitle( i18n( "Track Options" ) );
  m_actionRemove->plug( m_popupMenu );
  m_actionProperties->plug( m_popupMenu);
}



void K3bAudioView::slotDropped( KListView*, QDropEvent* e, QListViewItem* after )
{
  if( !e->isAccepted() )
    return;

  QString droppedText;
  QTextDrag::decode( e, droppedText );
  QStringList _urls = QStringList::split("\r\n", droppedText );
  uint _pos;
  if( after == 0L )
    _pos = 0;
  else
    _pos = after->text(0).toInt();
		
  emit dropped( _urls, _pos );
}

void K3bAudioView::slotItemMoved( QListViewItem* item, QListViewItem*, QListViewItem* afterNow )
{
  if( !item)
    return;
		
  uint before, after;
  // text starts at 1 but QList starts at 0
  before = item->text(0).toInt()-1;
  if( afterNow ) {
    after = afterNow->text(0).toInt()-1;
    if( before > after )
      after++;
  }
  else
    after = 0;

  m_doc->moveTrack( before, after );
}

void K3bAudioView::showPopupMenu( QListViewItem* _item, const QPoint& _point )
{
  if( _item )
    m_popupMenu->popup( _point );
}


void K3bAudioView::showPropertiesDialog()
{
  QList<K3bAudioTrack> selected = selectedTracks();
  if( !selected.isEmpty() ) {
    K3bAudioTrackDialog* d = new K3bAudioTrackDialog( selected, this );
    d->exec();
    delete d;
  }
}


QList<K3bAudioTrack> K3bAudioView::selectedTracks()
{
  QList<K3bAudioTrack> _selectedTracks;
  QList<QListViewItem> selectedItems = m_songlist->selectedItems();
  for( QListViewItem* item = selectedItems.first(); item != 0; item = selectedItems.next() ) {
    K3bAudioListViewItem* audioItem = dynamic_cast<K3bAudioListViewItem*>(item);
    if( audioItem ) {
      _selectedTracks.append( audioItem->audioTrack() );
    }
  }

  return _selectedTracks;
}


void K3bAudioView::slotRemoveTracks()
{
  QList<K3bAudioTrack> selected = selectedTracks();
  if( !selected.isEmpty() ) {

    for( K3bAudioTrack* track = selected.first(); track != 0; track = selected.next() ) {
      ((K3bAudioDoc*)doc)->removeTrack( track->index() );
		
      // not best, I think we should connect to doc.removedTrack (but while there is only one view this is not important!)
      QListViewItem* viewItem = m_itemMap[track];
      m_itemMap.remove( track );
      delete viewItem;
    }
  }

  if( m_doc->numOfTracks() == 0 ) {
    m_actionRemove->setEnabled(false);
    m_actionProperties->setEnabled(false);
  }
}


void K3bAudioView::slotUpdateItems()
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
      m_itemMap.insert( track, new K3bAudioListViewItem( track, m_songlist, m_itemMap[lastTrack] ) );

    lastTrack = track;
    track = m_doc->next();
  }

   m_fillStatusDisplay->update();

   if( m_doc->numOfTracks() > 0 ) {
     m_actionRemove->setEnabled(true);
     m_actionProperties->setEnabled(true);
   }
   else {
     m_actionRemove->setEnabled(false);
     m_actionProperties->setEnabled(false);
   }
}


#include "k3baudioview.moc"
