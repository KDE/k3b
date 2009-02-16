/*
 *
 * Copyright (C) 2005-2007 Sebastian Trueg <trueg@k3b.org>
 *           (C) 2009      Arthur Renato Mello <arthur@mandriva.com>
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

#include "k3bvideodvdview.h"
#include "k3bvideodvddoc.h"
#include "k3bdataprojectmodel.h"
#include "k3bvideodvdburndialog.h"
//#include "k3bdatadirtreeview.h"
//#include "k3bdatafileview.h"
#include "k3bdataurladdingdialog.h"
#include <k3bfillstatusdisplay.h>
#include <k3bprojectplugin.h>

#include <klocale.h>
#include <kmessagebox.h>
#include <kactioncollection.h>

#include <qsplitter.h>


K3bVideoDvdView::K3bVideoDvdView( K3bVideoDvdDoc* doc, QWidget *parent )
    : K3bStandardView( doc, parent ),
      m_doc(doc)
{
	m_model = new K3b::DataProjectModel(m_doc, this);
	// set the model for the K3bStandardView's views
	setModel(m_model);

#if 0
    // --- setup GUI ---------------------------------------------------
    QSplitter* mainSplitter = new QSplitter( this );
    m_dataDirTree = new K3bDataDirTreeView( this, doc, mainSplitter );
    m_dataFileView = new K3bDataFileView( this, doc, mainSplitter );
    mainSplitter->setStretchFactor( 0, 1 );
    mainSplitter->setStretchFactor( 1, 3 );
    setMainWidget( mainSplitter );

    connect( m_dataFileView, SIGNAL(dirSelected(K3bDirItem*)), m_dataDirTree, SLOT(setCurrentDir(K3bDirItem*)) );
#endif
    addPluginButtons( K3bProjectPlugin::VIDEO_DVD );
}
#warning get the currentDir connections from K3bDataView or maybe inherit from it


K3bVideoDvdView::~K3bVideoDvdView()
{
}


K3bProjectBurnDialog* K3bVideoDvdView::newBurnDialog( QWidget* parent )
{
    return new K3bVideoDvdBurnDialog( m_doc, parent );
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


void K3bVideoDvdView::addUrls( const KUrl::List& urls )
{
	/*
    K3bDataUrlAddingDialog::addUrls( urls, m_dataFileView->currentDir() );
	*/
}

#include "k3bvideodvdview.moc"
