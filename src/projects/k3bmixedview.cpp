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
#include "k3bdataprojectmodel.h"
#include "k3baudioprojectmodel.h"
#include "k3baudiotrackaddingdialog.h"
#include "k3bdataurladdingdialog.h"

//#include <k3baudiotrackplayer.h>
#include <k3baudiodoc.h>
#include <k3bdatadoc.h>
#include <k3bfillstatusdisplay.h>
#include <k3bprojectplugin.h>
#include <k3bdiritem.h>

#include <kdialog.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <KToolBar>


K3b::MixedView::MixedView( K3b::MixedDoc* doc, QWidget* parent )
    : K3b::StandardView( doc, parent ), m_doc(doc)
{
    m_model = new K3b::MixedProjectModel(m_doc, this);

    connect( this, SIGNAL( currentRootChanged( const QModelIndex& ) ),
             m_model, SLOT( slotCurrentRootIndexChanged( const QModelIndex& ) ) );

    setModel(m_model);

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

#if 0
#ifdef HAVE_MUSICBRAINZ
    toolBox()->addAction( m_audioListView->actionCollection()->action( "project_audio_musicbrainz" ) );
    toolBox()->addSeparator();
#endif
#endif
    addPluginButtons( K3b::ProjectPlugin::MIXED_CD );
}


K3b::MixedView::~MixedView()
{
}


K3b::AudioTrackPlayer* K3b::MixedView::player() const
{
    //return m_audioListView->player();
    return 0;
}


void K3b::MixedView::slotAudioTreeSelected()
{
    //m_widgetStack->setCurrentWidget( m_audioListView );
}


void K3b::MixedView::slotDataTreeSelected()
{
    //m_widgetStack->setCurrentWidget( m_dataFileView );
}


K3b::DirItem* K3b::MixedView::currentDir() const
{
#if 0
    if( m_widgetStack->currentWidget() == m_dataFileView )
        return m_dataFileView->currentDir();
    else
        return 0;
#endif
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


K3b::ProjectBurnDialog* K3b::MixedView::newBurnDialog( QWidget* parent )
{
    return new K3b::MixedBurnDialog( m_doc, parent );
}


void K3b::MixedView::addUrls( const KUrl::List& urls )
{
    QAbstractItemModel *model = m_model->subModelForIndex( currentRoot() );

    if (!model)
        return;

    // use cast to determine which tree is currently selected
    K3b::DataProjectModel *dataModel = dynamic_cast<K3b::DataProjectModel*>(model);
    K3b::AudioProjectModel *audioModel = dynamic_cast<K3b::AudioProjectModel*>(model);

    if (dataModel) {
        K3b::DirItem *item = dynamic_cast<K3b::DirItem*>(dataModel->itemForIndex(m_model->mapToSubModel(currentRoot())));
        if (!item)
            item = m_doc->dataDoc()->root();

        K3b::DataUrlAddingDialog::addUrls( urls, item );
    }
    else if (audioModel) {
        K3b::AudioTrackAddingDialog::addUrls( urls, m_doc->audioDoc(), 0, 0, 0, this );
    }
}

#include "k3bmixedview.moc"
