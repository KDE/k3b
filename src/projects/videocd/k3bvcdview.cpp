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

// QT-includes
#include <qlayout.h>
#include <qstring.h>


// KDE-includes
#include <klocale.h>
#include <kapplication.h>
#include <kdebug.h>

// K3b Includes
#include "k3bvcdview.h"
#include "k3bvcddoc.h"
#include "k3bvcdlistview.h"
#include "k3bvcdburndialog.h"
#include <k3bfillstatusdisplay.h>

K3bVcdView::K3bVcdView( K3bVcdDoc* pDoc, QWidget* parent, const char *name )
        : K3bView( pDoc, parent, name )
{
    m_doc = pDoc;

    // --- setup GUI ---------------------------------------------------
    QGridLayout* grid = new QGridLayout( this );

    m_vcdlist = new K3bVcdListView( this, pDoc, this );
    setMainWidget( m_vcdlist );
    fillStatusDisplay()->showSize();

    connect( m_vcdlist, SIGNAL( lengthReady() ), fillStatusDisplay(), SLOT( update() ) );
    connect( m_doc, SIGNAL( newTracks() ), fillStatusDisplay(), SLOT( update() ) );
}

K3bVcdView::~K3bVcdView()
{}


#include "k3bvcdview.moc"
