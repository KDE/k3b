/*
 *
 * $Id$
 * Copyright (C) 2003 Christian Kvasny <chris@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bvcdview.h"
#include "k3bvcddoc.h"
#include "k3bvcdlistview.h"
#include "k3bvcdburndialog.h"
#include <k3bfillstatusdisplay.h>


// QT-includes
#include <qlayout.h>
#include <qstring.h>


// KDE-includes
#include <klocale.h>
#include <kapplication.h>
#include <kdebug.h>


K3bVcdView::K3bVcdView( K3bVcdDoc* pDoc, QWidget* parent, const char *name )
  : K3bView( pDoc, parent, name )
{
  m_doc = pDoc;

  // --- setup GUI ---------------------------------------------------
  QGridLayout* grid = new QGridLayout( this );

  m_vcdlist = new K3bVcdListView( this, pDoc, this );
  m_fillStatusDisplay = new K3bFillStatusDisplay( doc, this );
  m_fillStatusDisplay->showSize();
  m_burnDialog = 0;

  grid->addWidget( m_vcdlist, 0, 0 );
  grid->addWidget( m_fillStatusDisplay, 1, 0 );
  grid->setRowStretch( 0, 1 );
  grid->setSpacing( 5 );
  grid->setMargin( 2 );


  connect( m_vcdlist, SIGNAL(lengthReady()), m_fillStatusDisplay, SLOT(update()) );
  connect( m_doc, SIGNAL(newTracks()), m_fillStatusDisplay, SLOT(update()) );
}

K3bVcdView::~K3bVcdView(){
}


void K3bVcdView::burnDialog( bool withWriting )
{
  K3bVcdBurnDialog d( (K3bVcdDoc*)getDocument(), this, "vcdburndialog", true );
  d.exec( withWriting );
}


#include "k3bvcdview.moc"
