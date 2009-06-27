/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 *           (C) 2009      Arthur Mello <arthur@mandriva.com>
 *           (C) 2009      Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
 *           (C) 2009      Michal Malek <michalm@jabster.pl>
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

#include "k3baudioprojectmodel.h"
#include "k3baudioview.h"
//#include "k3baudiotrackplayer.h"
#include "k3baudioburndialog.h"
#include "k3baudiotrackaddingdialog.h"
#include "k3baudiozerodata.h"
#include "k3baudiotracksplitdialog.h"
#include "k3baudiodatasourceeditwidget.h"
#include "k3baudiotrackdialog.h"
#include "../rip/k3bviewcolumnadjuster.h"

#include <config-k3b.h>

#include "k3bapplication.h"
#include "k3baudiodoc.h"
#include "k3baudiotrack.h"
#include "k3baudiofile.h"
#include "k3bpluginmanager.h"
#include "k3bmsfedit.h"
#include "k3baudiodecoder.h"

// this is not here becasue of base_*.ui troubles
#include "../rip/k3baudioprojectconvertingdialog.h"

#include "k3bfillstatusdisplay.h"
#include "k3bmsf.h"

// QT-includes
#include <qlayout.h>
#include <qstring.h>

// KDE-includes
#include <klocale.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <KToolBar>
#include <KAction>
#include <KActionCollection>
#include <KMenu>


K3b::AudioView::AudioView( K3b::AudioDoc* pDoc, QWidget* parent )
    : K3b::StandardView( pDoc, parent ),
      m_updatingColumnWidths(false)
{
    connect( this, SIGNAL(activated(QModelIndex)), SLOT(slotItemActivated(QModelIndex)) );
    
    m_doc = pDoc;

    m_model = new K3b::AudioProjectModel(m_doc, this);
    // set the model for the K3b::StandardView's views
    setModel(m_model);
    setAutoExpandDelay(200);

    // and hide the side panel as the audio project has no tree hierarchy
    setShowDirPanel(false);

    // trueg: I don't see why we use StandardView here. IMHO it only makes things a bit weird
    m_columnAdjuster = new K3b::ViewColumnAdjuster( fileView() );
    connect( m_columnAdjuster, SIGNAL( columnsNeedAjusting() ), this, SLOT( slotAdjustColumns() ) );

    fillStatusDisplay()->showTime();

    // add button for the audio conversion
    KAction* conversionAction = new KAction( this );
    conversionAction->setText( i18n("Convert Tracks") );
    conversionAction->setIcon( KIcon( "edit-redo" ) );
    conversionAction->setToolTip( i18n("Convert audio tracks to other audio formats." ) );
    connect( conversionAction, SIGNAL( triggered() ), this, SLOT(slotAudioConversion()) );
    actionCollection()->addAction( "project_audio_convert", conversionAction );

    toolBox()->addAction( conversionAction );
    toolBox()->addSeparator();

#ifdef __GNUC__
#warning enable player once ported to Phonon
#endif
//     toolBox()->addAction( m_songlist->player()->action( K3b::AudioTrackPlayer::ACTION_PLAY ) );
//     toolBox()->addAction( m_songlist->player()->action( K3b::AudioTrackPlayer::ACTION_PAUSE ) );
//     toolBox()->addAction( m_songlist->player()->action( K3b::AudioTrackPlayer::ACTION_STOP ) );
//     toolBox()->addSpacing();
//     toolBox()->addAction( m_songlist->player()->action( K3b::AudioTrackPlayer::ACTION_PREV ) );
//     toolBox()->addAction( m_songlist->player()->action( K3b::AudioTrackPlayer::ACTION_NEXT ) );
//     toolBox()->addSpacing();
//     m_songlist->player()->action( K3b::AudioTrackPlayer::ACTION_SEEK )->plug( toolBox() );
//     toolBox()->addSeparator();

#if 0
#ifdef HAVE_MUSICBRAINZ
    kDebug() << "(K3b::AudioView) m_songlist->actionCollection()->actions().count() " << m_songlist->actionCollection()->actions().count();
    toolBox()->addAction( m_songlist->actionCollection()->action( "project_audio_musicbrainz" ) );
    toolBox()->addSeparator();
#endif
#endif

    addPluginButtons();

    // this is just for testing (or not?)
    // most likely every project type will have it's rc file in the future
    // we only add the additional actions since K3b::View already added the default actions
    setXML( "<!DOCTYPE kpartgui SYSTEM \"kpartgui.dtd\">"
            "<kpartgui name=\"k3bproject\" version=\"1\">"
            "<MenuBar>"
            " <Menu name=\"project\"><text>&amp;Project</text>"
            "  <Action name=\"project_audio_convert\"/>"
#ifdef HAVE_MUSICBRAINZ
            "  <Action name=\"project_audio_musicbrainz\"/>"
#endif
            " </Menu>"
            "</MenuBar>"
            "</kpartgui>", true );

    // setup context menu actions
    setupActions();
}

K3b::AudioView::~AudioView()
{
}


void K3b::AudioView::init()
{
    if( k3bcore->pluginManager()->plugins( "AudioDecoder" ).isEmpty() )
        KMessageBox::error( this, i18n("No audio decoder plugins found. You will not be able to add any files "
                                       "to the audio project.") );
}

void K3b::AudioView::setupActions()
{
    m_actionProperties = new KAction( this );
    m_actionProperties->setText( i18n("Properties") );
    m_actionProperties->setIcon( KIcon( "document-properties" ) );
    connect( m_actionProperties, SIGNAL( triggered() ), this, SLOT( slotTrackProperties() ) );
    actionCollection()->addAction( "track_properties", m_actionProperties );

    m_actionRemove = new KAction( this );
    m_actionRemove->setText( i18n("Remove") );
    m_actionRemove->setIcon( KIcon( "edit-delete" ) );
    connect( m_actionRemove, SIGNAL( triggered() ), this, SLOT( slotRemove() ) );
    actionCollection()->addAction( "track_remove", m_actionRemove );

    m_actionAddSilence = new KAction( this );
    m_actionAddSilence->setText( i18n("Add Silence...") );
    connect( m_actionAddSilence, SIGNAL( triggered() ), this, SLOT( slotAddSilence() ) );
    actionCollection()->addAction( "track_add_silence", m_actionAddSilence );

    m_actionMergeTracks = new KAction( this );
    m_actionMergeTracks->setText( i18n("Merge Tracks") );
    connect( m_actionMergeTracks, SIGNAL( triggered() ), this, SLOT( slotMergeTracks() ) );
    actionCollection()->addAction( "track_merge", m_actionMergeTracks );

    m_actionSplitSource = new KAction( this );
    m_actionSplitSource->setText( i18n("Source to Track") );
    connect( m_actionSplitSource, SIGNAL( triggered() ), this, SLOT( slotSplitSource() ) );
    actionCollection()->addAction( "source_split", m_actionSplitSource );

    m_actionSplitTrack = new KAction( this );
    m_actionSplitTrack->setText( i18n("Split Track...") );
    connect( m_actionSplitTrack, SIGNAL( triggered() ), this, SLOT( slotSplitTrack() ) );
    actionCollection()->addAction( "track_split", m_actionSplitTrack );

    m_actionEditSource = new KAction( this );
    m_actionEditSource->setText( i18n("Edit Source...") );
    connect( m_actionEditSource, SIGNAL( triggered() ), this, SLOT( slotEditSource() ) );
    actionCollection()->addAction( "track_split", m_actionEditSource );

    m_actionPlayTrack = 0;//new KAction( i18n("Play Track"), "media-playback-start",
//                                      KShortcut(), this, SLOT(slotPlayTrack()),
//                                      actionCollection(), "track_play" );
#ifdef HAVE_MUSICBRAINZ
    KAction* mbAction = new KAction( this );
    mbAction->setText( i18n("Musicbrainz Lookup") );
    mbAction->setIcon( KIcon( "musicbrainz" ) );
    connect( mbAction, SIGNAL( triggered() ), this, SLOT( slotQueryMusicBrainz() ) );
    actionCollection()->addAction( "project_audio_musicbrainz", mbAction );
    mbAction->setToolTip( i18n("Try to determine meta information over the Internet") );
#endif
    
    m_popupMenu = new KMenu( this );
    if( m_actionPlayTrack != 0 ) {
        m_popupMenu->addAction( m_actionPlayTrack );
        m_popupMenu->addSeparator();
    }
    m_popupMenu->addAction( m_actionRemove );
    m_popupMenu->addAction( m_actionAddSilence );
    m_popupMenu->addSeparator();
    m_popupMenu->addAction( m_actionSplitSource );
    m_popupMenu->addAction( m_actionSplitTrack );
    m_popupMenu->addAction( m_actionEditSource );
    m_popupMenu->addAction( m_actionMergeTracks );
    m_popupMenu->addAction( m_actionProperties );
    m_popupMenu->addSeparator();
    m_popupMenu->addAction( actionCollection()->action("project_burn") );
}


void K3b::AudioView::trackProperties( const QModelIndexList& indexes )
{
    QList<K3b::AudioTrack*> tracks;
    QList<K3b::AudioDataSource*> sources;
    getSelectedItems( tracks, sources, indexes );

    // TODO: add tracks from sources to tracks

    if( !tracks.isEmpty() ) {
        K3b::AudioTrackDialog d( tracks, this );
        d.exec();
    }
    else {
        slotProperties();
    }
}


void K3b::AudioView::getSelectedItems( QList<K3b::AudioTrack*>& tracks,
                                          QList<K3b::AudioDataSource*>& sources )
{
    getSelectedItems( tracks, sources, currentSelection() );
}



K3b::ProjectBurnDialog* K3b::AudioView::newBurnDialog( QWidget* parent )
{
    return new K3b::AudioBurnDialog( m_doc, parent );
}


void K3b::AudioView::getSelectedItems( QList<AudioTrack*>& tracks,
                                       QList<AudioDataSource*>& sources,
                                       const QModelIndexList& indexes )
{
    tracks.clear();
    sources.clear();

    foreach( const QModelIndex& index, indexes ) {
        if ( K3b::AudioTrack* track = m_model->trackForIndex( index ) ) {
            tracks << track;
        }
        else if ( K3b::AudioDataSource* source = m_model->sourceForIndex( index ) ) {
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


void K3b::AudioView::selectionChanged( const QModelIndexList& indexes )
{
    QList<K3b::AudioTrack*> tracks;
    QList<K3b::AudioDataSource*> sources;
    getSelectedItems( tracks, sources, indexes );

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

        
void K3b::AudioView::contextMenu( const QPoint& pos )
{
    m_popupMenu->exec( pos );
}


void K3b::AudioView::slotItemActivated( const QModelIndex& index )
{
    trackProperties( QModelIndexList() << index );
}


void K3b::AudioView::slotAudioConversion()
{
    K3b::AudioProjectConvertingDialog dlg( m_doc, this );
    dlg.exec();
}


void K3b::AudioView::addUrls( const KUrl::List& urls )
{
    K3b::AudioTrackAddingDialog::addUrls( urls, m_doc, 0, 0, 0, this );
}

void K3b::AudioView::slotRemove()
{
    QList<K3b::AudioTrack*> tracks;
    QList<K3b::AudioDataSource*> sources;
    getSelectedItems( tracks, sources );

    //
    // remove all sources which belong to one of the selected tracks since they will be
    // deleted along with their tracks
    //
    QList<K3b::AudioDataSource*>::iterator srcIt = sources.begin();
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


void K3b::AudioView::slotAddSilence()
{
    QList<K3b::AudioTrack*> tracks;
    QList<K3b::AudioDataSource*> sources;
    getSelectedItems( tracks, sources );

    if( !sources.isEmpty() || !tracks.isEmpty() ) {
        //
        // create a simple dialog for asking the length of the silence
        //
        KDialog dlg( this);
        QWidget* widget = dlg.mainWidget();
        dlg.setButtons(KDialog::Ok|KDialog::Cancel);
        dlg.setDefaultButton(KDialog::Ok);
        dlg.setCaption(i18n("Add Silence"));

        QHBoxLayout* dlgLayout = new QHBoxLayout( widget );
        dlgLayout->setMargin( 0 );
        QLabel* label = new QLabel( i18n("Length of silence:"), widget );
        K3b::MsfEdit* msfEdit = new K3b::MsfEdit( widget );
        msfEdit->setValue( 150 ); // 2 seconds default
        msfEdit->setFocus();
        dlgLayout->addWidget( label );
        dlgLayout->addWidget( msfEdit );

        if( dlg.exec() == QDialog::Accepted ) {
            K3b::AudioZeroData* zero = new K3b::AudioZeroData( msfEdit->value() );
            if ( !tracks.isEmpty() )
                tracks.first()->addSource( zero );
            else
                zero->moveAfter( sources.first() );
        }
    }
}


void K3b::AudioView::slotMergeTracks()
{
    QList<K3b::AudioTrack*> tracks;
    QList<K3b::AudioDataSource*> sources;
    getSelectedItems( tracks, sources );

    // we simply merge the selected tracks ignoring any eventually selected sources
    if ( !tracks.isEmpty() ) {
        K3b::AudioTrack* firstTrack = tracks.takeFirst();
        while( !tracks.isEmpty() ) {
            firstTrack->merge( tracks.takeFirst(), firstTrack->lastSource() );
        }
    }
}


void K3b::AudioView::slotSplitSource()
{
    QList<K3b::AudioTrack*> tracks;
    QList<K3b::AudioDataSource*> sources;
    getSelectedItems( tracks, sources );

    if( !sources.isEmpty() ) {
        K3b::AudioDataSource* source = sources.first();
        // create a new track
        K3b::AudioTrack* track = new K3b::AudioTrack( m_doc );
        K3b::AudioTrack* trackAfter = source->track();
        if( trackAfter->numberSources() == 1 )
            trackAfter = trackAfter->prev();
        track->addSource( source->take() );
        track->moveAfter( trackAfter );

        // let's see if it's a file because in that case we can reuse the metainfo :)
        // TODO: maybe add meta data to sources
        if( K3b::AudioFile* file = dynamic_cast<K3b::AudioFile*>( track->firstSource() ) ) {
            track->setArtist( file->decoder()->metaInfo( K3b::AudioDecoder::META_ARTIST ) );
            track->setTitle( file->decoder()->metaInfo( K3b::AudioDecoder::META_TITLE ) );
            track->setSongwriter( file->decoder()->metaInfo( K3b::AudioDecoder::META_SONGWRITER ) );
            track->setComposer( file->decoder()->metaInfo( K3b::AudioDecoder::META_COMPOSER ) );
            track->setCdTextMessage( file->decoder()->metaInfo( K3b::AudioDecoder::META_COMMENT ) );
        }
    }
}


void K3b::AudioView::slotSplitTrack()
{
    QList<K3b::AudioTrack*> tracks;
    QList<K3b::AudioDataSource*> sources;
    getSelectedItems( tracks, sources );

    if( !tracks.isEmpty() ) {
        K3b::AudioTrackSplitDialog::splitTrack( tracks.first(), this );
    }
}


void K3b::AudioView::slotEditSource()
{
    QList<K3b::AudioTrack*> tracks;
    QList<K3b::AudioDataSource*> sources;
    getSelectedItems( tracks, sources );

    K3b::AudioDataSource* source = 0;
    if( !sources.isEmpty() && tracks.isEmpty() )
        source = sources.first();
    else if( !tracks.isEmpty() && sources.isEmpty() )
        source = tracks.first()->firstSource();

    if( source ) {
        KDialog dlg( this );
        dlg.setCaption( i18n("Edit Audio Track Source") );
        dlg.setButtons( KDialog::Ok|KDialog::Cancel );
        dlg.setDefaultButton( KDialog::Ok );

        K3b::AudioDataSourceEditWidget* editW = new K3b::AudioDataSourceEditWidget( &dlg );
        dlg.setMainWidget( editW );
        editW->loadSource( source );
        if( dlg.exec() == QDialog::Accepted ) {
            editW->saveSource();
        }
    }
}

void K3b::AudioView::slotQueryMusicBrainz()
{
#ifdef HAVE_MUSICBRAINZ
    QList<K3b::AudioTrack*> tracks;
    QList<K3b::AudioDataSource*> sources;
    getSelectedItems( tracks, sources );

    if( tracks.isEmpty() ) {
        KMessageBox::sorry( this, i18n("Please select an audio track.") );
        return;
    }

    // only one may use the tracks at the same time
//     if( m_currentlyPlayingTrack &&
//         tracks.containsRef( m_currentlyPlayingTrack ) )
//         m_player->stop();

    // now do the lookup on the files.
    K3b::AudioTrackTRMLookupDialog dlg( this );
    dlg.lookup( tracks );
#endif
}


void K3b::AudioView::slotTrackProperties()
{
    trackProperties( currentSelection() );
}

void K3b::AudioView::slotAdjustColumns()
{
    kDebug();

    if( m_updatingColumnWidths ) {
        kDebug() << "already updating column widths.";
        return;
    }

    m_updatingColumnWidths = true;

    // now properly resize the columns
    // minimal width for type, length, pregap
    // fixed for filename
    // expand for cd-text
    int titleWidth = m_columnAdjuster->columnSizeHint( K3b::AudioProjectModel::TitleColumn );
    int artistWidth = m_columnAdjuster->columnSizeHint( K3b::AudioProjectModel::ArtistColumn );
    int typeWidth = m_columnAdjuster->columnSizeHint( K3b::AudioProjectModel::TypeColumn );
    int lengthWidth = m_columnAdjuster->columnSizeHint( K3b::AudioProjectModel::LengthColumn );
    int filenameWidth = m_columnAdjuster->columnSizeHint( K3b::AudioProjectModel::FilenameColumn );

    // add a margin
    typeWidth += 10;
    lengthWidth += 10;

    // these always need to be completely visible
    fileView()->setColumnWidth( K3b::AudioProjectModel::TrackNumberColumn, m_columnAdjuster->columnSizeHint( K3b::AudioProjectModel::TrackNumberColumn ) );
    fileView()->setColumnWidth( K3b::AudioProjectModel::TypeColumn, typeWidth );
    fileView()->setColumnWidth( K3b::AudioProjectModel::LengthColumn, lengthWidth );

    int remaining = fileView()->contentsRect().width() - typeWidth - lengthWidth - fileView()->columnWidth(0);

    // now let's see if there is enough space for all
    if( remaining >= artistWidth + titleWidth + filenameWidth ) {
        remaining -= filenameWidth;
        remaining -= (titleWidth + artistWidth);
        fileView()->setColumnWidth( K3b::AudioProjectModel::ArtistColumn, artistWidth + remaining/2 );
        fileView()->setColumnWidth( K3b::AudioProjectModel::TitleColumn, titleWidth + remaining/2 );
        fileView()->setColumnWidth( K3b::AudioProjectModel::FilenameColumn, filenameWidth );
    }
    else if( remaining >= artistWidth + titleWidth + 20 ) {
        fileView()->setColumnWidth( K3b::AudioProjectModel::ArtistColumn, artistWidth );
        fileView()->setColumnWidth( K3b::AudioProjectModel::TitleColumn, titleWidth );
        fileView()->setColumnWidth( K3b::AudioProjectModel::FilenameColumn, remaining - artistWidth - titleWidth );
    }
    else {
        fileView()->setColumnWidth( K3b::AudioProjectModel::ArtistColumn, remaining/3 );
        fileView()->setColumnWidth( K3b::AudioProjectModel::TitleColumn, remaining/3 );
        fileView()->setColumnWidth( K3b::AudioProjectModel::FilenameColumn, remaining/3 );
    }

    m_updatingColumnWidths = false;
}

#include "k3baudioview.moc"
