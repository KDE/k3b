/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009      Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
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

#include "k3bmixeddoc.h"
#include "k3bmixedburndialog.h"
#include "k3bmixedprojectmodel.h"
#include "k3baudiotrackaddingdialog.h"
#include "k3bdataurladdingdialog.h"

//#include <k3baudiotrackplayer.h>
#include <k3baudiodoc.h>
#include <k3bdatadoc.h>
#include <k3bfillstatusdisplay.h>
#include <k3bprojectplugin.h>

#include <kdialog.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <KToolBar>


K3bMixedView::K3bMixedView( K3bMixedDoc* doc, QWidget* parent )
    : K3bStandardView( doc, parent ), m_doc(doc)
{
    m_model = new K3b::MixedProjectModel(m_doc, this);
    setModel(m_model);

#ifdef __GNUC__
#warning enable player once ported to Phonon
#endif
//   toolBox()->addAction( m_audioListView->player()->action( K3bAudioTrackPlayer::ACTION_PLAY ) );
//   toolBox()->addAction( m_audioListView->player()->action( K3bAudioTrackPlayer::ACTION_PAUSE ) );
//   toolBox()->addAction( m_audioListView->player()->action( K3bAudioTrackPlayer::ACTION_STOP ) );
//   toolBox()->addSpacing();
//   toolBox()->addAction( m_audioListView->player()->action( K3bAudioTrackPlayer::ACTION_PREV ) );
//   toolBox()->addAction( m_audioListView->player()->action( K3bAudioTrackPlayer::ACTION_NEXT ) );
//   toolBox()->addSpacing();
//   m_audioListView->player()->action( K3bAudioTrackPlayer::ACTION_SEEK )->plug( toolBox() );
//   toolBox()->addSeparator();

#if 0
#ifdef HAVE_MUSICBRAINZ
    toolBox()->addAction( m_audioListView->actionCollection()->action( "project_audio_musicbrainz" ) );
    toolBox()->addSeparator();
#endif
#endif
    addPluginButtons( K3bProjectPlugin::MIXED_CD );
}


K3bMixedView::~K3bMixedView()
{
}


K3bAudioTrackPlayer* K3bMixedView::player() const
{
    //return m_audioListView->player();
    return 0;
}


void K3bMixedView::slotAudioTreeSelected()
{
    //m_widgetStack->setCurrentWidget( m_audioListView );
}


void K3bMixedView::slotDataTreeSelected()
{
    //m_widgetStack->setCurrentWidget( m_dataFileView );
}


K3bDirItem* K3bMixedView::currentDir() const
{
#if 0
    if( m_widgetStack->currentWidget() == m_dataFileView )
        return m_dataFileView->currentDir();
    else
        return 0;
#endif
    return 0;
}


void K3bMixedView::slotBurn()
{
    if( m_doc->audioDoc()->numOfTracks() == 0 || m_doc->dataDoc()->size() == 0 ) {
        KMessageBox::information( this, i18n("Please add files and audio titles to your project first."),
                                  i18n("No Data to Burn"), QString(), false );
    }
    else {
        K3bProjectBurnDialog* dlg = newBurnDialog( this );
        if( dlg ) {
            dlg->execBurnDialog(true);
            delete dlg;
        }
        else {
            kDebug() << "(K3bDoc) Error: no burndialog available.";
        }
    }
}


K3bProjectBurnDialog* K3bMixedView::newBurnDialog( QWidget* parent )
{
    return new K3bMixedBurnDialog( m_doc, parent );
}


void K3bMixedView::addUrls( const KUrl::List& urls )
{
#if 0
    if( m_widgetStack->currentWidget() == m_dataFileView )
        K3bDataUrlAddingDialog::addUrls( urls, currentDir() );
    else
        K3bAudioTrackAddingDialog::addUrls( urls, m_doc->audioDoc(), 0, 0, 0, this );
#endif
}

#include "k3bmixedview.moc"
