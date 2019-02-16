/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009      Arthur Mello <arthur@mandriva.com>
 * Copyright (C) 2009      Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
 * Copyright (C) 2009-2010      Michal Malek <michalm@jabster.pl>
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
#include "k3baudiodoc.h"
#include "k3baudiofile.h"
#include "k3baudioprojectdelegate.h"
#include "k3baudioprojectmodel.h"
#include "k3baudiotrack.h"
#include "k3baudiotrackaddingdialog.h"
#include "k3baudiotrackdialog.h"
#include "k3baudiotracksplitdialog.h"
#include "k3baudiotracktrmlookupdialog.h"
#include "k3baudiozerodata.h"
#include "k3bmsfedit.h"
#include "k3bview.h"
#include "k3bviewcolumnadjuster.h"
// this is not here becasue of base_*.ui troubles
#include "../rip/k3baudioprojectconvertingdialog.h"

#include "config-k3b.h"
#ifdef ENABLE_AUDIO_PLAYER
#include "k3baudiotrackplayer.h"
#endif // ENABLE_AUDIO_PLAYER

#include <KLocalizedString>
#include <KMessageBox>
#include <KActionCollection>

#include <QAction>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QScrollBar>
#include <QTreeView>


K3b::AudioViewImpl::AudioViewImpl( View* view, AudioDoc* doc, KActionCollection* actionCollection )
:
    QObject( view ),
    m_view( view ),
    m_doc( doc ),
    m_model( new AudioProjectModel( doc, view ) ),
    m_trackView( new QTreeView( view ) ),
    m_delegate( new AudioProjectDelegate( *m_trackView, this ) ),
#ifdef ENABLE_AUDIO_PLAYER
    m_player( new AudioTrackPlayer( doc, actionCollection, this ) ),
#else
    m_player( 0 ),
#endif // ENABLE_AUDIO_PLAYER
    m_columnAdjuster( new ViewColumnAdjuster( this ) ),
    m_updatingColumnWidths( false )
{
    m_trackView->setModel( m_model );
    m_trackView->setItemDelegate( m_delegate );
    m_trackView->setAcceptDrops( true );
    m_trackView->setDragEnabled( true );
    m_trackView->setDragDropMode( QTreeView::DragDrop );
    m_trackView->setItemsExpandable( false );
    m_trackView->setRootIsDecorated( false );
    m_trackView->setSelectionMode( QTreeView::ExtendedSelection );
    m_trackView->setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );

    m_columnAdjuster->setView( m_trackView );

    m_actionAddSilence = createAction( m_view, i18n("Add Silence..."), 0, 0, this, SLOT(slotAddSilence()),
                                       actionCollection, "track_add_silence" );
    m_actionMergeTracks = createAction( m_view, i18n("Merge Tracks"), 0, 0, this, SLOT(slotMergeTracks()),
                                        actionCollection, "track_merge" );
    m_actionSplitSource = createAction( m_view, i18n("Source to Track"), 0, 0, this, SLOT(slotSplitSource()),
                                        actionCollection, "source_split" );
    m_actionSplitTrack = createAction( m_view, i18n("Split Track..."), 0, 0, this, SLOT(slotSplitTrack()),
                                       actionCollection, "track_split" );
    m_actionEditSource = createAction( m_view, i18n("Edit Source..."), 0, 0, this, SLOT(slotEditSource()),
                                       actionCollection, "edit_source" );
    m_actionPlayTrack = createAction( m_view, i18n("Play Track"), "media-playback-start", 0, this, SLOT(slotPlayTrack()),
                                      actionCollection, "track_play" );
    m_actionQueryMusicBrainz = createAction( m_view, i18n("Musicbrainz Lookup"), "musicbrainz", 0, this, SLOT(slotQueryMusicBrainz()),
                                        actionCollection, "project_audio_musicbrainz");
    m_actionQueryMusicBrainz->setToolTip( i18n("Try to determine meta information over the Internet") );
    m_actionQueryMusicBrainzTrack = createAction( m_view, i18n("Musicbrainz Lookup"), "musicbrainz", 0, this, SLOT(slotQueryMusicBrainzTrack()),
                                        actionCollection, "project_audio_musicbrainz_track");
    m_actionQueryMusicBrainzTrack->setToolTip( i18n("Try to determine meta information over the Internet") );
    m_actionProperties = createAction( m_view, i18n("Properties"), "document-properties", 0, this, SLOT(slotTrackProperties()),
                                       actionCollection, "track_properties" );
    m_actionRemove = createAction( m_trackView, i18n("Remove"), "edit-delete", Qt::Key_Delete, this, SLOT(slotRemove()),
                                   actionCollection, "track_remove" );
    m_actionRemove->setShortcutContext( Qt::WidgetShortcut );
    m_conversionAction = createAction( m_view, i18n("Convert Tracks"), "edit-redo", 0, this, SLOT(slotAudioConversion()),
                                       actionCollection, "project_audio_convert" );

    connect( m_columnAdjuster, SIGNAL(columnsNeedAjusting()),
             this, SLOT(slotAdjustColumns()) );
    connect( m_trackView->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
             this, SLOT(slotSelectionChanged()) );
#ifdef ENABLE_AUDIO_PLAYER
    connect( m_player, SIGNAL(playingTrack(K3b::AudioTrack)),
             this, SLOT(slotPlayingTrack(K3b::AudioTrack)) );
    connect( m_player, SIGNAL(stateChanged()),
             this, SLOT(slotPlayerStateChanged()) );
#endif // ENABLE_AUDIO_PLAYER

    // Create audio context menu
    QAction* separator = new QAction( this );
    separator->setSeparator( true );
    m_trackView->setContextMenuPolicy( Qt::ActionsContextMenu );
    m_trackView->addAction( m_actionPlayTrack );
    m_trackView->addAction( separator );
    m_trackView->addAction( m_actionRemove );
    m_trackView->addAction( m_actionAddSilence );
    m_trackView->addAction( separator );
    m_trackView->addAction( m_actionSplitSource );
    m_trackView->addAction( m_actionSplitTrack );
    m_trackView->addAction( m_actionEditSource );
    m_trackView->addAction( m_actionMergeTracks );
    m_trackView->addAction( m_actionProperties );
    m_trackView->addAction( separator );
    m_trackView->addAction( m_actionQueryMusicBrainzTrack );
    m_trackView->addAction( separator );
    m_trackView->addAction( actionCollection->action("project_burn") );

#ifndef ENABLE_MUSICBRAINZ
    m_actionQueryMusicBrainz->setEnabled( false );
    m_actionQueryMusicBrainz->setVisible( false );
    m_actionQueryMusicBrainzTrack->setEnabled( false );
    m_actionQueryMusicBrainzTrack->setVisible( false );
#endif
}


void K3b::AudioViewImpl::addUrls( const QList<QUrl>& urls )
{
    AudioTrackAddingDialog::addUrls( urls, m_doc, 0, 0, 0, m_view );
}


void K3b::AudioViewImpl::slotRemove()
{
    const QModelIndexList indexes = m_trackView->selectionModel()->selectedRows();
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


void K3b::AudioViewImpl::slotAddSilence()
{
    const QModelIndexList indexes = m_trackView->selectionModel()->selectedRows();
    QList<AudioTrack*> tracks;
    QList<AudioDataSource*> sources;
    tracksForIndexes( tracks, indexes );
    sourcesForIndexes( sources, indexes );

    if( !sources.isEmpty() || !tracks.isEmpty() ) {
        //
        // create a simple dialog for asking the length of the silence
        //
        QDialog dlg( m_view );
        dlg.setWindowTitle(i18n("Add Silence"));

        QHBoxLayout* dlgLayout = new QHBoxLayout( &dlg );
        dlgLayout->setContentsMargins( 0, 0, 0, 0 );
        QLabel* label = new QLabel( i18n("Length of silence:"), &dlg );
        MsfEdit* msfEdit = new MsfEdit( &dlg );
        msfEdit->setValue( 150 ); // 2 seconds default
        msfEdit->setFocus();

        QDialogButtonBox* buttonBox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg );
        connect( buttonBox, SIGNAL(accepted()), &dlg, SLOT(accept()) );
        connect( buttonBox, SIGNAL(rejected()), &dlg, SLOT(reject()) );

        dlgLayout->addWidget( label );
        dlgLayout->addWidget( msfEdit );
        dlgLayout->addWidget( buttonBox );

        if( dlg.exec() == QDialog::Accepted ) {
            AudioZeroData* zero = new AudioZeroData( msfEdit->value() );
            if ( !tracks.isEmpty() )
                tracks.first()->addSource( zero );
            else
                zero->moveAfter( sources.first() );
        }
    }
}


void K3b::AudioViewImpl::slotMergeTracks()
{
    const QModelIndexList indexes = m_trackView->selectionModel()->selectedRows();
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


void K3b::AudioViewImpl::slotSplitSource()
{
    const QModelIndexList indexes = m_trackView->selectionModel()->selectedRows();
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


void K3b::AudioViewImpl::slotSplitTrack()
{
    const QModelIndexList indexes = m_trackView->selectionModel()->selectedRows();
    QList<AudioTrack*> tracks;
    tracksForIndexes( tracks, indexes );

    if( !tracks.isEmpty() ) {
        AudioTrackSplitDialog::splitTrack( tracks.first(), m_view );
    }
}


void K3b::AudioViewImpl::slotEditSource()
{
    const QModelIndexList indexes = m_trackView->selectionModel()->selectedRows();
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
        QDialog dlg( m_view );
        dlg.setWindowTitle( i18n("Edit Audio Track Source") );

        AudioDataSourceEditWidget* editW = new AudioDataSourceEditWidget( &dlg );
        editW->loadSource( source );

        QDialogButtonBox* buttonBox = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg );
        connect( buttonBox, SIGNAL(accepted()), &dlg, SLOT(accept()) );
        connect( buttonBox, SIGNAL(rejected()), &dlg, SLOT(reject()) );

        QVBoxLayout* layout = new QVBoxLayout( &dlg );
        layout->addWidget( editW );
        layout->addWidget( buttonBox );

        if( dlg.exec() == QDialog::Accepted ) {
            editW->saveSource();
        }
    }
}


void K3b::AudioViewImpl::slotTrackProperties()
{
    const QModelIndexList indexes = m_trackView->selectionModel()->selectedRows();
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


void K3b::AudioViewImpl::slotPlayTrack()
{
    const QModelIndexList indexes = m_trackView->selectionModel()->selectedRows();
	QList<AudioTrack*> tracks;
    tracksForIndexes( tracks, indexes );

#ifdef ENABLE_AUDIO_PLAYER
    if( !tracks.empty() && tracks.first() != 0 )
        m_player->playTrack( *tracks.first() );
#endif // ENABLE_AUDIO_PLAYER
}


void K3b::AudioViewImpl::slotQueryMusicBrainz()
{
#ifdef ENABLE_MUSICBRAINZ
    QList<AudioTrack*> tracks;
    for( int i = 1; i <= m_doc->numOfTracks(); ++i ) {
        tracks.push_back( m_doc->getTrack( i ) );
    }

    if( tracks.isEmpty() ) {
        KMessageBox::sorry( m_view, i18n("Please add an audio track.") );
        return;
    }

    // only one may use the tracks at the same time
//     if( m_currentlyPlayingTrack &&
//         tracks.containsRef( m_currentlyPlayingTrack ) )
//         m_player->stop();

    // now do the lookup on the files.
    AudioTrackTRMLookupDialog dlg( m_view );
    dlg.lookup( tracks );
#endif
}


void K3b::AudioViewImpl::slotQueryMusicBrainzTrack()
{
#ifdef ENABLE_MUSICBRAINZ
    const QModelIndexList indexes = m_trackView->selectionModel()->selectedRows();
    QList<AudioTrack*> tracks;
    tracksForIndexes( tracks, indexes );

    if( tracks.isEmpty() ) {
        KMessageBox::sorry( m_view, i18n("Please select an audio track.") );
        return;
    }

    // only one may use the tracks at the same time
//     if( m_currentlyPlayingTrack &&
//         tracks.containsRef( m_currentlyPlayingTrack ) )
//         m_player->stop();

    // now do the lookup on the files.
    AudioTrackTRMLookupDialog dlg( m_view );
    dlg.lookup( tracks );
#endif
}


void K3b::AudioViewImpl::slotSelectionChanged()
{
    if( m_trackView->hasFocus() ) {
        QModelIndexList indexes = m_trackView->selectionModel()->selectedRows();

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
}


void K3b::AudioViewImpl::slotAudioConversion()
{
    if( m_doc->numOfTracks() > 0 ) {
        AudioProjectConvertingDialog dlg( m_doc, m_view );
        dlg.exec();
    }
    else {
        KMessageBox::information( m_view, i18n("Please add files to your project first."),
                                  i18n("No tracks to convert") );
    }
}


void K3b::AudioViewImpl::slotAdjustColumns()
{
    qDebug();

    if( m_updatingColumnWidths ) {
        qDebug() << "already updating column widths.";
        return;
    }

    m_updatingColumnWidths = true;

    // now properly resize the columns
    // minimal width for type, length, pregap
    // fixed for filename
    // expand for cd-text
    int titleWidth = m_columnAdjuster->columnSizeHint( AudioProjectModel::TitleColumn );
    int artistWidth = m_columnAdjuster->columnSizeHint( AudioProjectModel::ArtistColumn );
    int typeWidth = m_columnAdjuster->columnSizeHint( AudioProjectModel::TypeColumn );
    int lengthWidth = m_columnAdjuster->columnSizeHint( AudioProjectModel::LengthColumn );
    int filenameWidth = m_columnAdjuster->columnSizeHint( AudioProjectModel::FilenameColumn );

    // add a margin
    typeWidth += 10;
    lengthWidth += 10;

    // these always need to be completely visible
    m_trackView->setColumnWidth( AudioProjectModel::TrackNumberColumn, m_columnAdjuster->columnSizeHint( AudioProjectModel::TrackNumberColumn ) );
    m_trackView->setColumnWidth( AudioProjectModel::TypeColumn, typeWidth );
    m_trackView->setColumnWidth( AudioProjectModel::LengthColumn, lengthWidth );

    int remaining = m_trackView->viewport()->contentsRect().width() - typeWidth - lengthWidth - m_trackView->columnWidth(0);

    // now let's see if there is enough space for all
    if( remaining >= artistWidth + titleWidth + filenameWidth ) {
        remaining -= filenameWidth;
        remaining -= (titleWidth + artistWidth);
        m_trackView->setColumnWidth( AudioProjectModel::ArtistColumn, artistWidth + remaining/2 );
        m_trackView->setColumnWidth( AudioProjectModel::TitleColumn, titleWidth + remaining/2 );
        m_trackView->setColumnWidth( AudioProjectModel::FilenameColumn, filenameWidth );
    }
    else if( remaining >= artistWidth + titleWidth + 20 ) {
        m_trackView->setColumnWidth( AudioProjectModel::ArtistColumn, artistWidth );
        m_trackView->setColumnWidth( AudioProjectModel::TitleColumn, titleWidth );
        m_trackView->setColumnWidth( AudioProjectModel::FilenameColumn, remaining - artistWidth - titleWidth );
    }
    else {
        m_trackView->setColumnWidth( AudioProjectModel::ArtistColumn, remaining/3 );
        m_trackView->setColumnWidth( AudioProjectModel::TitleColumn, remaining/3 );
        m_trackView->setColumnWidth( AudioProjectModel::FilenameColumn, remaining/3 );
    }

    m_updatingColumnWidths = false;
}


void K3b::AudioViewImpl::slotPlayingTrack( const K3b::AudioTrack& track )
{
    QModelIndex index = m_model->indexForTrack( &track );
    m_trackView->scrollTo( index );
    m_delegate->setPlayingTrack( index );
}


void K3b::AudioViewImpl::slotPlayerStateChanged()
{
#ifdef ENABLE_AUDIO_PLAYER
    if( m_player->state() == AudioTrackPlayer::Stopped ) {
        m_delegate->setPlayingTrack( QModelIndex() );
    }
#endif // ENABLE_AUDIO_PLAYER
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


