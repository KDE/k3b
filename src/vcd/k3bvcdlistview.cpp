/*
*
* $Id$
* Copyright (C) 2003 Christian Kvasny <chris@k3b.org>
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

#include <qheader.h>
#include <qtimer.h>
#include <qdragobject.h>
#include <qpoint.h>
#include <qptrlist.h>
#include <qstringlist.h>
#include <qevent.h>
#include <qpainter.h>
#include <qfontmetrics.h>

#include <kiconloader.h>
#include <kurl.h>
#include <kurldrag.h>
#include <klocale.h>
#include <kaction.h>
#include <kpopupmenu.h>
#include <kdialog.h>

// K3b Includes
#include "k3bvcdlistview.h"
#include "k3bvcdlistviewitem.h"
#include "k3bvcdtrack.h"
#include "k3bvcdtrackdialog.h"
#include "k3bvcddoc.h"
#include <k3b.h>
#include <k3bview.h>

K3bVcdListView::K3bVcdListView( K3bView* view, K3bVcdDoc* doc, QWidget *parent, const char *name )
        : K3bListView( parent, name ), m_doc( doc ), m_view( view )
{
    setAcceptDrops( true );
    setDropVisualizer( true );
    setAllColumnsShowFocus( true );
    setDragEnabled( true );
    setSelectionModeExt( KListView::Extended );
    setItemsMovable( false );

    setNoItemText( i18n( "Use drag'n'drop to add mpeg video files to the project." ) + "\n"
                   + i18n( "After that press the burn button to write the CD." ) );

    setSorting( 0 );

    setupActions();
    setupPopupMenu();

    setupColumns();
    header() ->setClickEnabled( false );

    connect( this, SIGNAL( dropped( KListView*, QDropEvent*, QListViewItem* ) ),
             this, SLOT( slotDropped( KListView*, QDropEvent*, QListViewItem* ) ) );
    connect( this, SIGNAL( contextMenu( KListView*, QListViewItem*, const QPoint& ) ),
             this, SLOT( showPopupMenu( KListView*, QListViewItem*, const QPoint& ) ) );
    connect( this, SIGNAL( doubleClicked( QListViewItem*, const QPoint&, int ) ),
             this, SLOT( showPropertiesDialog() ) );

    connect( m_doc, SIGNAL( newTracks() ), this, SLOT( slotUpdateItems() ) );


    slotUpdateItems();
}

K3bVcdListView::~K3bVcdListView()
{}

void K3bVcdListView::setupColumns()
{
    addColumn( i18n( "No." ) );
    addColumn( i18n( "Title" ) );
    addColumn( i18n( "Type" ) );
    addColumn( i18n( "Size" ) );
    addColumn( i18n( "Display" ) );
    addColumn( i18n( "Fps" ) );
    addColumn( i18n( "Mbps" ) );
    addColumn( i18n( "Duration" ) );
    addColumn( i18n( "File Size" ) );
    addColumn( i18n( "Filename" ) );
}


void K3bVcdListView::setupActions()
{
    m_actionCollection = new KActionCollection( this );
    m_actionProperties = new KAction( i18n( "Properties..." ), "misc", 0, this, SLOT( showPropertiesDialog() ), actionCollection() );
    m_actionRemove = new KAction( i18n( "Remove" ), "editdelete", Key_Delete, this, SLOT( slotRemoveTracks() ), actionCollection() );

    // disabled by default
    m_actionRemove->setEnabled( false );
}


void K3bVcdListView::setupPopupMenu()
{
    m_popupMenu = new KPopupMenu( this, "VcdViewPopupMenu" );
    m_actionRemove->plug( m_popupMenu );
    m_popupMenu->insertSeparator();
    m_actionProperties->plug( m_popupMenu );
    m_popupMenu->insertSeparator();
    k3bMain() ->actionCollection() ->action( "file_burn" ) ->plug( m_popupMenu );
}


bool K3bVcdListView::acceptDrag( QDropEvent* e ) const
{
    // the first is for built-in item moving, the second for dropping urls
    return ( KListView::acceptDrag( e ) || QUriDrag::canDecode( e ) );
}


QDragObject* K3bVcdListView::dragObject()
{
    QPtrList<QListViewItem> list = selectedItems();

    if ( list.isEmpty() )
        return 0;

    QPtrListIterator<QListViewItem> it( list );
    KURL::List urls;

    for ( ; it.current(); ++it )
        urls.append( KURL( ( ( K3bVcdListViewItem* ) it.current() ) ->vcdTrack() ->absPath() ) );

    return KURLDrag::newDrag( urls, viewport() );
}


void K3bVcdListView::slotDropped( KListView*, QDropEvent* e, QListViewItem* after )
{
    if ( !e->isAccepted() )
        return ;

    int pos;
    if ( after == 0L )
        pos = 0;
    else
        pos = ( ( K3bVcdListViewItem* ) after ) ->vcdTrack() ->index() + 1;

    if ( e->source() == viewport() ) {
        QPtrList<QListViewItem> sel = selectedItems();
        QPtrListIterator<QListViewItem> it( sel );
        K3bVcdTrack* trackAfter = ( after ? ( ( K3bVcdListViewItem* ) after ) ->vcdTrack() : 0 );
        while ( it.current() ) {
            K3bVcdTrack * track = ( ( K3bVcdListViewItem* ) it.current() ) ->vcdTrack();
            m_doc->moveTrack( track, trackAfter );
            trackAfter = track;
            ++it;
        }

        sort();  // This is so lame!
    } else {
        KURL::List urls;
        KURLDrag::decode( e, urls );

        m_doc->addTracks( urls, pos );
    }
}


void K3bVcdListView::insertItem( QListViewItem* item )
{
    KListView::insertItem( item );

    // make sure at least one item is selected
    if ( selectedItems().isEmpty() ) {
        setSelected( firstChild(), true );
    }
}

void K3bVcdListView::showPopupMenu( KListView*, QListViewItem* _item, const QPoint& _point )
{
    if ( _item ) {
        m_actionRemove->setEnabled( true );
    } else {
        m_actionRemove->setEnabled( false );
    }

    m_popupMenu->popup( _point );
}

void K3bVcdListView::showPropertiesDialog()
{
    QPtrList<K3bVcdTrack> selected = selectedTracks();
    if ( !selected.isEmpty() && selected.count() == 1 ) {
        QPtrList<K3bVcdTrack> tracks = *m_doc->tracks();
        K3bVcdTrackDialog d( m_doc, tracks, selected, this );
        if ( d.exec() ) {
            repaint();
        }
    } else {
        m_view->burnDialog( false );
    }
}

QPtrList<K3bVcdTrack> K3bVcdListView::selectedTracks()
{
    QPtrList<K3bVcdTrack> selectedTracks;
    QPtrList<QListViewItem> selectedVI( selectedItems() );
    for ( QListViewItem * item = selectedVI.first(); item != 0; item = selectedVI.next() ) {
        K3bVcdListViewItem * vcdItem = dynamic_cast<K3bVcdListViewItem*>( item );
        if ( vcdItem ) {
            selectedTracks.append( vcdItem->vcdTrack() );
        }
    }

    return selectedTracks;
}


void K3bVcdListView::slotRemoveTracks()
{
    QPtrList<K3bVcdTrack> selected = selectedTracks();
    if ( !selected.isEmpty() ) {

        for ( K3bVcdTrack * track = selected.first(); track != 0; track = selected.next() ) {
            m_doc->removeTrack( track );

            // not best, I think we should connect to doc.removedTrack (but since there is only one view this is not important!)
            QListViewItem* viewItem = m_itemMap[ track ];
            m_itemMap.remove( track );
            delete viewItem;
        }
    }

    if ( m_doc->numOfTracks() == 0 ) {
        m_actionRemove->setEnabled( false );
    }
}


void K3bVcdListView::slotUpdateItems()
{
    // iterate through all doc-tracks and test if we have a listItem, if not, create one
    K3bVcdTrack * track = m_doc->first();
    K3bVcdTrack* lastTrack = 0;
    while ( track != 0 ) {
        if ( !m_itemMap.contains( track ) )
            m_itemMap.insert( track, new K3bVcdListViewItem( track, this, m_itemMap[ lastTrack ] ) );

        lastTrack = track;
        track = m_doc->next();
    }

    if ( m_doc->numOfTracks() > 0 ) {
        m_actionRemove->setEnabled( true );
        m_actionProperties->setEnabled( true );
    } else {
        m_actionRemove->setEnabled( false );
        m_actionProperties->setEnabled( false );
    }
}

#include "k3bvcdlistview.moc"
