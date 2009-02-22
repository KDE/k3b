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


K3b::VideoDvdView::VideoDvdView( K3b::VideoDvdDoc* doc, QWidget *parent )
    : K3b::StandardView( doc, parent ),
      m_doc(doc)
{
	m_model = new K3b::DataProjectModel(m_doc, this);
	// set the model for the K3b::StandardView's views
	setModel(m_model);

#if 0
    // --- setup GUI ---------------------------------------------------
    QSplitter* mainSplitter = new QSplitter( this );
    m_dataDirTree = new K3b::DataDirTreeView( this, doc, mainSplitter );
    m_dataFileView = new K3b::DataFileView( this, doc, mainSplitter );
    mainSplitter->setStretchFactor( 0, 1 );
    mainSplitter->setStretchFactor( 1, 3 );
    setMainWidget( mainSplitter );

    connect( m_dataFileView, SIGNAL(dirSelected(K3b::DirItem*)), m_dataDirTree, SLOT(setCurrentDir(K3b::DirItem*)) );
#endif
    addPluginButtons( K3b::ProjectPlugin::VIDEO_DVD );
}
#warning get the currentDir connections from K3b::DataView or maybe inherit from it


K3b::VideoDvdView::~VideoDvdView()
{
}


K3b::ProjectBurnDialog* K3b::VideoDvdView::newBurnDialog( QWidget* parent )
{
    return new K3b::VideoDvdBurnDialog( m_doc, parent );
}


void K3b::VideoDvdView::init()
{
    KMessageBox::information( this,
                              i18n("Be aware that you need to provide the complete Video DVD filestructure. "
                                   "K3b does not support video transcoding and preparation of video object "
                                   "files yet. That means you need to already have the VTS_X_YY.VOB "
                                   "and VTS_X_YY.IFO files."),
                              i18n("K3b Video DVD Restrictions"),
                              "video_dvd_restrictions" );
}


void K3b::VideoDvdView::addUrls( const KUrl::List& urls )
{
	/*
    K3b::DataUrlAddingDialog::addUrls( urls, m_dataFileView->currentDir() );
	*/
}

#include "k3bvideodvdview.moc"
