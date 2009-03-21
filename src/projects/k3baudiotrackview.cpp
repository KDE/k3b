/*
 *
 * Copyright (C) 2004-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3baudiotrackview.h"
#include "k3baudiotrack.h"
#include "k3baudiodatasource.h"
#include "k3baudiotrackdialog.h"
#include "k3baudiodoc.h"
#include "k3baudiozerodata.h"
#include "k3baudiotracksplitdialog.h"
#include "k3baudiofile.h"
//#include "k3baudiotrackplayer.h"
//#include "k3baudiocdtrackdrag.h"
#include "k3baudiocdtracksource.h"
#include "k3baudiotracktrmlookupdialog.h"
#include "k3baudiodatasourceeditwidget.h"
#include "k3baudiotrackaddingdialog.h"
#include "k3baudioprojectmodel.h"
#include "../rip/k3bviewcolumnadjuster.h"

#include <config-k3b.h>

#include <k3bview.h>
#include <k3baudiodecoder.h>
#include <k3bmsfedit.h>

#include <qtimer.h>
#include <qpoint.h>
#include <qstringlist.h>
#include <qevent.h>
#include <qpixmap.h>
#include <qpainter.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qlist.h>
#include <QHBoxLayout>
#include <QDragLeaveEvent>
#include <QKeyEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QResizeEvent>
#include <QFocusEvent>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QtGui/QHeaderView>

#include <kurl.h>
#include <k3urldrag.h>
#include <klocale.h>
#include <kaction.h>
#include <kmenu.h>
#include <kiconloader.h>
#include <kapplication.h>
#include <kmessagebox.h>
#include <kdialog.h>
#include <K3URLDrag>
#include <kactioncollection.h>


K3b::AudioTrackView::AudioTrackView( K3b::AudioDoc* doc, QWidget* parent )
    : QTreeView( parent ),
      m_doc(doc),
      m_updatingColumnWidths(false),
      m_currentlyPlayingTrack(0)
{
    m_model = new K3b::AudioProjectModel( doc, this );
    setModel( m_model );

    m_columnAdjuster = new K3b::ViewColumnAdjuster( this );
    connect( m_columnAdjuster, SIGNAL( columnsNeedAjusting() ), this, SLOT( slotAdjustColumns() ) );

#ifdef __GNUC__
#warning Port the Audio player to Phonon
#endif
//    m_player = new K3b::AudioTrackPlayer( m_doc, this );
//     connect( m_player, SIGNAL(playingTrack(K3b::AudioTrack*)), this,
//              SLOT(showPlayerIndicator(K3b::AudioTrack*)) );
//     connect( m_player, SIGNAL(paused(bool)), this, SLOT(togglePauseIndicator(bool)) );
//     connect( m_player, SIGNAL(stopped()), this, SLOT(removePlayerIndicator()) );

//     setItemMargin( 5 );
    setDragDropMode( QAbstractItemView::DragDrop );
    setDropIndicatorShown( true );
    setDragEnabled(true);
    setAcceptDrops( true );
    setContextMenuPolicy( Qt::CustomContextMenu );
    setDropIndicatorShown( true );
    setItemsExpandable( true );
    setRootIsDecorated( false );

//     setDropVisualizer( true );
//     setAllColumnsShowFocus( true );
//     setDragEnabled( true );
    //  setSelectionModeExt( K3ListView::Konqueror ); // FileManager in KDE3
//     setSelectionModeExt( K3ListView::Extended );
//     setItemsMovable( false );
//    setAlternateBackground( QColor() ); // disable alternate colors

//     setNoItemText( i18n("Use drag'n'drop to add audio files to the project.") + "\n"
//                    + i18n("After that press the burn button to write the CD." ) );

//     setupColumns();
    setupActions();

//     m_playerItemAnimator = new K3b::ListViewItemAnimator( this );

//     m_animationTimer = new QTimer( this );
//     connect( m_animationTimer, SIGNAL(timeout()), this, SLOT(slotAnimation()) );

//     m_autoOpenTrackTimer = new QTimer( this );
//     connect( m_autoOpenTrackTimer, SIGNAL(timeout()), this, SLOT(slotDragTimeout()) );

//     connect( this, SIGNAL(dropped(QDropEvent*, Q3ListViewItem*, Q3ListViewItem*)),
//              this, SLOT(slotDropped(QDropEvent*, Q3ListViewItem*, Q3ListViewItem*)) );
    connect( this, SIGNAL(customContextMenuRequested(const QPoint&)),
             this, SLOT(showPopupMenu(const QPoint&)) );
//     connect( this, SIGNAL(doubleClicked(Q3ListViewItem*, const QPoint&, int)),
//              this, SLOT(slotProperties()) );

//     connect( doc, SIGNAL(changed()),
//              this, SLOT(slotChanged()) );
    connect( doc, SIGNAL(trackChanged(K3b::AudioTrack*)),
             this, SLOT(slotTrackChanged(K3b::AudioTrack*)) );
//     connect( doc, SIGNAL(trackRemoved(K3b::AudioTrack*)),
//              this, SLOT(slotTrackRemoved(K3b::AudioTrack*)) );

//    slotChanged();

    // a little background pix hack because I am simply incapable of doing it another way. :(
//   static QPixmap s_bgPix("/tmp/trueg/audio_bg.png");
//   setK3bBackgroundPixmap( s_bgPix, TOP_LEFT );
}


K3b::AudioTrackView::~AudioTrackView()
{
}


void K3b::AudioTrackView::setupColumns()
{
//     addColumn( i18n("No.") );
//     addColumn( i18n("Artist (CD-Text)") );
//     addColumn( i18n("Title (CD-Text)") );
//     addColumn( i18n("Type") );
//     addColumn( i18n("Length") );
//     addColumn( i18n("Filename") );

//     setColumnAlignment( 3, Qt::AlignHCenter );
//     setColumnAlignment( 4, Qt::AlignHCenter );

//     setColumnWidthMode( 1, Manual );
//     setColumnWidthMode( 2, Manual );
//     setColumnWidthMode( 3, Manual );
//     setColumnWidthMode( 4, Manual );
//     setColumnWidthMode( 5, Manual );

//     header()->setResizeEnabled( false );
//     header()->setClickEnabled( false );
//     setSorting( -1 );
}


void K3b::AudioTrackView::setupActions()
{
    m_actionCollection = new KActionCollection( this );

    m_actionProperties = new KAction( this );
    m_actionProperties->setText( i18n("Properties") );
    m_actionProperties->setIcon( KIcon( "document-properties" ) );
    connect( m_actionProperties, SIGNAL( triggered() ), this, SLOT( slotProperties() ) );
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
}


#if 0
#ifdef __GNUC__
#warning FIXME: K3b::AudioCdTrackDrag
#endif
//     else if( K3b::AudioCdTrackDrag::canDecode( e ) ) {
//         kDebug() << "(K3b::AudioTrackView) audiocdtrack dropped.";
//         K3b::Device::Toc toc;
//         K3b::Device::Device* dev = 0;
//         K3b::CddbResultEntry cddb;
//         QList<int> trackNumbers;

//         K3b::AudioCdTrackDrag::decode( e, toc, trackNumbers, cddb, &dev );

//         // for now we just create one source
//         for( QList<int>::const_iterator it = trackNumbers.begin();
//              it != trackNumbers.end(); ++it ) {
//             int trackNumber = *it;

//             K3b::AudioCdTrackSource* source = new K3b::AudioCdTrackSource( toc, trackNumber, cddb, dev );
//             if( m_dropTrackParent ) {
//                 source->moveAfter( m_dropSourceAfter );
//                 if( m_dropSourceAfter )
//                     m_dropSourceAfter = source;
//             }
//             else {
//                 K3b::AudioTrack* track = new K3b::AudioTrack();
//                 track->setPerformer( cddb.artists[trackNumber-1] );
//                 track->setTitle( cddb.titles[trackNumber-1] );
//                 track->addSource( source );
//                 if( m_dropTrackAfter )
//                     track->moveAfter( m_dropTrackAfter );
//                 else
//                     m_doc->addTrack( track, 0 );

//                 m_dropTrackAfter = track;
//             }
//         }
//     }
#endif


// void K3b::AudioTrackView::slotChanged()
// {
//     kDebug() << "(K3b::AudioTrackView::slotChanged)";
//     // we only need to add new items here. Everything else is done in the
//     // specific slots below
//     K3b::AudioTrack* track = m_doc->firstTrack();
//     bool newTracks = false;
//     while( track ) {
//         bool newTrack;
//         getTrackViewItem( track, &newTrack );
//         if( newTrack )
//             newTracks = true;
//         track = track->next();
//     }

//     if( newTracks ) {
//         m_animationTimer->start(200);
//         showAllSources();
//     }

//     header()->setVisible( m_doc->numOfTracks() > 0 );

//     kDebug() << "(K3b::AudioTrackView::slotChanged) finished";
// }


void K3b::AudioTrackView::slotTrackChanged( K3b::AudioTrack* track )
{
    kDebug() << "(K3b::AudioTrackView::slotTrackChanged( " << track << " )";

    //
    // There may be some tracks around that have not been added to the list yet
    // (and might never). We ignore them until they are in the list and then
    // we create the item in slotChanged
    //
    if( track->inList() ) {
        // FIXME: make it nice looking
        setExpanded( m_model->indexForTrack( track ), track->numberSources() > 1 );

    }
    kDebug() << "(K3b::AudioTrackView::slotTrackChanged( " << track << " ) finished";
}


void K3b::AudioTrackView::dragEnterEvent( QDragEnterEvent* event )
{
    QAbstractItemView::dragEnterEvent( event );
}


void K3b::AudioTrackView::dragLeaveEvent( QDragLeaveEvent* event )
{
    QAbstractItemView::dragLeaveEvent( event );
}


void K3b::AudioTrackView::dragMoveEvent( QDragMoveEvent* event )
{
    QAbstractItemView::dragMoveEvent( event );
}


// void K3b::AudioTrackView::slotTrackRemoved( K3b::AudioTrack* track )
// {
//     kDebug() << "(K3b::AudioTrackView::slotTrackRemoved( " << track << " )";
//     if ( m_playerItemAnimator->item() == m_trackItemMap[track] ) {
//         m_playerItemAnimator->stop();
//     }
//     delete m_trackItemMap[track];
//     m_trackItemMap.remove(track);
// }


// void K3b::AudioTrackView::showAllSources()
// {
//     // TODO: add an action to show all sources

//     Q3ListViewItem* item = firstChild();
//     while( item ) {
//         if( K3b::AudioTrackViewItem* tv = dynamic_cast<K3b::AudioTrackViewItem*>( item ) )
//             tv->showSources( tv->track()->numberSources() != 1 );
//         item = item->nextSibling();
//     }
// }


void K3b::AudioTrackView::slotAdjustColumns()
{
    kDebug();

    if( m_updatingColumnWidths ) {
        kDebug() << "(K3b::AudioTrackView) already updating column widths.";
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
    setColumnWidth( K3b::AudioProjectModel::TrackNumberColumn, m_columnAdjuster->columnSizeHint( K3b::AudioProjectModel::TrackNumberColumn ) );
    setColumnWidth( K3b::AudioProjectModel::TypeColumn, typeWidth );
    setColumnWidth( K3b::AudioProjectModel::LengthColumn, lengthWidth );

    int remaining = contentsRect().width() - typeWidth - lengthWidth - columnWidth(0);

    // now let's see if there is enough space for all
    if( remaining >= artistWidth + titleWidth + filenameWidth ) {
        remaining -= filenameWidth;
        remaining -= (titleWidth + artistWidth);
        setColumnWidth( K3b::AudioProjectModel::ArtistColumn, artistWidth + remaining/2 );
        setColumnWidth( K3b::AudioProjectModel::TitleColumn, titleWidth + remaining/2 );
        setColumnWidth( K3b::AudioProjectModel::FilenameColumn, filenameWidth );
    }
    else if( remaining >= artistWidth + titleWidth + 20 ) {
        setColumnWidth( K3b::AudioProjectModel::ArtistColumn, artistWidth );
        setColumnWidth( K3b::AudioProjectModel::TitleColumn, titleWidth );
        setColumnWidth( K3b::AudioProjectModel::FilenameColumn, remaining - artistWidth - titleWidth );
    }
    else {
        setColumnWidth( K3b::AudioProjectModel::ArtistColumn, remaining/3 );
        setColumnWidth( K3b::AudioProjectModel::TitleColumn, remaining/3 );
        setColumnWidth( K3b::AudioProjectModel::FilenameColumn, remaining/3 );
    }

    m_updatingColumnWidths = false;
}


// void K3b::AudioTrackView::slotAnimation()
// {
//     resizeColumns();
//     Q3ListViewItem* item = firstChild();

//     bool animate = false;

//     while( item ) {
//         K3b::AudioTrackViewItem* trackItem = dynamic_cast<K3b::AudioTrackViewItem*>(item);
//         if( trackItem->animate() )
//             animate = true;
//         else
//             trackItem->setEnabled( true ); // files analysed, cd-text loaded
//         item = item->nextSibling();
//     }

//     if( !animate ) {
//         m_animationTimer->stop();
//     }
// }


void K3b::AudioTrackView::getSelectedItems( QList<K3b::AudioTrack*>& tracks,
                                          QList<K3b::AudioDataSource*>& sources )
{
    tracks.clear();
    sources.clear();

    QModelIndexList indexes = selectionModel()->selectedRows();
    foreach( const QModelIndex& index, indexes ) {
        if ( K3b::AudioTrack* track = m_model->trackForIndex( index ) ) {
            tracks << track;
        }
        else if ( K3b::AudioDataSource* source = m_model->sourceForIndex( index ) ) {
#warning Do not select hidden sources once the hiding of sources works again
            // do not select hidden source items or unfinished source files
            if( source->isValid() && source->length() != 0 ) {
                sources << source;
            }
        }
    }
}


void K3b::AudioTrackView::slotRemove()
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
    foreach( K3b::AudioTrack* track, tracks )
        delete track;

    //
    // Now delete all the sources
    //
    foreach( K3b::AudioDataSource* source, sources )
        delete source;
}


void K3b::AudioTrackView::slotAddSilence()
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
        dlgLayout->setSpacing( KDialog::spacingHint() );
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


void K3b::AudioTrackView::slotMergeTracks()
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


void K3b::AudioTrackView::slotSplitSource()
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


void K3b::AudioTrackView::slotSplitTrack()
{
    QList<K3b::AudioTrack*> tracks;
    QList<K3b::AudioDataSource*> sources;
    getSelectedItems( tracks, sources );

    if( !tracks.isEmpty() ) {
        K3b::AudioTrackSplitDialog::splitTrack( tracks.first(), this );
    }
}


void K3b::AudioTrackView::slotEditSource()
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


void K3b::AudioTrackView::showPopupMenu( const QPoint& pos )
{
    QList<K3b::AudioTrack*> tracks;
    QList<K3b::AudioDataSource*> sources;
    getSelectedItems( tracks, sources );

    int numTracks = tracks.count();
    int numSources = sources.count();

    // build the menu
    KMenu popupMenu;

    if( m_actionPlayTrack && numTracks >= 1 ) {
        popupMenu.addAction( m_actionPlayTrack );
        popupMenu.addSeparator();
    }

    if( numTracks + numSources )
        popupMenu.addAction( m_actionRemove );

    if( numSources + numTracks == 1 )
        popupMenu.addAction( m_actionAddSilence );

    if( numSources == 1 && numTracks == 0 ) {
        popupMenu.addSeparator();
        popupMenu.addAction( m_actionSplitSource );
        popupMenu.addAction( m_actionEditSource );
    }
    else if( numTracks == 1 && numSources == 0 ) {
        popupMenu.addSeparator();

        if( tracks.first()->length().lba() > 60 )
            popupMenu.addAction( m_actionSplitTrack );

        popupMenu.addAction( m_actionEditSource );

    }
    else if( numTracks > 1 ) {
        popupMenu.addSeparator();
        popupMenu.addAction( m_actionMergeTracks );
    }

    popupMenu.addAction( m_actionProperties );
    popupMenu.addSeparator();
    popupMenu.addAction( static_cast<K3b::View*>(m_doc->view())->actionCollection()->action( "project_burn" ) );

    popupMenu.exec( mapToGlobal( pos ) );
}


void K3b::AudioTrackView::slotProperties()
{
    QList<K3b::AudioTrack*> tracks;
    QList<K3b::AudioDataSource*> sources;
    getSelectedItems( tracks, sources );

    // TODO: add tracks from sources to tracks

    if( !tracks.isEmpty() ) {
        K3b::AudioTrackDialog d( tracks, this );
        d.exec();
    }
    else {
        static_cast<K3b::View*>(m_doc->view())->slotProperties();
    }
}


void K3b::AudioTrackView::slotPlayTrack()
{
#if 0
    QList<K3b::AudioTrack*> tracks;
    QList<K3b::AudioDataSource*> sources;
    getSelectedItems( tracks, sources );
#ifdef __GNUC__
#warning FIXME: slotPlayTrack
#endif
//     if( tracks.count() > 0 )
//         m_player->playTrack( tracks.first() );
#endif
}


void K3b::AudioTrackView::showPlayerIndicator( K3b::AudioTrack* track )
{
#if 0
    removePlayerIndicator();
    m_currentlyPlayingTrack = track;
    K3b::AudioTrackViewItem* item = getTrackViewItem( track );
    item->setPixmap( 1, SmallIcon( "media-playback-start" ) );
    m_playerItemAnimator->setItem( item, 1 );
#endif
}


void K3b::AudioTrackView::togglePauseIndicator( bool b )
{
#if 0
    if( m_currentlyPlayingTrack ) {
        if( b )
            m_playerItemAnimator->setPixmap( SmallIcon( "media-playback-pause" ) );
        else
            m_playerItemAnimator->setPixmap( SmallIcon( "media-playback-start" ) );
    }
#endif
}


void K3b::AudioTrackView::removePlayerIndicator()
{
#if 0
    if( m_currentlyPlayingTrack )
        getTrackViewItem( m_currentlyPlayingTrack )->setPixmap( 1, QPixmap() );
    m_playerItemAnimator->stop();
    m_currentlyPlayingTrack = 0;
#endif
}


void K3b::AudioTrackView::slotQueryMusicBrainz()
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

#include "k3baudiotrackview.moc"
