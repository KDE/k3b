/*
 *
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bmixedview.h"

#include "k3bmixeddoc.h"
#include "k3bmixedburndialog.h"
#include "k3bmixeddirtreeview.h"
#include "k3baudiotrackaddingdialog.h"
#include "k3bdataurladdingdialog.h"

//#include <k3baudiotrackplayer.h>
#include <k3baudiodoc.h>
#include <k3bdataviewitem.h>
#include <k3bdatafileview.h>
#include <k3bdatadoc.h>
#include <k3baudiotrackview.h>
#include <k3bfillstatusdisplay.h>
#include <k3bprojectplugin.h>

#include <q3widgetstack.h>
#include <qsplitter.h>
#include <qlayout.h>
#include <q3valuelist.h>

#include <kdialog.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <KToolBar>


K3bMixedView::K3bMixedView( K3bMixedDoc* doc, QWidget* parent )
  : K3bView( doc, parent ), m_doc(doc)
{
  QSplitter* splitter = new QSplitter( this );
  m_mixedDirTreeView = new K3bMixedDirTreeView( this, doc, splitter );
  m_widgetStack = new Q3WidgetStack( splitter );
  m_dataFileView = new K3bDataFileView( this, m_mixedDirTreeView, doc->dataDoc(), m_widgetStack );
  m_mixedDirTreeView->setFileView( m_dataFileView );
  m_audioListView = new K3bAudioTrackView( doc->audioDoc(), m_widgetStack );

  setMainWidget( splitter );

  connect( m_mixedDirTreeView, SIGNAL(audioTreeSelected()),
	   this, SLOT(slotAudioTreeSelected()) );
  connect( m_mixedDirTreeView, SIGNAL(dataTreeSelected()),
	   this, SLOT(slotDataTreeSelected()) );

  m_widgetStack->raiseWidget( m_dataFileView );

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

#ifdef HAVE_MUSICBRAINZ
  toolBox()->addAction( m_audioListView->actionCollection()->action( "project_audio_musicbrainz" ) );
  toolBox()->addSeparator();
#endif

  addPluginButtons( K3bProjectPlugin::MIXED_CD );

  m_mixedDirTreeView->checkForNewItems();
  m_dataFileView->checkForNewItems();
}


K3bMixedView::~K3bMixedView()
{
}


K3bAudioTrackPlayer* K3bMixedView::player() const
{
  return m_audioListView->player();
}


void K3bMixedView::slotAudioTreeSelected()
{
  m_widgetStack->raiseWidget( m_audioListView );
}


void K3bMixedView::slotDataTreeSelected()
{
  m_widgetStack->raiseWidget( m_dataFileView );
}


K3bDirItem* K3bMixedView::currentDir() const
{
  if( m_widgetStack->visibleWidget() == m_dataFileView )
    return m_dataFileView->currentDir();
  else
    return 0;
}


void K3bMixedView::slotBurn()
{
  if( m_doc->audioDoc()->numOfTracks() == 0 || m_doc->dataDoc()->size() == 0 ) {
    KMessageBox::information( this, i18n("Please add files and audio titles to your project first."),
			      i18n("No Data to Burn"), QString::null, false );
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
  if( m_widgetStack->visibleWidget() == m_dataFileView )
    K3bDataUrlAddingDialog::addUrls( urls, currentDir() );
  else
    K3bAudioTrackAddingDialog::addUrls( urls, m_doc->audioDoc(), 0, 0, 0, this );
}

#include "k3bmixedview.moc"
