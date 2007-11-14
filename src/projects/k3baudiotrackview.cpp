/* 
 *
 * $Id$
 * Copyright (C) 2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include <config-k3b.h>

#include "k3baudiotrackview.h"
#include "k3baudiotrackviewitem.h"
#include "k3baudiodatasourceviewitem.h"
#include "k3baudiotrack.h"
#include "k3baudiodatasource.h"
#include "k3baudiotrackdialog.h"
#include "k3baudiodoc.h"
#include "k3baudiozerodata.h"
#include "k3baudiotracksplitdialog.h"
#include "k3baudiofile.h"
#include "k3baudiotrackplayer.h"
#include "k3baudiocdtrackdrag.h"
#include "k3baudiocdtracksource.h"
#include "k3baudiotracktrmlookupdialog.h"
#include "k3baudiodatasourceeditwidget.h"
#include "k3baudiotrackaddingdialog.h"

#include <k3bview.h>
#include <k3blistviewitemanimator.h>
#include <k3baudiodecoder.h>
#include <k3bmsfedit.h>

#include <q3header.h>
#include <qtimer.h>
#include <q3dragobject.h>
#include <qpoint.h>
#include <q3ptrlist.h>
#include <qstringlist.h>
#include <qevent.h>
#include <qpixmap.h>
#include <qpainter.h>
#include <qlayout.h>
#include <qlabel.h>
#include <q3valuelist.h>
//Added by qt3to4:
#include <Q3HBoxLayout>
#include <QDragLeaveEvent>
#include <QKeyEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QResizeEvent>
#include <QFocusEvent>
#include <QMouseEvent>
#include <Q3VBoxLayout>

#include <kurl.h>
#include <kurldrag.h>
#include <klocale.h>
#include <kaction.h>
#include <kmenu.h>
#include <kiconloader.h>
#include <kapplication.h>
#include <kmessagebox.h>
#include <kdialog.h>


K3bAudioTrackView::K3bAudioTrackView( K3bAudioDoc* doc, QWidget* parent )
  : K3bListView( parent ),
    m_doc(doc),
    m_updatingColumnWidths(false),
    m_currentMouseOverItem(0),
    m_currentlyPlayingTrack(0)
{
  m_player = new K3bAudioTrackPlayer( m_doc, this );
  connect( m_player, SIGNAL(playingTrack(K3bAudioTrack*)), this,
	   SLOT(showPlayerIndicator(K3bAudioTrack*)) );
  connect( m_player, SIGNAL(paused(bool)), this, SLOT(togglePauseIndicator(bool)) );
  connect( m_player, SIGNAL(stopped()), this, SLOT(removePlayerIndicator()) );

  setItemMargin( 5 );
  setAcceptDrops( true );
  setDropVisualizer( true );
  setAllColumnsShowFocus( true );
  setDragEnabled( true );
  //  setSelectionModeExt( K3ListView::Konqueror ); // FileManager in KDE3
  setSelectionModeExt( K3ListView::Extended );
  setItemsMovable( false );
  setAlternateBackground( QColor() ); // disable alternate colors

  setNoItemText( i18n("Use drag'n'drop to add audio files to the project.") + "\n"
		 + i18n("After that press the burn button to write the CD." ) );

  setupColumns();
  setupActions();

  m_playerItemAnimator = new K3bListViewItemAnimator( this );

  m_animationTimer = new QTimer( this );
  connect( m_animationTimer, SIGNAL(timeout()), this, SLOT(slotAnimation()) );

  m_autoOpenTrackTimer = new QTimer( this );
  connect( m_autoOpenTrackTimer, SIGNAL(timeout()), this, SLOT(slotDragTimeout()) );

  connect( this, SIGNAL(dropped(QDropEvent*, Q3ListViewItem*, Q3ListViewItem*)),
	   this, SLOT(slotDropped(QDropEvent*, Q3ListViewItem*, Q3ListViewItem*)) );
  connect( this, SIGNAL(contextMenu(K3ListView*, Q3ListViewItem*, const QPoint&)),
	   this, SLOT(showPopupMenu(K3ListView*, Q3ListViewItem*, const QPoint&)) );
  connect( this, SIGNAL(doubleClicked(Q3ListViewItem*, const QPoint&, int)),
	   this, SLOT(slotProperties()) );

  connect( doc, SIGNAL(changed()),
	   this, SLOT(slotChanged()) );
  connect( doc, SIGNAL(trackChanged(K3bAudioTrack*)),
	   this, SLOT(slotTrackChanged(K3bAudioTrack*)) );
  connect( doc, SIGNAL(trackRemoved(K3bAudioTrack*)),
	   this, SLOT(slotTrackRemoved(K3bAudioTrack*)) );

  slotChanged();

  // a little background pix hack because I am simply incapable of doing it another way. :(
//   static QPixmap s_bgPix("/tmp/trueg/audio_bg.png");
//   setK3bBackgroundPixmap( s_bgPix, TOP_LEFT );
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
  m_popupMenu = new KMenu( this );

  m_actionProperties = new KAction( i18n("Properties"), "misc",
				    KShortcut(), this, SLOT(slotProperties()), 
				    actionCollection(), "track_properties" );
  m_actionRemove = new KAction( i18n( "Remove" ), "editdelete",
				Qt::Key_Delete, this, SLOT(slotRemove()), 
				actionCollection(), "track_remove" );

  m_actionAddSilence = new KAction( i18n("Add Silence") + "...", "misc",
				    KShortcut(), this, SLOT(slotAddSilence()),
				    actionCollection(), "track_add_silence" );
  m_actionMergeTracks = new KAction( i18n("Merge Tracks"), "misc",
				     KShortcut(), this, SLOT(slotMergeTracks()),
				     actionCollection(), "track_merge" );
  m_actionSplitSource = new KAction( i18n("Source to Track"), "misc",
				     KShortcut(), this, SLOT(slotSplitSource()),
				     actionCollection(), "source_split" );
  m_actionSplitTrack = new KAction( i18n("Split Track..."), 0,
				    KShortcut(), this, SLOT(slotSplitTrack()),
				    actionCollection(), "track_split" );
  m_actionEditSource = new KAction( i18n("Edit Source..."), 0,
				    KShortcut(), this, SLOT(slotEditSource()),
				    actionCollection(), "source_edit" );
  m_actionPlayTrack = new KAction( i18n("Play Track"), "player_play",
				   KShortcut(), this, SLOT(slotPlayTrack()),
				   actionCollection(), "track_play" );
#ifdef HAVE_MUSICBRAINZ
  KAction* mbAction = new KAction( i18n("Musicbrainz Lookup"), "musicbrainz", 0, this, 
				   SLOT(slotQueryMusicBrainz()),
				   actionCollection(), "project_audio_musicbrainz" );
  mbAction->setToolTip( i18n("Try to determine meta information over the internet") );
#endif
}


bool K3bAudioTrackView::acceptDrag(QDropEvent* e) const
{
  // the first is for built-in item moving, the second for dropping urls, the third for dropping audio tracks
  return ( K3ListView::acceptDrag(e) || K3URLDrag::canDecode(e) || K3bAudioCdTrackDrag::canDecode(e) );
}


Q3DragObject* K3bAudioTrackView::dragObject()
{
  Q3PtrList<Q3ListViewItem> list = selectedItems();

  if( list.isEmpty() )
    return 0;

  KUrl::List urls;

  for( Q3PtrListIterator<Q3ListViewItem> it(list); it.current(); ++it ) {
    Q3ListViewItem* item = *it;
    // we simply ignore open track items to not include files twice
    // we also don't want the invisible source items
    Q3ListViewItem* parentItem = K3bListView::parentItem(item);
    if( !item->isOpen() && ( !parentItem || parentItem->isOpen() ) ) {
      if( K3bAudioDataSourceViewItem* sourceItem = dynamic_cast<K3bAudioDataSourceViewItem*>( item ) ) {
	if( K3bAudioFile* file = dynamic_cast<K3bAudioFile*>( sourceItem->source() ) )
	  urls.append( KUrl::fromPathOrUrl(file->filename()) );
      }
      else {
	K3bAudioTrackViewItem* trackItem = static_cast<K3bAudioTrackViewItem*>( item );
	K3bAudioDataSource* source = trackItem->track()->firstSource();
	while( source ) {
	  if( K3bAudioFile* file = dynamic_cast<K3bAudioFile*>( source ) )
	    urls.append( KUrl::fromPathOrUrl(file->filename()) );
	  source = source->next();
	}
      }
    }
  }

  return new K3URLDrag( urls, viewport() );
}


void K3bAudioTrackView::slotDropped( QDropEvent* e, Q3ListViewItem* parent, Q3ListViewItem* after )
{
  m_autoOpenTrackTimer->stop();

  if( !e->isAccepted() )
    return;

  m_dropTrackAfter = 0;
  m_dropTrackParent = 0;
  m_dropSourceAfter = 0;
  if( after ) {
    if( K3bAudioTrackViewItem* tv = dynamic_cast<K3bAudioTrackViewItem*>( after ) ) {
      m_dropTrackAfter = tv->track();
    }
    else if( K3bAudioDataSourceViewItem* sv = dynamic_cast<K3bAudioDataSourceViewItem*>( after ) ) {
      m_dropSourceAfter = sv->source();
    }
  }

  if( K3bAudioTrackViewItem* tv = dynamic_cast<K3bAudioTrackViewItem*>( parent ) ) {
    m_dropTrackParent = tv->track();
  }

  //
  // In case the sources are not shown we do not want to handle them because the average
  // user would be confused otherwise
  //
  if( m_dropTrackParent && !m_trackItemMap[m_dropTrackParent]->showingSources() ) {
    kDebug() << "(K3bAudioTrackView) dropped after track which does not show it's sources.";
    m_dropTrackAfter = m_dropTrackParent;
    m_dropTrackParent = 0;
  }

  if( e->source() == viewport() ) {

    bool copyItems = (e->action() == QDropEvent::Copy);

    // 1. tracks (with some of their sources) -> move complete tracks around
    // 2. sources (multiple sources) -> move the sources to the destination track
    // 3. tracks and sources (the latter without their track) -> ignore the latter sources
    Q3PtrList<K3bAudioTrack> tracks;
    Q3PtrList<K3bAudioDataSource> sources;
    getSelectedItems( tracks, sources );

    //
    // remove all sources which belong to one of the selected tracks since they will be 
    // moved along with their tracks
    //
    Q3PtrListIterator<K3bAudioDataSource> srcIt( sources );
    while( srcIt.current() ) {
      if( tracks.containsRef( srcIt.current()->track() ) )
	sources.removeRef( *srcIt );
      else
	++srcIt;
    }

    //
    // Now move (or copy) all the tracks
    //
    for( Q3PtrListIterator<K3bAudioTrack> it( tracks ); it.current(); ++it ) {
      K3bAudioTrack* track = *it;
      if( m_dropTrackParent ) {
	m_dropTrackParent->merge( copyItems ? track->copy() : track, m_dropSourceAfter );
      }
      else if( m_dropTrackAfter ) {
	if( copyItems )
	  track->copy()->moveAfter( m_dropTrackAfter );
	else
	  track->moveAfter( m_dropTrackAfter );
      }
      else {
	if( copyItems )
	  track->copy()->moveAhead( m_doc->firstTrack() );
	else
	  track->moveAhead( m_doc->firstTrack() );
      }
    }

    //
    // now move (or copy) the sources
    //
    for( Q3PtrListIterator<K3bAudioDataSource> it( sources ); it.current(); ++it ) {
      K3bAudioDataSource* source = *it;
      if( m_dropTrackParent ) {
	if( m_dropSourceAfter ) {
	  if( copyItems )
	    source->copy()->moveAfter( m_dropSourceAfter );
	  else
	    source->moveAfter( m_dropSourceAfter );
	}
	else {
	  if( copyItems )
	    source->copy()->moveAhead( m_dropTrackParent->firstSource() );
	  else
	    source->moveAhead( m_dropTrackParent->firstSource() );
	}
      }
      else {
	// create a new track
	K3bAudioTrack* track = new K3bAudioTrack( m_doc );

	// special case: the source we remove from the track is the last and the track
	// will be deleted.
	if( !copyItems && m_dropTrackAfter == source->track() && m_dropTrackAfter->numberSources() == 1 )
	  m_dropTrackAfter = m_dropTrackAfter->prev();

	if( copyItems )
	  track->addSource( source->copy() );
	else
	  track->addSource( source );

	if( m_dropTrackAfter ) {
	  track->moveAfter( m_dropTrackAfter );
	  m_dropTrackAfter = track;
	}
	else {
	  track->moveAhead( m_doc->firstTrack() );
	  m_dropTrackAfter = track;
	}
      }
    }
  }
  else if( K3bAudioCdTrackDrag::canDecode( e ) ) {
    kDebug() << "(K3bAudioTrackView) audiocdtrack dropped.";
    K3bDevice::Toc toc;
    K3bDevice::Device* dev = 0;
    K3bCddbResultEntry cddb;
    Q3ValueList<int> trackNumbers;

    K3bAudioCdTrackDrag::decode( e, toc, trackNumbers, cddb, &dev );

    // for now we just create one source
    for( Q3ValueList<int>::const_iterator it = trackNumbers.begin();
	 it != trackNumbers.end(); ++it ) {
      int trackNumber = *it;

      K3bAudioCdTrackSource* source = new K3bAudioCdTrackSource( toc, trackNumber, cddb, dev );
      if( m_dropTrackParent ) {
	source->moveAfter( m_dropSourceAfter );
	if( m_dropSourceAfter )
	  m_dropSourceAfter = source;
      }
      else {
	K3bAudioTrack* track = new K3bAudioTrack();
	track->setPerformer( cddb.artists[trackNumber-1] );
	track->setTitle( cddb.titles[trackNumber-1] );
	track->addSource( source );
	if( m_dropTrackAfter )
	  track->moveAfter( m_dropTrackAfter );
	else
	  m_doc->addTrack( track, 0 );

	m_dropTrackAfter = track;
      }
    }
  }
  else{
    m_dropUrls.clear();
    if( K3URLDrag::decode( e, m_dropUrls ) ) {
      //
      // This is a small (not to ugly) hack to circumvent problems with the
      // event queues: the url adding dialog will be non-modal regardless of
      // the settings in case we open it directly.
      //
      QTimer::singleShot( 0, this, SLOT(slotAddUrls()) );
    }
  }
  
  showAllSources();
  
  // now grab that focus
  setFocus();
}


void K3bAudioTrackView::slotAddUrls()
{
  K3bAudioTrackAddingDialog::addUrls( m_dropUrls, m_doc, m_dropTrackAfter, m_dropTrackParent, m_dropSourceAfter, this );
}


void K3bAudioTrackView::slotChanged()
{
  kDebug() << "(K3bAudioTrackView::slotChanged)";
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

  header()->setShown( m_doc->numOfTracks() > 0 );

  kDebug() << "(K3bAudioTrackView::slotChanged) finished";
}


K3bAudioTrackViewItem* K3bAudioTrackView::getTrackViewItem( K3bAudioTrack* track, bool* isNewItem )
{
  QMap<K3bAudioTrack*, K3bAudioTrackViewItem*>::iterator itemIt = m_trackItemMap.find(track);
  if( itemIt == m_trackItemMap.end() ) {
    kDebug() << "(K3bAudioTrackView) new track " << track;
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
  kDebug() << "(K3bAudioTrackView::slotTrackChanged( " << track << " )";

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
  kDebug() << "(K3bAudioTrackView::slotTrackChanged( " << track << " ) finished";
}


void K3bAudioTrackView::slotTrackRemoved( K3bAudioTrack* track )
{
  kDebug() << "(K3bAudioTrackView::slotTrackRemoved( " << track << " )";
  if ( m_playerItemAnimator->item() == m_trackItemMap[track] ) {
      m_playerItemAnimator->stop();
  }
  delete m_trackItemMap[track];
  m_trackItemMap.erase(track);
}


void K3bAudioTrackView::showAllSources()
{
  // TODO: add an action to show all sources

  Q3ListViewItem* item = firstChild();
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
  Q3ListViewItem* parent = 0;
  Q3ListViewItem* after = 0;
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
    kDebug() << "(K3bAudioTrackView) already updating column widths.";
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

  for( Q3ListViewItemIterator it( this ); it.current(); ++it ) {
    artistWidth = qMax( artistWidth, it.current()->width( fontMetrics(), this, 1 ) );
    titleWidth = qMax( titleWidth, it.current()->width( fontMetrics(), this, 2 ) );
    typeWidth = qMax( typeWidth, it.current()->width( fontMetrics(), this, 3 ) );
    lengthWidth = qMax( lengthWidth, it.current()->width( fontMetrics(), this, 4 ) );
    filenameWidth = qMax( filenameWidth, it.current()->width( fontMetrics(), this, 5 ) );
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
  resizeColumns();
  Q3ListViewItem* item = firstChild();

  bool animate = false;

  while( item ) {
    K3bAudioTrackViewItem* trackItem = dynamic_cast<K3bAudioTrackViewItem*>(item);
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


void K3bAudioTrackView::getSelectedItems( Q3PtrList<K3bAudioTrack>& tracks, 
					  Q3PtrList<K3bAudioDataSource>& sources )
{
  tracks.clear();
  sources.clear();

  Q3PtrList<Q3ListViewItem> items = selectedItems();
  for( Q3PtrListIterator<Q3ListViewItem> it( items ); it.current(); ++it ) {
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
  Q3PtrList<K3bAudioTrack> tracks;
  Q3PtrList<K3bAudioDataSource> sources;
  getSelectedItems( tracks, sources );

  //
  // remove all sources which belong to one of the selected tracks since they will be 
  // deleted along with their tracks
  //
  Q3PtrListIterator<K3bAudioDataSource> srcIt( sources );
  while( srcIt.current() ) {
    if( tracks.containsRef( srcIt.current()->track() ) )
      sources.removeRef( *srcIt );
    else
      ++srcIt;
  }

  //
  // Now delete all the tracks
  //
  for( Q3PtrListIterator<K3bAudioTrack> it( tracks ); it.current(); ++it )
    delete *it;

  //
  // Now delete all the sources
  //
  for( Q3PtrListIterator<K3bAudioDataSource> it( sources ); it.current(); ++it )
    delete *it;
}


void K3bAudioTrackView::slotAddSilence()
{
  Q3ListViewItem* item = selectedItems().first();
  if( item ) {
    //
    // create a simple dialog for asking the length of the silence
    //
    KDialog dlg( KDialogBase::Plain, 
		     i18n("Add Silence"),
		     KDialog::Ok|KDialogBase::Cancel,
		     KDialog::Ok,
		     this );
    Q3HBoxLayout* dlgLayout = new Q3HBoxLayout( dlg.plainPage(), 0, KDialog::spacingHint() );
    dlgLayout->setAutoAdd( true );
    (void)new QLabel( i18n("Length of silence:"), dlg.plainPage() );
    K3bMsfEdit* msfEdit = new K3bMsfEdit( dlg.plainPage() );
    msfEdit->setValue( 150 ); // 2 seconds default
    msfEdit->setFocus();

    if( dlg.exec() == QDialog::Accepted ) {
      K3bAudioZeroData* zero = new K3bAudioZeroData( msfEdit->value() );
      if( K3bAudioTrackViewItem* tv = dynamic_cast<K3bAudioTrackViewItem*>(item) ) {
	tv->track()->addSource( zero );
      }
      else if( K3bAudioDataSourceViewItem* sv = dynamic_cast<K3bAudioDataSourceViewItem*>(item) ) {
	zero->moveAfter( sv->source() );
      }
    }
  }
}


void K3bAudioTrackView::slotMergeTracks()
{
  Q3PtrList<K3bAudioTrack> tracks;
  Q3PtrList<K3bAudioDataSource> sources;
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
  Q3ListViewItem* item = selectedItems().first();
  if( K3bAudioDataSourceViewItem* sv = dynamic_cast<K3bAudioDataSourceViewItem*>(item) ) {
    // create a new track
    K3bAudioTrack* track = new K3bAudioTrack( m_doc );
    K3bAudioTrack* trackAfter = sv->source()->track();
    if( trackAfter->numberSources() == 1 )
      trackAfter = trackAfter->prev();
    track->addSource( sv->source()->take() );
    track->moveAfter( trackAfter );

    // let's see if it's a file because in that case we can reuse the metainfo :)
    // TODO: maybe add meta data to sources
    if( K3bAudioFile* file = dynamic_cast<K3bAudioFile*>( track->firstSource() ) ) {
      track->setArtist( file->decoder()->metaInfo( K3bAudioDecoder::META_ARTIST ) );
      track->setTitle( file->decoder()->metaInfo( K3bAudioDecoder::META_TITLE ) );
      track->setSongwriter( file->decoder()->metaInfo( K3bAudioDecoder::META_SONGWRITER ) );
      track->setComposer( file->decoder()->metaInfo( K3bAudioDecoder::META_COMPOSER ) );
      track->setCdTextMessage( file->decoder()->metaInfo( K3bAudioDecoder::META_COMMENT ) );
    }
  }
}


void K3bAudioTrackView::slotSplitTrack()
{
  Q3ListViewItem* item = selectedItems().first();
  if( K3bAudioTrackViewItem* tv = dynamic_cast<K3bAudioTrackViewItem*>(item) ) {
    K3bAudioTrackSplitDialog::splitTrack( tv->track(), this );
  }
}


void K3bAudioTrackView::slotEditSource()
{
  Q3ListViewItem* item = selectedItems().first();

  K3bAudioDataSource* source = 0;
  if( K3bAudioDataSourceViewItem* sv = dynamic_cast<K3bAudioDataSourceViewItem*>(item) )
    source = sv->source();
  else if( K3bAudioTrackViewItem* tv = dynamic_cast<K3bAudioTrackViewItem*>(item) )
    source = tv->track()->firstSource();

  if( source ) {
    KDialog dlg( KDialogBase::Plain,
		     i18n("Edit Audio Track Source"),
		     KDialog::Ok|KDialogBase::Cancel,
		     KDialog::Ok,
		     this,
		     0,
		     true,
		     true );
    Q3VBoxLayout* lay = new Q3VBoxLayout( dlg.plainPage() );
    lay->setMargin( 0 );
    lay->setSpacing( KDialog::spacingHint() );
    lay->setAutoAdd( true );
    K3bAudioDataSourceEditWidget* editW = new K3bAudioDataSourceEditWidget( dlg.plainPage() );
    editW->loadSource( source );
    if( dlg.exec() == QDialog::Accepted )
      editW->saveSource();
  }
}


void K3bAudioTrackView::showPopupMenu( K3ListView*, Q3ListViewItem* item, const QPoint& pos )
{
  Q3PtrList<K3bAudioTrack> tracks;
  Q3PtrList<K3bAudioDataSource> sources;
  getSelectedItems( tracks, sources );

  int numTracks = tracks.count();
  int numSources = sources.count();

  // build the menu
  m_popupMenu->clear();

  if( numTracks >= 1 ) {
    m_actionPlayTrack->plug( m_popupMenu );
    m_popupMenu->insertSeparator();
  }

  if( item )
    m_actionRemove->plug( m_popupMenu );

  if( numSources + numTracks == 1 )
    m_actionAddSilence->plug( m_popupMenu );

  if( numSources == 1 && numTracks == 0 ) {
    m_popupMenu->insertSeparator();
    m_actionSplitSource->plug( m_popupMenu );
    m_actionEditSource->plug( m_popupMenu );
  }
  else if( numTracks == 1 && numSources == 0 ) {
    m_popupMenu->insertSeparator();
   
    
  
    if( K3bAudioTrackViewItem* tv = dynamic_cast<K3bAudioTrackViewItem*>(item) )
       if( tv->track()->length().lba() > 60 )
          m_actionSplitTrack->plug( m_popupMenu );
    
    m_actionEditSource->plug( m_popupMenu );
 
  }
  else if( numTracks > 1 ) {
    m_popupMenu->insertSeparator();
    m_actionMergeTracks->plug( m_popupMenu );
  }

  m_actionProperties->plug( m_popupMenu );
  m_popupMenu->insertSeparator();
  static_cast<K3bView*>(m_doc->view())->actionCollection()->action( "project_burn" )->plug( m_popupMenu );

  m_popupMenu->popup( pos );
}


void K3bAudioTrackView::slotProperties()
{
  Q3PtrList<K3bAudioTrack> tracks;
  Q3PtrList<K3bAudioDataSource> sources;
  getSelectedItems( tracks, sources );

  // TODO: add tracks from sources to tracks

  if( !tracks.isEmpty() ) {
    K3bAudioTrackDialog d( tracks, this );
     d.exec();
  }  
  else {
    static_cast<K3bView*>(m_doc->view())->slotProperties();
  }
}


void K3bAudioTrackView::slotPlayTrack()
{
  Q3PtrList<K3bAudioTrack> tracks;
  Q3PtrList<K3bAudioDataSource> sources;
  getSelectedItems( tracks, sources );
  if( tracks.count() > 0 )
    m_player->playTrack( tracks.first() );
}


void K3bAudioTrackView::showPlayerIndicator( K3bAudioTrack* track )
{
  removePlayerIndicator();
  m_currentlyPlayingTrack = track;
  K3bAudioTrackViewItem* item = getTrackViewItem( track );
  item->setPixmap( 1, SmallIcon( "player_play" ) );  
  m_playerItemAnimator->setItem( item, 1 );
}


void K3bAudioTrackView::togglePauseIndicator( bool b )
{
  if( m_currentlyPlayingTrack ) {
    if( b )
      m_playerItemAnimator->setPixmap( SmallIcon( "player_pause" ) );
    else
      m_playerItemAnimator->setPixmap( SmallIcon( "player_play" ) );
  }
}


void K3bAudioTrackView::removePlayerIndicator()
{
  if( m_currentlyPlayingTrack )
    getTrackViewItem( m_currentlyPlayingTrack )->setPixmap( 1, QPixmap() );
  m_playerItemAnimator->stop();
  m_currentlyPlayingTrack = 0;
}


void K3bAudioTrackView::slotQueryMusicBrainz()
{
#ifdef HAVE_MUSICBRAINZ
  Q3PtrList<K3bAudioTrack> tracks;
  Q3PtrList<K3bAudioDataSource> sources;
  getSelectedItems( tracks, sources );

  if( tracks.isEmpty() ) {
    KMessageBox::sorry( this, i18n("Please select an audio track.") );
    return;
  }

  // only one may use the tracks at the same time
  if( m_currentlyPlayingTrack &&
      tracks.containsRef( m_currentlyPlayingTrack ) )
    m_player->stop();

  // now do the lookup on the files.
  K3bAudioTrackTRMLookupDialog dlg( this );
  dlg.lookup( tracks );
#endif
}

#include "k3baudiotrackview.moc"
