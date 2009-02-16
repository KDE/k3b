/*
*
* Copyright (C) 2003-2004 Christian Kvasny <chris@k3b.org>
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

// K3b Includes
#include "k3bvcdprojectmodel.h"
#include "k3bvcdview.h"
#include "k3bvcddoc.h"
#include "k3bvcdlistview.h"
#include "k3bvcdburndialog.h"
#include <k3bfillstatusdisplay.h>
#include <k3bexternalbinmanager.h>
#include <k3bcore.h>

// QT-includes
#include <qlayout.h>
#include <qstring.h>


// KDE-includes
#include <klocale.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kmessagebox.h>

K3bVcdView::K3bVcdView( K3bVcdDoc* pDoc, QWidget* parent )
        : K3bStandardView( pDoc, parent )
{
    m_doc = pDoc;

    m_model = new K3b::VcdProjectModel(m_doc, this);
    // set the model for the K3bStandardView's views
    setModel(m_model);
    setShowDirPanel(false);

#if 0
    // --- setup GUI ---------------------------------------------------

    m_vcdlist = new K3bVcdListView( this, pDoc, this );
    setMainWidget( m_vcdlist );
    fillStatusDisplay() ->showSize();

    connect( m_vcdlist, SIGNAL( lengthReady() ), fillStatusDisplay(), SLOT( update() ) );
    connect( m_doc, SIGNAL( newTracks() ), fillStatusDisplay(), SLOT( update() ) );
#endif
}

K3bVcdView::~K3bVcdView()
{}


K3bProjectBurnDialog* K3bVcdView::newBurnDialog( QWidget * parent)
{
  return new K3bVcdBurnDialog( m_doc, parent );
}


void K3bVcdView::init()
{
  if( !k3bcore->externalBinManager()->foundBin( "vcdxbuild" ) ) {
    kDebug() << "(K3bVcdView) could not find vcdxbuild executable";
    KMessageBox::information( this,
			      i18n( "Could not find VcdImager executable. "
				    "To create VideoCD's you must install VcdImager >= 0.7.12. "
				    "You can find this on your distribution disks or download "
				    "it from http://www.vcdimager.org" ) );
  }
}

#include "k3bvcdview.moc"
