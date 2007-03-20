/*
 *
 * $Id$
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bvideodvdview.h"
#include "k3bvideodvddoc.h"
#include "k3bvideodvdburndialog.h"
#include "k3bdatadirtreeview.h"
#include "k3bdatafileview.h"
#include "k3bdataurladdingdialog.h"
#include <k3bfillstatusdisplay.h>
#include <k3bdatafileview.h>
#include <k3btoolbox.h>
#include <k3bprojectplugin.h>

#include <klocale.h>
#include <kmessagebox.h>
#include <kactioncollection.h>

#include <qsplitter.h>


K3bVideoDvdView::K3bVideoDvdView( K3bVideoDvdDoc* doc, QWidget *parent, const char *name )
  : K3bView( doc, parent, name ),
    m_doc(doc)
{
  fillStatusDisplay()->showDvdSizes(true);

  // --- setup GUI ---------------------------------------------------
  QSplitter* mainSplitter = new QSplitter( this );
  m_dataDirTree = new K3bDataDirTreeView( this, doc, mainSplitter );
  m_dataFileView = new K3bDataFileView( this, m_dataDirTree, doc, mainSplitter );
  m_dataDirTree->setFileView( m_dataFileView );
  setMainWidget( mainSplitter );

  connect( m_dataFileView, SIGNAL(dirSelected(K3bDirItem*)), m_dataDirTree, SLOT(setCurrentDir(K3bDirItem*)) );

  m_dataDirTree->checkForNewItems();
  m_dataFileView->checkForNewItems();

  addPluginButtons( K3bProjectPlugin::VIDEO_DVD );
}


K3bVideoDvdView::~K3bVideoDvdView()
{
}


K3bProjectBurnDialog* K3bVideoDvdView::newBurnDialog( QWidget* parent, const char* name )
{
  return new K3bVideoDvdBurnDialog( m_doc, parent, name, true );
}


void K3bVideoDvdView::init()
{
  KMessageBox::information( this,
			    i18n("Be aware that you need to provide the complete Video DVD filestructure. "
				 "K3b does not support video transcoding and preparation of video object "
				 "files yet. That means you need to already have the VTS_X_YY.VOB "
				 "and VTS_X_YY.IFO files."),
			    i18n("K3b Video DVD Restrictions"),
			    "video_dvd_restrictions" );
}


void K3bVideoDvdView::addUrls( const KURL::List& urls )
{
  K3bDataUrlAddingDialog::addUrls( urls, m_dataFileView->currentDir() );
}

#include "k3bvideodvdview.moc"
