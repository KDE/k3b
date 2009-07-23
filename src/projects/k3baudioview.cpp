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

#include "k3baudioview.h"

#include "k3bapplication.h"
#include "k3baudioburndialog.h"
#include "k3baudiodoc.h"
#include "k3baudioprojectmodel.h"
#include "k3baudioviewimpl.h"
#include "k3bfillstatusdisplay.h"
#include "k3bpluginmanager.h"
//#include "k3baudiotrackplayer.h"
#include "../rip/k3bviewcolumnadjuster.h"

#include <QLayout>
#include <QString>

#include <KAction>
#include <KActionCollection>
#include <KDebug>
#include <KLocale>
#include <KMessageBox>
#include <KToolBar>
#include <KMenu>


K3b::AudioView::AudioView( K3b::AudioDoc* doc, QWidget* parent )
    : K3b::StandardView( doc, parent ),
      m_updatingColumnWidths(false)
{
    m_doc = doc;
    m_model = new AudioProjectModel(m_doc, this);
    m_audioViewImpl = new AudioViewImpl( this, m_doc, m_model, actionCollection() );
    
    setAutoExpandDelay(200);
    // and hide the side panel as the audio project has no tree hierarchy
    setShowDirPanel(false);

    // trueg: I don't see why we use StandardView here. IMHO it only makes things a bit weird
    m_columnAdjuster = new ViewColumnAdjuster( fileView() );
    connect( m_columnAdjuster, SIGNAL( columnsNeedAjusting() ), this, SLOT( slotAdjustColumns() ) );
    connect( this, SIGNAL(activated(QModelIndex)),
             m_audioViewImpl, SLOT(slotItemActivated(QModelIndex)) );
    
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
             this, SLOT(slotTrackProperties()) );
    connect( actionCollection()->action( "track_remove" ), SIGNAL( triggered() ),
             this, SLOT(slotRemove()) );

    fillStatusDisplay()->showTime();
    
    // set the model for the StandardView's views
    setModel(m_model);

    toolBox()->addAction( actionCollection()->action( "project_audio_convert" ) );
    toolBox()->addSeparator();

#ifdef __GNUC__
#warning enable player once ported to Phonon
#endif
//     toolBox()->addAction( m_songlist->player()->action( AudioTrackPlayer::ACTION_PLAY ) );
//     toolBox()->addAction( m_songlist->player()->action( AudioTrackPlayer::ACTION_PAUSE ) );
//     toolBox()->addAction( m_songlist->player()->action( AudioTrackPlayer::ACTION_STOP ) );
//     toolBox()->addSpacing();
//     toolBox()->addAction( m_songlist->player()->action( AudioTrackPlayer::ACTION_PREV ) );
//     toolBox()->addAction( m_songlist->player()->action( AudioTrackPlayer::ACTION_NEXT ) );
//     toolBox()->addSpacing();
//     m_songlist->player()->action( AudioTrackPlayer::ACTION_SEEK )->plug( toolBox() );
//     toolBox()->addSeparator();

#if 0
#ifdef HAVE_MUSICBRAINZ
    kDebug() << "(AudioView) m_songlist->actionCollection()->actions().count() " << m_songlist->actionCollection()->actions().count();
    toolBox()->addAction( m_songlist->actionCollection()->action( "project_audio_musicbrainz" ) );
    toolBox()->addSeparator();
#endif
#endif

    addPluginButtons();

    // this is just for testing (or not?)
    // most likely every project type will have it's rc file in the future
    // we only add the additional actions since View already added the default actions
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


K3b::ProjectBurnDialog* K3b::AudioView::newBurnDialog( QWidget* parent )
{
    return new AudioBurnDialog( m_doc, parent );
}


void K3b::AudioView::selectionChanged( const QModelIndexList& indexes )
{
    m_audioViewImpl->slotSelectionChanged( indexes );
}

        
void K3b::AudioView::contextMenu( const QPoint& pos )
{
    m_audioViewImpl->popupMenu()->exec( pos );
}


void K3b::AudioView::addUrls( const KUrl::List& urls )
{
    m_audioViewImpl->addUrls( urls );
}

void K3b::AudioView::slotRemove()
{
    m_audioViewImpl->remove( currentSelection() );
}


void K3b::AudioView::slotAddSilence()
{
    m_audioViewImpl->addSilence( currentSelection() );
}


void K3b::AudioView::slotMergeTracks()
{
    m_audioViewImpl->mergeTracks( currentSelection() );
}


void K3b::AudioView::slotSplitSource()
{
    m_audioViewImpl->splitSource( currentSelection() );
}


void K3b::AudioView::slotSplitTrack()
{
    m_audioViewImpl->splitTrack( currentSelection() );
}


void K3b::AudioView::slotEditSource()
{
    m_audioViewImpl->editSource( currentSelection() );
}

void K3b::AudioView::slotQueryMusicBrainz()
{
    m_audioViewImpl->queryMusicBrainz( currentSelection() );
}


void K3b::AudioView::slotTrackProperties()
{
    m_audioViewImpl->properties( currentSelection() );
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
    int titleWidth = m_columnAdjuster->columnSizeHint( AudioProjectModel::TitleColumn );
    int artistWidth = m_columnAdjuster->columnSizeHint( AudioProjectModel::ArtistColumn );
    int typeWidth = m_columnAdjuster->columnSizeHint( AudioProjectModel::TypeColumn );
    int lengthWidth = m_columnAdjuster->columnSizeHint( AudioProjectModel::LengthColumn );
    int filenameWidth = m_columnAdjuster->columnSizeHint( AudioProjectModel::FilenameColumn );

    // add a margin
    typeWidth += 10;
    lengthWidth += 10;

    // these always need to be completely visible
    fileView()->setColumnWidth( AudioProjectModel::TrackNumberColumn, m_columnAdjuster->columnSizeHint( AudioProjectModel::TrackNumberColumn ) );
    fileView()->setColumnWidth( AudioProjectModel::TypeColumn, typeWidth );
    fileView()->setColumnWidth( AudioProjectModel::LengthColumn, lengthWidth );

    int remaining = fileView()->contentsRect().width() - typeWidth - lengthWidth - fileView()->columnWidth(0);

    // now let's see if there is enough space for all
    if( remaining >= artistWidth + titleWidth + filenameWidth ) {
        remaining -= filenameWidth;
        remaining -= (titleWidth + artistWidth);
        fileView()->setColumnWidth( AudioProjectModel::ArtistColumn, artistWidth + remaining/2 );
        fileView()->setColumnWidth( AudioProjectModel::TitleColumn, titleWidth + remaining/2 );
        fileView()->setColumnWidth( AudioProjectModel::FilenameColumn, filenameWidth );
    }
    else if( remaining >= artistWidth + titleWidth + 20 ) {
        fileView()->setColumnWidth( AudioProjectModel::ArtistColumn, artistWidth );
        fileView()->setColumnWidth( AudioProjectModel::TitleColumn, titleWidth );
        fileView()->setColumnWidth( AudioProjectModel::FilenameColumn, remaining - artistWidth - titleWidth );
    }
    else {
        fileView()->setColumnWidth( AudioProjectModel::ArtistColumn, remaining/3 );
        fileView()->setColumnWidth( AudioProjectModel::TitleColumn, remaining/3 );
        fileView()->setColumnWidth( AudioProjectModel::FilenameColumn, remaining/3 );
    }

    m_updatingColumnWidths = false;
}

#include "k3baudioview.moc"
