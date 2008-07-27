/*
*
* Copyright (C) 2003-2004 Christian Kvasny <chris@k3b.org>
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

// K3b Includes
#include "k3bvcdlistview.h"
#include "k3bvcdlistviewitem.h"
#include "k3bvcdtrack.h"
#include "k3bvcdtrackdialog.h"
#include "k3bvcddoc.h"
#include <k3bview.h>
#include <k3baction.h>

#include <q3header.h>
#include <qtimer.h>
#include <q3dragobject.h>
#include <qpoint.h>
#include <qlist.h>
#include <qstringlist.h>
#include <qevent.h>
#include <qpainter.h>
#include <qfontmetrics.h>
//Added by qt3to4:
#include <QDropEvent>

#include <kiconloader.h>
#include <kurl.h>
#include <k3urldrag.h>
#include <klocale.h>
#include <kaction.h>
#include <kmenu.h>
#include <kdialog.h>
#include <kactioncollection.h>

K3bVcdListView::K3bVcdListView( K3bView* view, K3bVcdDoc* doc, QWidget *parent )
        : K3bListView( parent ), m_doc( doc ), m_view( view )
{
    setAcceptDrops( true );
    setDropVisualizer( true );
    setAllColumnsShowFocus( true );
    setDragEnabled( true );
    setSelectionModeExt( K3ListView::Extended );
    setItemsMovable( false );

    setNoItemText( i18n( "Use drag'n'drop to add MPEG video files to the project." ) + "\n"
                   + i18n( "After that press the burn button to write the CD." ) );

    setSorting( 0 );

    setupActions();
    setupPopupMenu();

    setupColumns();
    header() ->setClickEnabled( false );

    connect( this, SIGNAL( dropped( K3ListView*, QDropEvent*, Q3ListViewItem* ) ),
             this, SLOT( slotDropped( K3ListView*, QDropEvent*, Q3ListViewItem* ) ) );
    connect( this, SIGNAL( contextMenu( K3ListView*, Q3ListViewItem*, const QPoint& ) ),
             this, SLOT( showPopupMenu( K3ListView*, Q3ListViewItem*, const QPoint& ) ) );
    connect( this, SIGNAL( doubleClicked( Q3ListViewItem*, const QPoint&, int ) ),
             this, SLOT( showPropertiesDialog() ) );

    connect( m_doc, SIGNAL( changed() ), this, SLOT( slotUpdateItems() ) );
    connect( m_doc, SIGNAL( trackRemoved( K3bVcdTrack* ) ), this, SLOT( slotTrackRemoved( K3bVcdTrack* ) ) );

    slotUpdateItems();
}

K3bVcdListView::~K3bVcdListView()
{}

void K3bVcdListView::setupColumns()
{
    addColumn( i18n( "No." ) );
    addColumn( i18n( "Title" ) );
    addColumn( i18n( "Type" ) );
    addColumn( i18n( "Resolution" ) );
    addColumn( i18n( "High Resolution" ) );
    addColumn( i18n( "Framerate" ) );
    addColumn( i18n( "Muxrate" ) );
    addColumn( i18n( "Duration" ) );
    addColumn( i18n( "File Size" ) );
    addColumn( i18n( "Filename" ) );
}


void K3bVcdListView::setupActions()
{
    m_actionCollection = new KActionCollection( this );
    m_actionProperties = K3b::createAction( this, i18n( "Properties" ), "document-properties", 0, this, SLOT( showPropertiesDialog() ), actionCollection() );
    m_actionRemove = K3b::createAction( this, i18n( "Remove" ), "edit-delete", Qt::Key_Delete, this, SLOT( slotRemoveTracks() ), actionCollection() );

    // disabled by default
    m_actionRemove->setEnabled( false );
}


void K3bVcdListView::setupPopupMenu()
{
    m_popupMenu = new KMenu( this );
    m_popupMenu->addAction( m_actionRemove );
    m_popupMenu->addSeparator();
    m_popupMenu->addAction( m_actionProperties );
    m_popupMenu->addSeparator();
    m_popupMenu->addAction( m_view->actionCollection() ->action( "project_burn" ) );
}


bool K3bVcdListView::acceptDrag( QDropEvent* e ) const
{
    // the first is for built-in item moving, the second for dropping urls
    return ( K3ListView::acceptDrag( e ) || K3URLDrag::canDecode( e ) );
}


Q3DragObject* K3bVcdListView::dragObject()
{
    QList<Q3ListViewItem*> list = selectedItems();

    if ( list.isEmpty() )
        return 0;

    KUrl::List urls;

    Q_FOREACH( Q3ListViewItem* item, list )
        urls.append( KUrl( ( ( K3bVcdListViewItem* ) item ) ->vcdTrack() ->absolutePath() ) );

    return K3URLDrag::newDrag( urls, viewport() );
}


void K3bVcdListView::slotDropped( K3ListView*, QDropEvent* e, Q3ListViewItem* after )
{
    if ( !e->isAccepted() )
        return ;

    int pos;
    if ( after == 0L )
        pos = 0;
    else
        pos = ( ( K3bVcdListViewItem* ) after ) ->vcdTrack() ->index() + 1;

    if ( e->source() == viewport() ) {
        QList<Q3ListViewItem*> sel = selectedItems();
        K3bVcdTrack* trackAfter = ( after ? ( ( K3bVcdListViewItem* ) after ) ->vcdTrack() : 0 );
        Q_FOREACH( Q3ListViewItem* item, sel ) {
            K3bVcdTrack * track = ( ( K3bVcdListViewItem* ) item ) ->vcdTrack();
            m_doc->moveTrack( track, trackAfter );
            trackAfter = track;
        }
    } else {
        KUrl::List urls;
        K3URLDrag::decode( e, urls );

        m_doc->addTracks( urls, pos );
    }

  // now grab that focus
  setFocus();
}


void K3bVcdListView::insertItem( Q3ListViewItem* item )
{
    K3ListView::insertItem( item );

    // make sure at least one item is selected
    if ( selectedItems().isEmpty() ) {
        setSelected( firstChild(), true );
    }
}

void K3bVcdListView::showPopupMenu( K3ListView*, Q3ListViewItem* _item, const QPoint& _point )
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
    QList<K3bVcdTrack*> selected = selectedTracks();
    if ( !selected.isEmpty() && selected.count() == 1 ) {
        QList<K3bVcdTrack*> tracks = *m_doc->tracks();
        K3bVcdTrackDialog d( m_doc, tracks, selected, this );
        if ( d.exec() ) {
            repaint();
        }
    } else {
      m_view->slotProperties();
    }
}

QList<K3bVcdTrack*> K3bVcdListView::selectedTracks()
{
    QList<K3bVcdTrack*> selectedTracks;
    QList<Q3ListViewItem*> selectedVI( selectedItems() );
    Q_FOREACH( Q3ListViewItem* item, selectedVI ) {
        K3bVcdListViewItem * vcdItem = dynamic_cast<K3bVcdListViewItem*>( item );
        if ( vcdItem ) {
            selectedTracks.append( vcdItem->vcdTrack() );
        }
    }

    return selectedTracks;
}


void K3bVcdListView::slotRemoveTracks()
{
    QList<K3bVcdTrack*> selected = selectedTracks();
    if ( !selected.isEmpty() ) {
        Q_FOREACH( K3bVcdTrack* track,  selected ) {
            m_doc->removeTrack( track );
        }
    }

    if ( m_doc->numOfTracks() == 0 ) {
        m_actionRemove->setEnabled( false );
    }
}


void K3bVcdListView::slotTrackRemoved( K3bVcdTrack* track )
{
    Q3ListViewItem * viewItem = m_itemMap[ track ];
    m_itemMap.remove( track );
    delete viewItem;
}


void K3bVcdListView::slotUpdateItems()
{
    // iterate through all doc-tracks and test if we have a listItem, if not, create one
    QList<K3bVcdTrack*> tracks = *m_doc->tracks();
    K3bVcdTrack* lastTrack = 0;
    Q_FOREACH( K3bVcdTrack* track, tracks ) {
        if ( !m_itemMap.contains( track ) )
            m_itemMap.insert( track, new K3bVcdListViewItem( track, this, m_itemMap[ lastTrack ] ) );

        lastTrack = track;
    }

    if ( m_doc->numOfTracks() > 0 ) {
        m_actionRemove->setEnabled( true );
    } else {
        m_actionRemove->setEnabled( false );
    }

    sort();  // This is so lame!

    header()->setVisible( m_doc->numOfTracks() > 0 );
}

#include "k3bvcdlistview.moc"
