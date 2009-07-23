/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009      Arthur Mello <arthur@mandriva.com>
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

#include "k3baudioviewimpl.h"

#include "k3baction.h"
#include "k3baudiodatasourceeditwidget.h"
#include "k3baudiodecoder.h"
#include "k3baudiofile.h"
#include "k3baudioprojectmodel.h"
#include "k3baudiotrack.h"
#include "k3baudiotrackaddingdialog.h"
#include "k3baudiotrackdialog.h"
#include "k3baudiotracksplitdialog.h"
#include "k3baudiozerodata.h"
#include "k3bmsfedit.h"
#include "k3bview.h"
// this is not here becasue of base_*.ui troubles
#include "../rip/k3baudioprojectconvertingdialog.h"

#include <config-k3b.h>

#include <KAction>
#include <KActionCollection>
#include <KLocale>
#include <KMenu>


K3b::AudioViewImpl::AudioViewImpl( View* view, AudioDoc* doc, AudioProjectModel* model, KActionCollection* actionCollection )
    : QObject( view ), m_view( view ), m_doc( doc ), m_model( model )
{
    m_actionAddSilence = createAction( m_view, i18n("Add Silence..."), 0, 0, 0, 0,
                                       actionCollection, "track_add_silence" );
    m_actionMergeTracks = createAction( m_view, i18n("Merge Tracks"), 0, 0, 0, 0,
                                        actionCollection, "track_merge" );
    m_actionSplitSource = createAction( m_view, i18n("Source to Track"), 0, 0, 0, 0,
                                        actionCollection, "source_split" );
    m_actionSplitTrack = createAction( m_view, i18n("Split Track..."), 0, 0, 0, 0,
                                       actionCollection, "track_split" );
    m_actionEditSource = createAction( m_view, i18n("Edit Source..."), 0, 0, 0, 0,
                                       actionCollection, "edit_source" );
    m_actionPlayTrack = createAction( m_view, i18n("Play Track"), "media-playback-start", 0, 0, 0,
                                      actionCollection, "track_play" );
    m_actionMusicBrainz = createAction( m_view, i18n("Musicbrainz Lookup"), "musicbrainz", 0, 0, 0,
                                        actionCollection, "project_audio_musicbrainz");
    m_actionMusicBrainz->setToolTip( i18n("Try to determine meta information over the Internet") );
    m_actionProperties = createAction( m_view, i18n("Properties"), "document-properties", 0, 0, 0,
                                       actionCollection, "track_properties" );
    m_actionRemove = createAction( m_view, i18n("Remove"), "edit-delete", Qt::Key_Delete, 0, 0,
                                   actionCollection, "track_remove" );
    m_conversionAction = createAction( m_view, i18n("Convert Tracks"), "edit-redo", 0, this, SLOT(slotAudioConversion()),
                                       actionCollection, "project_audio_convert" );
    
    // Create audio context menu
    m_popupMenu = new KMenu( m_view );
    m_popupMenu->addAction( m_actionPlayTrack );
    m_popupMenu->addSeparator();
    m_popupMenu->addAction( m_actionRemove );
    m_popupMenu->addAction( m_actionAddSilence );
    m_popupMenu->addSeparator();
    m_popupMenu->addAction( m_actionSplitSource );
    m_popupMenu->addAction( m_actionSplitTrack );
    m_popupMenu->addAction( m_actionEditSource );
    m_popupMenu->addAction( m_actionMergeTracks );
    m_popupMenu->addAction( m_actionProperties );
    m_popupMenu->addSeparator();
    m_popupMenu->addAction( m_actionMusicBrainz );
    m_popupMenu->addSeparator();
    m_popupMenu->addAction( actionCollection->action("project_burn") );
    
#ifndef HAVE_MUSICBRAINZ
    m_actionMusicBrainz->setEnabled( false );
    m_actionMusicBrainz->setVisible( false );
#endif
    
#ifdef __GNUC__
#warning enable player once ported to Phonon
#endif
    m_actionPlayTrack->setEnabled( false );
    m_actionPlayTrack->setVisible( false );
}


void K3b::AudioViewImpl::addUrls( const KUrl::List& urls )
{
    AudioTrackAddingDialog::addUrls( urls, m_doc, 0, 0, 0, m_view );
}

    
void K3b::AudioViewImpl::remove( const QModelIndexList& indexes )
{
    QList<AudioTrack*> tracks;
    QList<AudioDataSource*> sources;
    tracksForIndexes( tracks, indexes );
    sourcesForIndexes( sources, indexes );
    
    //
    // remove all sources which belong to one of the selected tracks since they will be
    // deleted along with their tracks
    //
    QList<AudioDataSource*>::iterator srcIt = sources.begin();
    while( srcIt != sources.end() ) {
        if( tracks.contains( ( *srcIt )->track() ) )
            srcIt = sources.erase( srcIt );
        else
            ++srcIt;
    }

    //
    // Now delete all the tracks
    //
    qDeleteAll( tracks );

    //
    // Now delete all the sources
    //
    qDeleteAll( sources );
}


void K3b::AudioViewImpl::addSilence( const QModelIndexList& indexes )
{
    QList<AudioTrack*> tracks;
    QList<AudioDataSource*> sources;
    tracksForIndexes( tracks, indexes );
    sourcesForIndexes( sources, indexes );

    if( !sources.isEmpty() || !tracks.isEmpty() ) {
        //
        // create a simple dialog for asking the length of the silence
        //
        KDialog dlg( m_view );
        QWidget* widget = dlg.mainWidget();
        dlg.setButtons(KDialog::Ok|KDialog::Cancel);
        dlg.setDefaultButton(KDialog::Ok);
        dlg.setCaption(i18n("Add Silence"));

        QHBoxLayout* dlgLayout = new QHBoxLayout( widget );
        dlgLayout->setMargin( 0 );
        QLabel* label = new QLabel( i18n("Length of silence:"), widget );
        MsfEdit* msfEdit = new MsfEdit( widget );
        msfEdit->setValue( 150 ); // 2 seconds default
        msfEdit->setFocus();
        dlgLayout->addWidget( label );
        dlgLayout->addWidget( msfEdit );

        if( dlg.exec() == QDialog::Accepted ) {
            AudioZeroData* zero = new AudioZeroData( msfEdit->value() );
            if ( !tracks.isEmpty() )
                tracks.first()->addSource( zero );
            else
                zero->moveAfter( sources.first() );
        }
    }
}


void K3b::AudioViewImpl::mergeTracks( const QModelIndexList& indexes )
{
    QList<AudioTrack*> tracks;
    tracksForIndexes( tracks, indexes );

    // we simply merge the selected tracks ignoring any eventually selected sources
    if ( !tracks.isEmpty() ) {
        AudioTrack* firstTrack = tracks.takeFirst();
        while( !tracks.isEmpty() ) {
            firstTrack->merge( tracks.takeFirst(), firstTrack->lastSource() );
        }
    }
}


void K3b::AudioViewImpl::splitSource( const QModelIndexList& indexes )
{
    QList<AudioDataSource*> sources;
    sourcesForIndexes( sources, indexes );

    if( !sources.isEmpty() ) {
        AudioDataSource* source = sources.first();
        // create a new track
        AudioTrack* track = new AudioTrack( m_doc );
        AudioTrack* trackAfter = source->track();
        if( trackAfter->numberSources() == 1 )
            trackAfter = trackAfter->prev();
        track->addSource( source->take() );
        track->moveAfter( trackAfter );

        // let's see if it's a file because in that case we can reuse the metainfo :)
        // TODO: maybe add meta data to sources
        if( AudioFile* file = dynamic_cast<AudioFile*>( track->firstSource() ) ) {
            track->setArtist( file->decoder()->metaInfo( AudioDecoder::META_ARTIST ) );
            track->setTitle( file->decoder()->metaInfo( AudioDecoder::META_TITLE ) );
            track->setSongwriter( file->decoder()->metaInfo( AudioDecoder::META_SONGWRITER ) );
            track->setComposer( file->decoder()->metaInfo( AudioDecoder::META_COMPOSER ) );
            track->setCdTextMessage( file->decoder()->metaInfo( AudioDecoder::META_COMMENT ) );
        }
    }
}


void K3b::AudioViewImpl::splitTrack( const QModelIndexList& indexes )
{
    QList<AudioTrack*> tracks;
    tracksForIndexes( tracks, indexes );

    if( !tracks.isEmpty() ) {
        AudioTrackSplitDialog::splitTrack( tracks.first(), m_view );
    }
}


void K3b::AudioViewImpl::editSource( const QModelIndexList& indexes )
{
    QList<AudioTrack*> tracks;
    QList<AudioDataSource*> sources;
    tracksForIndexes( tracks, indexes );
    sourcesForIndexes( sources, indexes );

    AudioDataSource* source = 0;
    if( !sources.isEmpty() && tracks.isEmpty() )
        source = sources.first();
    else if( !tracks.isEmpty() && sources.isEmpty() )
        source = tracks.first()->firstSource();

    if( source ) {
        KDialog dlg( m_view );
        dlg.setCaption( i18n("Edit Audio Track Source") );
        dlg.setButtons( KDialog::Ok|KDialog::Cancel );
        dlg.setDefaultButton( KDialog::Ok );

        AudioDataSourceEditWidget* editW = new AudioDataSourceEditWidget( &dlg );
        dlg.setMainWidget( editW );
        editW->loadSource( source );
        if( dlg.exec() == QDialog::Accepted ) {
            editW->saveSource();
        }
    }
}


void K3b::AudioViewImpl::properties( const QModelIndexList& indexes )
{
    QList<AudioTrack*> tracks;
    tracksForIndexes( tracks, indexes );

    // TODO: add tracks from sources to tracks

    if( !tracks.isEmpty() ) {
        AudioTrackDialog d( tracks, m_view );
        d.exec();
    }
    else {
        m_view->slotProperties();
    }
}


void K3b::AudioViewImpl::queryMusicBrainz( const QModelIndexList& indexes )
{
#ifdef HAVE_MUSICBRAINZ
    QList<AudioTrack*> tracks;
    tracksForIndexes( tracks, indexes );

    if( tracks.isEmpty() ) {
        KMessageBox::sorry( this, i18n("Please select an audio track.") );
        return;
    }

    // only one may use the tracks at the same time
//     if( m_currentlyPlayingTrack &&
//         tracks.containsRef( m_currentlyPlayingTrack ) )
//         m_player->stop();

    // now do the lookup on the files.
    AudioTrackTRMLookupDialog dlg( this );
    dlg.lookup( tracks );
#else
    Q_UNUSED( indexes );
#endif
}


void K3b::AudioViewImpl::slotSelectionChanged( const QModelIndexList& indexes )
{
    QList<K3b::AudioTrack*> tracks;
    QList<K3b::AudioDataSource*> sources;
    tracksForIndexes( tracks, indexes );
    sourcesForIndexes( sources, indexes );

    int numTracks = tracks.count();
    int numSources = sources.count();

    if( m_actionPlayTrack != 0 ) {
        m_actionPlayTrack->setVisible( numTracks >= 1 );
    }
    m_actionRemove->setVisible( numTracks + numSources );
    m_actionAddSilence->setVisible( numSources + numTracks == 1 );

    if( numSources == 1 && numTracks == 0 ) {
        m_actionSplitSource->setVisible( true );
        m_actionSplitTrack->setVisible( false );
        m_actionEditSource->setVisible( true );
        m_actionMergeTracks->setVisible( false );
    }
    else if( numTracks == 1 && numSources == 0 ) {
        m_actionSplitSource->setVisible( false );
        m_actionSplitTrack->setVisible( tracks.first()->length().lba() > 60 );
        m_actionEditSource->setVisible( true );
        m_actionMergeTracks->setVisible( false );
    }
    else {
        m_actionSplitSource->setVisible( false );
        m_actionSplitTrack->setVisible( false );
        m_actionEditSource->setVisible( false );
        m_actionMergeTracks->setVisible( numTracks > 1 );
    }
}


void K3b::AudioViewImpl::slotItemActivated( const QModelIndex& index )
{
    properties( QModelIndexList() << index );
}


void K3b::AudioViewImpl::slotAudioConversion()
{
    AudioProjectConvertingDialog dlg( m_doc, m_view );
    dlg.exec();
}


void K3b::AudioViewImpl::tracksForIndexes( QList<AudioTrack*>& tracks,
                                           const QModelIndexList& indexes ) const
{
    tracks.clear();
    
    foreach( const QModelIndex& index, indexes ) {
        if ( AudioTrack* track = m_model->trackForIndex( index ) ) {
            tracks << track;
        }
    }
}


void K3b::AudioViewImpl::sourcesForIndexes( QList<AudioDataSource*>& sources,
                                            const QModelIndexList& indexes ) const
{
    sources.clear();

    foreach( const QModelIndex& index, indexes ) {
        if ( AudioDataSource* source = m_model->sourceForIndex( index ) ) {
#ifdef __GNUC__
#warning Do not select hidden sources once the hiding of sources works again
#endif
            // do not select hidden source items or unfinished source files
            if( source->isValid() && source->length() != 0 ) {
                sources << source;
            }
        }
    }
}

#include "k3baudioviewimpl.moc"
