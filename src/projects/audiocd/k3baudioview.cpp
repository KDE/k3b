/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3baudioview.h"
#include "k3baudiodoc.h"
#include "audiolistview.h"
#include "k3baudioburndialog.h"
#include <k3bfillstatusdisplay.h>
#include <k3bmsf.h>

// QT-includes
#include <qlayout.h>
#include <qstring.h>


// KDE-includes
#include <klocale.h>
#include <kapplication.h>
#include <kdebug.h>


K3bAudioView::K3bAudioView( K3bAudioDoc* pDoc, QWidget* parent, const char *name )
  : K3bView( pDoc, parent, name )
{
  m_doc = pDoc;

  m_songlist = new K3bAudioListView( this, pDoc, this );
  setMainWidget( m_songlist );
  fillStatusDisplay()->showTime();
}

K3bAudioView::~K3bAudioView(){
}

#include "k3baudioview.moc"
