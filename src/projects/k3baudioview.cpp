/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 *           (C) 2009      Arthur Mello <arthur@mandriva.com>
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
#include "k3baudiotrackview.h"
//#include "k3baudiotrackplayer.h"
#include "k3baudioburndialog.h"
#include "k3baudiotrackaddingdialog.h"
#include <k3bapplication.h>

#include <config-k3b.h>

#include <k3baudiodoc.h>
#include <k3baudiotrack.h>
#include <k3baudiofile.h>
#include <k3bpluginmanager.h>

// this is not here becasue of base_*.ui troubles
#include "../rip/k3baudioprojectconvertingdialog.h"

#include <k3bfillstatusdisplay.h>
#include <k3bmsf.h>
#include <k3bprojectplugin.h>

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


K3bAudioView::K3bAudioView( K3bAudioDoc* pDoc, QWidget* parent )
    : K3bStandardView( pDoc, parent )
{
    m_doc = pDoc;

	m_model = new K3b::AudioProjectModel(m_doc, this);
	// set the model for the K3bStandardView's views
	setModel(m_model);
	setShowDirPanel(false);

#if 0
    m_songlist = new K3bAudioTrackView( m_doc, this );
    setMainWidget( m_songlist );
#endif
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
//     toolBox()->addAction( m_songlist->player()->action( K3bAudioTrackPlayer::ACTION_PLAY ) );
//     toolBox()->addAction( m_songlist->player()->action( K3bAudioTrackPlayer::ACTION_PAUSE ) );
//     toolBox()->addAction( m_songlist->player()->action( K3bAudioTrackPlayer::ACTION_STOP ) );
//     toolBox()->addSpacing();
//     toolBox()->addAction( m_songlist->player()->action( K3bAudioTrackPlayer::ACTION_PREV ) );
//     toolBox()->addAction( m_songlist->player()->action( K3bAudioTrackPlayer::ACTION_NEXT ) );
//     toolBox()->addSpacing();
//     m_songlist->player()->action( K3bAudioTrackPlayer::ACTION_SEEK )->plug( toolBox() );
//     toolBox()->addSeparator();

#if 0
#ifdef HAVE_MUSICBRAINZ
    kDebug() << "(K3bAudioView) m_songlist->actionCollection()->actions().count() " << m_songlist->actionCollection()->actions().count();
    toolBox()->addAction( m_songlist->actionCollection()->action( "project_audio_musicbrainz" ) );
    toolBox()->addSeparator();
#endif
#endif

    addPluginButtons( K3bProjectPlugin::AUDIO_CD );

    // this is just for testing (or not?)
    // most likely every project type will have it's rc file in the future
    // we only add the additional actions since K3bView already added the default actions
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

K3bAudioView::~K3bAudioView()
{
}


void K3bAudioView::init()
{
    if( k3bcore->pluginManager()->plugins( "AudioDecoder" ).isEmpty() )
        KMessageBox::error( this, i18n("No audio decoder plugins found. You will not be able to add any files "
                                       "to the audio project!") );
}


K3bProjectBurnDialog* K3bAudioView::newBurnDialog( QWidget* parent )
{
    return new K3bAudioBurnDialog( m_doc, parent );
}


void K3bAudioView::slotAudioConversion()
{
    K3bAudioProjectConvertingDialog dlg( m_doc, this );
    dlg.exec();
}


void K3bAudioView::addUrls( const KUrl::List& urls )
{
    K3bAudioTrackAddingDialog::addUrls( urls, m_doc, 0, 0, 0, this );
}

#include "k3baudioview.moc"
