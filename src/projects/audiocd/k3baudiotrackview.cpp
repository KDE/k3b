/* 
 *
 * $Id$
 * Copyright (C) 2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3baudiotrackview.h"
#include "k3baudiotrackviewitem.h"
#include "k3baudiodatasourceviewitem.h"
#include "k3baudiotrack.h"
#include "k3baudiodatasource.h"
#include "k3baudiotrackdialog.h"
#include "k3baudiodoc.h"
#include "k3baudiozerodata.h"

#include <k3bview.h>
#include <k3bcdtextvalidator.h>

#include <qheader.h>
#include <qtimer.h>
#include <qdragobject.h>
#include <qpoint.h>
#include <qptrlist.h>
#include <qstringlist.h>
#include <qevent.h>

#include <kurl.h>
#include <kurldrag.h>
#include <klocale.h>
#include <kaction.h>
#include <kpopupmenu.h>
#include <kiconloader.h>
#include <kapplication.h>

#define XK_MISCELLANY
#include <X11/keysymdef.h>
#undef XK_MISCELLANY



K3bAudioTrackView::K3bAudioTrackView( K3bAudioDoc* doc, QWidget* parent, const char* name )
  : K3bListView( parent, name ),
    m_doc(doc),
    m_updatingColumnWidths(false)
{
  setAcceptDrops( true );
  setDropVisualizer( true );
  setAllColumnsShowFocus( true );
  setDragEnabled( true );
  //  setSelectionModeExt( KListView::Konqueror ); // FileManager in KDE3
  setSelectionModeExt( KListView::Extended );
  setItemsMovable( false );
  setAlternateBackground( QColor() ); // disable alternate colors

  setNoItemText( i18n("Use drag'n'drop to add audio files to the project.") + "\n"
		 + i18n("After that press the burn button to write the CD." ) );

  setValidator( new K3bCdTextValidator( this ) );

  setupColumns();
  setupActions();

  m_animationTimer = new QTimer( this );
  connect( m_animationTimer, SIGNAL(timeout()), this, SLOT(slotAnimation()) );

  m_autoOpenTrackTimer = new QTimer( this );
  connect( m_autoOpenTrackTimer, SIGNAL(timeout()), this, SLOT(slotDragTimeout()) );

  connect( this, SIGNAL(dropped(QDropEvent*, QListViewItem*, QListViewItem*)),
	   this, SLOT(slotDropped(QDropEvent*, QListViewItem*, QListViewItem*)) );
  connect( this, SIGNAL(contextMenu(KListView*, QListViewItem*, const QPoint&)),
	   this, SLOT(showPopupMenu(KListView*, QListViewItem*, const QPoint&)) );
  connect( this, SIGNAL(doubleClicked(QListViewItem*, const QPoint&, int)),
	   this, SLOT(showPropertiesDialog()) );

  connect( doc, SIGNAL(changed()),
	   this, SLOT(slotChanged()) );
  connect( doc, SIGNAL(trackChanged(K3bAudioTrack*)),
	   this, SLOT(slotTrackChanged(K3bAudioTrack*)) );
  connect( doc, SIGNAL(trackRemoved(K3bAudioTrack*)),
	   this, SLOT(slotTrackRemoved(K3bAudioTrack*)) );

  slotChanged();
}


K3bAudioTrackView::~K3bAudioTrackView()
{
}


void K3bAudioTrackView::setupColumns()
{
  addColumn( i18n("No.") );
  addColumn( i18n("Artist (CD-Text)") );
  addColumn( i18n("Title (CD-Text)") );
  addColumn( i18n("Type") );
  addColumn( i18n("Length") );
  addColumn( i18n("Filename") );

  setColumnAlignment( 3, Qt::AlignHCenter );
  setColumnAlignment( 4, Qt::AlignHCenter );

  setColumnWidthMode( 1, Manual );
  setColumnWidthMode( 2, Manual );
  setColumnWidthMode( 3, Manual );
  setColumnWidthMode( 4, Manual );
  setColumnWidthMode( 5, Manual );

  header()->setResizeEnabled( false );
  header()->setClickEnabled( false );
  setSorting( -1 );
}


void K3bAudioTrackView::setupActions()
{
  m_actionCollection = new KActionCollection( this );
  m_popupMenu = new KPopupMenu( this );

  m_actionProperties = new KAction( i18n("Properties"), "misc",
				    0, this, SLOT(showPropertiesDialog()), 
				    actionCollection(), "track_properties" );
  m_actionRemove = new KAction( i18n( "Remove" ), "editdelete",
				Key_Delete, this, SLOT(slotRemove()), 
				actionCollection(), "track_remove" );

  m_actionAddSilence = new KAction( i18n("Add Silence"), "misc",
				    0, this, SLOT(slotAddSilence()),
				    actionCollection(), "track_add_silence" );
  m_actionMergeTracks = new KAction( i18n("Merge Tracks"), "misc",
				     0, this, SLOT(slotMergeTracks()),
				     actionCollection(), "track_merge" );
  m_actionSplitSource = new KAction( i18n("Source to Track"), "misc",
				     0, this, SLOT(slotSplitSource()),
				     actionCollection(), "source_split" );

  // TODO: add more actions (add source, merge tracks, split source,...)
}


bool K3bAudioTrackView::acceptDrag(QDropEvent* e) const
{
  // the first is for built-in item moving, the second for dropping urls
  return ( KListView::acceptDrag(e) || KURLDrag::canDecode(e) );
}


QDragObject* K3bAudioTrackView::dragObject()
{
  QPtrList<QListViewItem> list = selectedItems();

  if( list.isEmpty() )
    return 0;

  QPtrListIterator<QListViewItem> it(list);
  KURL::List urls;

  for( ; it.current(); ++it ) {
    // FIXME
  }

  return new KURLDrag( urls, viewport() );
}


void K3bAudioTrackView::slotDropped( QDropEvent* e, QListViewItem* parent, QListViewItem* after )
{
  m_autoOpenTrackTimer->stop();

  if( !e->isAccepted() )
    return;

  K3bAudioTrack* trackAfter = 0;
  K3bAudioTrack* trackParent = 0;
  K3bAudioDataSource* sourceAfter = 0;
  if( after ) {
    if( K3bAudioTrackViewItem* tv = dynamic_cast<K3bAudioTrackViewItem*>( after ) ) {
      trackAfter = tv->track();
    }
    else if( K3bAudioDataSourceViewItem* sv = dynamic_cast<K3bAudioDataSourceViewItem*>( after ) ) {
      sourceAfter = sv->source();
    }
  }

  if( K3bAudioTrackViewItem* tv = dynamic_cast<K3bAudioTrackViewItem*>( parent ) ) {
    trackParent = tv->track();
  }

  //
  // In case the sources are not shown we do not want to handle them because the average
  // user would be confused otherwise
  //
  if( trackParent && !m_trackItemMap[trackParent]->showingSources() ) {
    kdDebug() << "(K3bAudioTrackView) dropped after track which does not show it's sources." << endl;
    trackAfter = trackParent;
    trackParent = 0;
  }

  if( e->source() == viewport() ) {

    bool copyItems = (e->action() == QDropEvent::Copy);

    // 1. tracks (with some of their sources) -> move complete tracks around
    // 2. sources (multiple sources) -> move the sources to the destination track
    // 3. tracks and sources (the latter without their track) -> ignore the latter sources
    QPtrList<K3bAudioTrack> tracks;
    QPtrList<K3bAudioDataSource> sources;
    getSelectedItems( tracks, sources );

    //
    // remove all sources which belong to one of the selected tracks since they will be 
    // moved along with their tracks
    //
    QPtrListIterator<K3bAudioDataSource> srcIt( sources );
    while( srcIt.current() ) {
      if( tracks.containsRef( srcIt.current()->track() ) )
	sources.removeRef( *srcIt );
      else
	++srcIt;
    }

    //
    // Now move (or copy) all the tracks
    //
    for( QPtrListIterator<K3bAudioTrack> it( tracks ); it.current(); ++it ) {
      K3bAudioTrack* track = *it;
      if( trackParent ) {
	trackParent->merge( copyItems ? track->copy() : track, sourceAfter );
      }
      else if( trackAfter ) {
	if( copyItems )
	  track->copy()->moveAfter( trackAfter );
	else
	  track->moveAfter( trackAfter );
      }
    }

    //
    // now move (or copy) the sources
    //
    for( QPtrListIterator<K3bAudioDataSource> it( sources ); it.current(); ++it ) {
      K3bAudioDataSource* source = *it;
      if( trackParent ) {
	if( sourceAfter ) {
	  if( copyItems )
	    source->copy()->moveAfter( sourceAfter );
	  else
	    source->moveAfter( sourceAfter );
	}
	else {
	  if( copyItems )
	    source->copy()->moveAhead( trackParent->firstSource() );
	  else
	    source->moveAhead( trackParent->firstSource() );
	}
      }
      else {
	// create a new track
	K3bAudioTrack* track = new K3bAudioTrack( m_doc );

	// special case: the source we remove from the track is the last and the track
	// will be deleted.
	if( !copyItems && trackAfter == source->track() && trackAfter->numberSources() == 1 )
	  trackAfter = trackAfter->prev();

	if( copyItems )
	  track->addSource( source->copy() );
	else
	  track->addSource( source );

	if( trackAfter ) {
	  track->moveAfter( trackAfter );
	  trackAfter = track;
	}
	else {
	  track->moveAhead( m_doc->firstTrack() );
	  trackAfter = track;
	}
      }
    }
  }
  else {
    KURL::List urls;
    KURLDrag::decode( e, urls );

    if( trackParent ) {
      m_doc->addSources( trackParent, urls, sourceAfter );
    }
    else {
      // add as new tracks
      m_doc->addTracks( urls, trackAfter ? trackAfter->index()+1 : 0 );
    }
  }

  showAllSources();
}


void K3bAudioTrackView::slotChanged()
{
  kdDebug() << "(K3bAudioTrackView::slotChanged)" << endl;
  // we only need to add new items here. Everything else is done in the
  // specific slots below
  K3bAudioTrack* track = m_doc->firstTrack();
  bool newTracks = false;
  while( track ) {
    bool newTrack;
    getTrackViewItem( track, &newTrack );
    if( newTrack )
      newTracks = true;
    track = track->next();
  }

  if( newTracks ) {
    m_animationTimer->start(200);
    showAllSources();
  }

  kdDebug() << "(K3bAudioTrackView::slotChanged) finished" << endl;
}


K3bAudioTrackViewItem* K3bAudioTrackView::getTrackViewItem( K3bAudioTrack* track, bool* isNewItem )
{
  QMap<K3bAudioTrack*, K3bAudioTrackViewItem*>::iterator itemIt = m_trackItemMap.find(track);
  if( itemIt == m_trackItemMap.end() ) {
    kdDebug() << "(K3bAudioTrackView) new track " << track << endl;
    K3bAudioTrackViewItem* prevItem = 0;
    if( track->prev() && m_trackItemMap.contains( track->prev() ) )
      prevItem = m_trackItemMap[track->prev()];
    K3bAudioTrackViewItem* newItem = new K3bAudioTrackViewItem( this, prevItem, track );
    // 
    // disable the item until the files have been analysed
    // so the user may not change the cd-text until the one from the
    // file is loaded.
    //
    // Since for some reason QT thinks it's bad to open disabled items
    // we need to open it before disabling it
    //
    newItem->showSources( track->numberSources() != 1 );
    newItem->setEnabled( false );
    m_trackItemMap[track] = newItem;

    if( isNewItem )
      *isNewItem = true;
    return newItem;
  }
  else {
    if( isNewItem )
      *isNewItem = false;
    return *itemIt;
  }
}


void K3bAudioTrackView::slotTrackChanged( K3bAudioTrack* track )
{
  kdDebug() << "(K3bAudioTrackView::slotTrackChanged( " << track << " )" << endl;

  //
  // There may be some tracks around that have not been added to the list yet
  // (and might never). We ignore them until they are in the list and then
  // we create the item in slotChanged
  //
  if( track->inList() ) {
    K3bAudioTrackViewItem* item = getTrackViewItem(track);
    item->updateSourceItems();

    if( track->numberSources() > 1 )
      item->showSources(true);
   
    // the length might have changed
    item->repaint();

    // FIXME: only do this if the position really changed
    // move the item if the position has changed
    if( track->prev() && m_trackItemMap.contains(track->prev()) )
      item->moveItem( m_trackItemMap[track->prev()] );
    else if( !track->prev() ) {
      takeItem( item );
      insertItem( item );
    }

    // start the animation in case new sources have been added
    m_animationTimer->start( 200 );

    showAllSources();
  }
  kdDebug() << "(K3bAudioTrackView::slotTrackChanged( " << track << " ) finished" << endl;
}


void K3bAudioTrackView::slotTrackRemoved( K3bAudioTrack* track )
{
  kdDebug() << "(K3bAudioTrackView::slotTrackRemoved( " << track << " )" << endl;
  delete m_trackItemMap[track];
  m_trackItemMap.erase(track);
}


void K3bAudioTrackView::showAllSources()
{
  // TODO: add an action to show all sources

  QListViewItem* item = firstChild();
  while( item ) {
    if( K3bAudioTrackViewItem* tv = dynamic_cast<K3bAudioTrackViewItem*>( item ) )
      tv->showSources( tv->track()->numberSources() != 1 );
    item = item->nextSibling();
  }
}


void K3bAudioTrackView::keyPressEvent( QKeyEvent* e )
{
  //  showAllSources();

  K3bListView::keyPressEvent(e);
}


void K3bAudioTrackView::keyReleaseEvent( QKeyEvent* e )
{
  //  showAllSources();

  K3bListView::keyReleaseEvent(e);
}


void K3bAudioTrackView::contentsMouseMoveEvent( QMouseEvent* e )
{
  //  showAllSources();

  K3bListView::contentsMouseMoveEvent( e );
}


void K3bAudioTrackView::focusOutEvent( QFocusEvent* e )
{
  //  showAllSources();

  K3bListView::focusOutEvent( e );
}


void K3bAudioTrackView::resizeEvent( QResizeEvent* e )
{
  K3bListView::resizeEvent(e);

  resizeColumns();
}


void K3bAudioTrackView::contentsDragMoveEvent( QDragMoveEvent* event )
{
  K3bAudioTrackViewItem* item = findTrackItem( event->pos() );
  if( m_currentMouseOverItem != item ) {
    showAllSources(); // hide previous sources
    m_currentMouseOverItem = item;
  }
  if( m_currentMouseOverItem )
    m_autoOpenTrackTimer->start( 1000 ); // 1 sec

  K3bListView::contentsDragMoveEvent( event );
}


void K3bAudioTrackView::contentsDragLeaveEvent( QDragLeaveEvent* e )
{
  m_autoOpenTrackTimer->stop();
  K3bListView::contentsDragLeaveEvent( e );
}


K3bAudioTrackViewItem* K3bAudioTrackView::findTrackItem( const QPoint& pos ) const
{
  QListViewItem* parent = 0;
  QListViewItem* after = 0;
  K3bAudioTrackView* that = const_cast<K3bAudioTrackView*>(this);
  that->findDrop( pos, parent, after );
  if( parent )
    return static_cast<K3bAudioTrackViewItem*>( parent );
  else if( K3bAudioTrackViewItem* tv = dynamic_cast<K3bAudioTrackViewItem*>(after) )
    return tv;
  else if( K3bAudioDataSourceViewItem* sv = dynamic_cast<K3bAudioDataSourceViewItem*>(after) )
    return sv->trackViewItem();
  else
    return 0;
}


void K3bAudioTrackView::resizeColumns()
{
  if( m_updatingColumnWidths ) {
    kdDebug() << "(K3bAudioTrackView) already updating column widths." << endl;
    return;
  }

  m_updatingColumnWidths = true;

  // now properly resize the columns
  // minimal width for type, length, pregap
  // fixed for filename
  // expand for cd-text
  int titleWidth = header()->fontMetrics().width( header()->label(1) );
  int artistWidth = header()->fontMetrics().width( header()->label(2) );
  int typeWidth = header()->fontMetrics().width( header()->label(3) );
  int lengthWidth = header()->fontMetrics().width( header()->label(4) );
  int filenameWidth = header()->fontMetrics().width( header()->label(5) );

  for( QListViewItemIterator it( this ); it.current(); ++it ) {
    artistWidth = QMAX( artistWidth, it.current()->width( fontMetrics(), this, 1 ) );
    titleWidth = QMAX( titleWidth, it.current()->width( fontMetrics(), this, 2 ) );
    typeWidth = QMAX( typeWidth, it.current()->width( fontMetrics(), this, 3 ) );
    lengthWidth = QMAX( lengthWidth, it.current()->width( fontMetrics(), this, 4 ) );
    filenameWidth = QMAX( filenameWidth, it.current()->width( fontMetrics(), this, 5 ) );
  }

  // add a margin
  typeWidth += 10;
  lengthWidth += 10;

  // these always need to be completely visible
  setColumnWidth( 3, typeWidth );
  setColumnWidth( 4, lengthWidth );

  int remaining = visibleWidth() - typeWidth - lengthWidth - columnWidth(0);

  // now let's see if there is enough space for all
  if( remaining >= artistWidth + titleWidth + filenameWidth ) {
    remaining -= filenameWidth;
    remaining -= (titleWidth + artistWidth);
    setColumnWidth( 1, artistWidth + remaining/2 );
    setColumnWidth( 2, titleWidth + remaining/2 );
    setColumnWidth( 5, filenameWidth );
  }
  else if( remaining >= artistWidth + titleWidth + 20 ) {
    setColumnWidth( 1, artistWidth );
    setColumnWidth( 2, titleWidth );
    setColumnWidth( 5, remaining - artistWidth - titleWidth );
  }
  else {
    setColumnWidth( 1, remaining/3 );
    setColumnWidth( 2, remaining/3 );
    setColumnWidth( 5, remaining/3 );
  }

  triggerUpdate();
  m_updatingColumnWidths = false;
}


void K3bAudioTrackView::slotAnimation()
{
  kdDebug() << "(K3bAudioTrackView::slotAnimation)" << endl;
  resizeColumns();
  QListViewItem* item = firstChild();

  bool animate = false;

  while( item ) {
    K3bAudioTrackViewItem* trackItem = dynamic_cast<K3bAudioTrackViewItem*>(item);
    kdDebug() << "(K3bAudioTrackView::slotAnimation) for: " << trackItem << endl;
    if( trackItem->animate() )
      animate = true;
    else
      trackItem->setEnabled( true ); // files analysed, cd-text loaded
    item = item->nextSibling();
  }

  if( !animate ) {
    m_animationTimer->stop();
  }
}


void K3bAudioTrackView::slotDragTimeout()
{
  m_autoOpenTrackTimer->stop();

  if( m_currentMouseOverItem ) {
    m_currentMouseOverItem->showSources( true );
  }
}


void K3bAudioTrackView::getSelectedItems( QPtrList<K3bAudioTrack>& tracks, 
					  QPtrList<K3bAudioDataSource>& sources )
{
  tracks.clear();
  sources.clear();

  QPtrList<QListViewItem> items = selectedItems();
  for( QPtrListIterator<QListViewItem> it( items ); it.current(); ++it ) {
    if( K3bAudioTrackViewItem* tv = dynamic_cast<K3bAudioTrackViewItem*>( *it ) )
      tracks.append( tv->track() );
    else {
      K3bAudioDataSourceViewItem* sv = static_cast<K3bAudioDataSourceViewItem*>( *it );
      // do not select hidden source items or unfinished source files
      if( sv->trackViewItem()->showingSources() && 
	  !(sv->source()->isValid() && sv->source()->length() == 0) )
	sources.append( sv->source() );
    }
  }
}


void K3bAudioTrackView::slotRemove()
{
  QPtrList<K3bAudioTrack> tracks;
  QPtrList<K3bAudioDataSource> sources;
  getSelectedItems( tracks, sources );

  //
  // remove all sources which belong to one of the selected tracks since they will be 
  // deleted along with their tracks
  //
  QPtrListIterator<K3bAudioDataSource> srcIt( sources );
  while( srcIt.current() ) {
    if( tracks.containsRef( srcIt.current()->track() ) )
      sources.removeRef( *srcIt );
    else
      ++srcIt;
  }

  //
  // Now delete all the tracks
  //
  for( QPtrListIterator<K3bAudioTrack> it( tracks ); it.current(); ++it )
    delete *it;

  //
  // Now delete all the sources
  //
  for( QPtrListIterator<K3bAudioDataSource> it( sources ); it.current(); ++it )
    delete *it;
}


void K3bAudioTrackView::slotAddSilence()
{
  QListViewItem* item = selectedItems().first();
  if( item ) {
    K3bAudioZeroData* zero = new K3bAudioZeroData( m_doc );
    if( K3bAudioTrackViewItem* tv = dynamic_cast<K3bAudioTrackViewItem*>(item) ) {
      tv->track()->addSource( zero );
    }
    else if( K3bAudioDataSourceViewItem* sv = dynamic_cast<K3bAudioDataSourceViewItem*>(item) ) {
      zero->moveAfter( sv->source() );
    }
  }
}


void K3bAudioTrackView::slotMergeTracks()
{
  QPtrList<K3bAudioTrack> tracks;
  QPtrList<K3bAudioDataSource> sources;
  getSelectedItems( tracks, sources );

  // we simply merge the selected tracks ignoring any eventually selected sources
  K3bAudioTrack* firstTrack = tracks.first();
  tracks.remove();
  while( K3bAudioTrack* mergeTrack = tracks.first() ) {
    tracks.remove();
    firstTrack->merge( mergeTrack, firstTrack->lastSource() );
  }
}


void K3bAudioTrackView::slotSplitSource()
{
  QListViewItem* item = selectedItems().first();
  if( K3bAudioDataSourceViewItem* sv = dynamic_cast<K3bAudioDataSourceViewItem*>(item) ) {
    // create a new track
    K3bAudioTrack* track = new K3bAudioTrack( m_doc );
    K3bAudioTrack* trackAfter = sv->source()->track();
    if( trackAfter->numberSources() == 1 )
      trackAfter = trackAfter->prev();
    track->addSource( sv->source()->take() );
    track->moveAfter( trackAfter );
  }
}


void K3bAudioTrackView::showPopupMenu( KListView*, QListViewItem* item, const QPoint& pos )
{
  QPtrList<K3bAudioTrack> tracks;
  QPtrList<K3bAudioDataSource> sources;
  getSelectedItems( tracks, sources );

  int numTracks = tracks.count();
  int numSources = sources.count();

  // build the menu
  m_popupMenu->clear();

  if( item )
    m_actionRemove->plug( m_popupMenu );

  if( numSources + numTracks == 1 )
    m_actionAddSilence->plug( m_popupMenu );

  if( numSources == 1 && numTracks == 0 ) {
    m_popupMenu->insertSeparator();
    m_actionSplitSource->plug( m_popupMenu );
  }
  else if( numTracks > 1 && numSources == 0 ) {
    m_popupMenu->insertSeparator();
    m_actionMergeTracks->plug( m_popupMenu );
  }

  m_actionProperties->plug( m_popupMenu );
  m_popupMenu->insertSeparator();
  m_doc->actionCollection()->action( "project_burn" )->plug( m_popupMenu );

  m_popupMenu->popup( pos );
}

#include "k3baudiotrackview.moc"
