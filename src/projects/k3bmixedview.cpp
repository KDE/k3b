/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009      Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
 * Copyright (C) 2009      Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bmixedview.h"
#include "k3baudiodoc.h"
#include "k3baudioprojectmodel.h"
//#include "k3baudiotrackplayer.h"
#include "k3baudioviewimpl.h"
#include "k3bdatadoc.h"
#include "k3bdataprojectmodel.h"
#include "k3bdataviewimpl.h"
#include "k3bmixeddoc.h"
#include "k3bmixedburndialog.h"
#include "k3bmixedprojectmodel.h"
#include "k3bvolumenamewidget.h"

#include <KAction>
#include <KActionCollection>
#include <KDebug>
#include <KLocale>
#include <KMenu>
#include <KMessageBox>
#include <KToolBar>

K3b::MixedView::MixedView( K3b::MixedDoc* doc, QWidget* parent )
    : K3b::StandardView( doc, parent ), m_doc(doc)
{
    m_model = new MixedProjectModel(m_doc, this);
    m_audioViewImpl = new AudioViewImpl( this, m_doc->audioDoc(), m_model->audioModel(), actionCollection() );
    m_dataViewImpl = new DataViewImpl( this, m_doc->dataDoc(), m_model->dataModel(), actionCollection() );

    connect( this, SIGNAL( currentRootChanged(QModelIndex) ),
             m_model, SLOT( slotCurrentRootIndexChanged(QModelIndex) ) );
    connect( this, SIGNAL( currentRootChanged(QModelIndex) ), SLOT( slotCurrentRootChanged(QModelIndex) ) );
    connect( this, SIGNAL(activated(QModelIndex)), SLOT(slotItemActivated(QModelIndex)) );
    connect( m_dataViewImpl, SIGNAL(setCurrentRoot(QModelIndex)),
             this, SLOT(slotSetCurrentRoot(QModelIndex)) );
    
    // Connect audio actions
    connect( actionCollection()->action( "track_add_silence" ), SIGNAL( triggered() ),
             this, SLOT(slotAddSilence()) );
    connect( actionCollection()->action( "track_merge" ), SIGNAL( triggered() ),
             this, SLOT(slotMergeTracks()) );
    connect( actionCollection()->action( "source_split" ), SIGNAL( triggered() ),
             this, SLOT(slotSplitSource()) );
    connect( actionCollection()->action( "track_split" ), SIGNAL( triggered() ),
             this, SLOT(slotSplitTrack()) );
    connect( actionCollection()->action( "edit_source" ), SIGNAL( triggered() ),
             this, SLOT(slotEditSource()) );
    //connect( actionCollection()->action( "track_play" ), SIGNAL( triggered() ),
    //         this, SLOT(slotPlayTrack()) );
    connect( actionCollection()->action( "project_audio_musicbrainz" ), SIGNAL( triggered() ),
             this, SLOT(slotQueryMusicBrainz()) );
    connect( actionCollection()->action( "track_properties" ), SIGNAL( triggered() ),
             this, SLOT(slotItemProperties()) );
    connect( actionCollection()->action( "track_remove" ), SIGNAL( triggered() ),
             this, SLOT(slotRemove()) );
    
    // Connect data actions
    connect( actionCollection()->action( "new_dir" ), SIGNAL( triggered() ),
             this, SLOT(slotNewDir()) );
    connect( actionCollection()->action( "remove" ), SIGNAL( triggered() ),
             this, SLOT(slotRemove()) );
    connect( actionCollection()->action( "rename" ), SIGNAL( triggered() ),
             this, SLOT(slotRenameItem()) );
    connect( actionCollection()->action( "parent_dir" ), SIGNAL( triggered() ),
             this, SLOT(slotParentDir()) );
    connect( actionCollection()->action( "properties" ), SIGNAL( triggered() ),
             this, SLOT(slotItemProperties()) );
    connect( actionCollection()->action( "open" ), SIGNAL( triggered() ),
             this, SLOT(slotOpen()) );

    setModel(m_model);
    
    // Setup toolbar
    toolBox()->addAction( actionCollection()->action( "parent_dir" ) );
    toolBox()->addSeparator();
    addPluginButtons();
    toolBox()->addWidget( new VolumeNameWidget( doc->dataDoc(), toolBox() ) );

#ifdef __GNUC__
#warning enable player once ported to Phonon
#endif
//   toolBox()->addAction( m_audioListView->player()->action( K3b::AudioTrackPlayer::ACTION_PLAY ) );
//   toolBox()->addAction( m_audioListView->player()->action( K3b::AudioTrackPlayer::ACTION_PAUSE ) );
//   toolBox()->addAction( m_audioListView->player()->action( K3b::AudioTrackPlayer::ACTION_STOP ) );
//   toolBox()->addSpacing();
//   toolBox()->addAction( m_audioListView->player()->action( K3b::AudioTrackPlayer::ACTION_PREV ) );
//   toolBox()->addAction( m_audioListView->player()->action( K3b::AudioTrackPlayer::ACTION_NEXT ) );
//   toolBox()->addSpacing();
//   m_audioListView->player()->action( K3b::AudioTrackPlayer::ACTION_SEEK )->plug( toolBox() );
//   toolBox()->addSeparator();
}


K3b::MixedView::~MixedView()
{
}


K3b::AudioTrackPlayer* K3b::MixedView::player() const
{
    //return m_audioListView->player();
    return 0;
}


void K3b::MixedView::slotBurn()
{
    if( m_doc->audioDoc()->numOfTracks() == 0 || m_doc->dataDoc()->size() == 0 ) {
        KMessageBox::information( this, i18n("Please add files and audio titles to your project first."),
                                  i18n("No Data to Burn"), QString(), false );
    }
    else {
        K3b::ProjectBurnDialog* dlg = newBurnDialog( this );
        if( dlg ) {
            dlg->execBurnDialog(true);
            delete dlg;
        }
        else {
            kDebug() << "(K3b::Doc) Error: no burndialog available.";
        }
    }
}


void K3b::MixedView::addUrls( const KUrl::List& urls )
{
    if( currentSubModel() == m_model->dataModel() ) {
        QModelIndex parent = m_model->mapToSubModel( currentRoot() );
        m_dataViewImpl->addUrls( parent, urls );
    }
    else if( currentSubModel() == m_model->audioModel() ) {
        m_audioViewImpl->addUrls( urls );
    }
}


void K3b::MixedView::slotAddSilence()
{
    if( currentSubModel() == m_model->audioModel() ) {
        QModelIndexList selection;
        mapToSubModel( selection, currentSelection() );
        m_audioViewImpl->addSilence( selection );
    }
}


void K3b::MixedView::slotRemove()
{
    if( currentSubModel() == m_model->dataModel() ) {
        slotRemoveSelectedIndexes();
    }
    else if( currentSubModel() == m_model->audioModel() ) {
        QModelIndexList selection;
        mapToSubModel( selection, currentSelection() );
        m_audioViewImpl->remove( selection );
    }
}


void K3b::MixedView::slotMergeTracks()
{
    if( currentSubModel() == m_model->audioModel() ) {
        QModelIndexList selection;
        mapToSubModel( selection, currentSelection() );
        m_audioViewImpl->mergeTracks( selection );
    }
}


void K3b::MixedView::slotSplitSource()
{
    if( currentSubModel() == m_model->audioModel() ) {
        QModelIndexList selection;
        mapToSubModel( selection, currentSelection() );
        m_audioViewImpl->splitSource( selection );
    }
}


void K3b::MixedView::slotSplitTrack()
{
    if( currentSubModel() == m_model->audioModel() ) {
        QModelIndexList selection;
        mapToSubModel( selection, currentSelection() );
        m_audioViewImpl->splitTrack( selection );
    }
}


void K3b::MixedView::slotEditSource()
{
    if( currentSubModel() == m_model->audioModel() ) {
        QModelIndexList selection;
        mapToSubModel( selection, currentSelection() );
        m_audioViewImpl->editSource( selection );
    }
}


void K3b::MixedView::slotQueryMusicBrainz()
{
    if( currentSubModel() == m_model->audioModel() ) {
        QModelIndexList selection;
        mapToSubModel( selection, currentSelection() );
        m_audioViewImpl->queryMusicBrainz( selection );
    }
}


void K3b::MixedView::slotNewDir()
{
    if( currentSubModel() == m_model->dataModel() ) {
        QModelIndex parent = m_model->mapToSubModel( currentRoot() );
        m_dataViewImpl->newDir( parent );
    }
}


void K3b::MixedView::slotItemProperties()
{
    QModelIndexList selection;
    mapToSubModel( selection, currentSelection() );
    
    if( currentSubModel() == m_model->dataModel() ) {
        m_dataViewImpl->properties( selection );
    }
    else if( currentSubModel() == m_model->audioModel() ) {
        m_audioViewImpl->properties( selection );
    }
}


void K3b::MixedView::slotOpen()
{
    if( currentSubModel() == m_model->dataModel() ) {
        QModelIndexList selection;
        mapToSubModel( selection, currentSelection() );
        m_dataViewImpl->open( selection );
    }
}


void K3b::MixedView::slotCurrentRootChanged( const QModelIndex& newRoot )
{
    QAbstractItemModel* currentSubModel = m_model->subModelForIndex( newRoot );
    if( currentSubModel != 0 ) {
        m_model->setSupportedDragActions( currentSubModel->supportedDragActions() );
    }
    
    if( currentSubModel == m_model->dataModel() ) {
        m_dataViewImpl->slotCurrentRootChanged( m_model->mapToSubModel( newRoot ) );
    }
    else {
        m_dataViewImpl->slotCurrentRootChanged( QModelIndex() );
    }
}


void K3b::MixedView::slotItemActivated( const QModelIndex& index )
{
    if( currentSubModel() == m_model->dataModel() ) {
        m_dataViewImpl->slotItemActivated( m_model->mapToSubModel( index ) );
    }
    else if( currentSubModel() == m_model->audioModel() ) {
        m_audioViewImpl->slotItemActivated( m_model->mapToSubModel( index ) );
    }
}


void K3b::MixedView::slotSetCurrentRoot( const QModelIndex& index )
{
    StandardView::setCurrentRoot( m_model->mapFromSubModel( index ) );
}


QAbstractItemModel* K3b::MixedView::currentSubModel() const
{
    return m_model->subModelForIndex( currentRoot() );
}


K3b::ProjectBurnDialog* K3b::MixedView::newBurnDialog( QWidget* parent )
{
    return new K3b::MixedBurnDialog( m_doc, parent );
}


void K3b::MixedView::selectionChanged( const QModelIndexList& indexes )
{
    QModelIndexList selection;
    mapToSubModel( selection, indexes );
    
    if( currentSubModel() == m_model->dataModel() ) {
        m_dataViewImpl->slotSelectionChanged( selection );
    }
    else if( currentSubModel() == m_model->audioModel() ) {
        m_audioViewImpl->slotSelectionChanged( selection );
    }
}


void K3b::MixedView::contextMenu( const QPoint& pos )
{
    if( currentSubModel() == m_model->dataModel() )
        m_dataViewImpl->popupMenu()->exec( pos );
    else if( currentSubModel() == m_model->audioModel() )
        m_audioViewImpl->popupMenu()->exec( pos );
}


void K3b::MixedView::mapToSubModel( QModelIndexList& subIndexes, const QModelIndexList& indexes ) const
{
    foreach( const QModelIndex& index, indexes ) {
        subIndexes.push_back( m_model->mapToSubModel( index ) );
    }
}

#include "k3bmixedview.moc"
