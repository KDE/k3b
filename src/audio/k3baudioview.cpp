/***************************************************************************
                          k3baudioview.cpp  -  description
                             -------------------
    begin                : Tue Mar 27 2001
    copyright            : (C) 2001 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "k3baudioview.h"
#include "k3baudiodoc.h"
#include "audiolistview.h"
#include "k3baudioburndialog.h"
#include "../k3bfillstatusdisplay.h"


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

  QGridLayout* grid = new QGridLayout( this );
  grid->setSpacing( 5 );
  grid->setMargin( 2 );
	
  m_songlist = new K3bAudioListView( this, pDoc, this );
  m_fillStatusDisplay = new K3bFillStatusDisplay( doc, this );
  m_fillStatusDisplay->showTime();
  m_burnDialog = 0;
	
  grid->addWidget( m_songlist, 0, 0 );
  grid->addWidget( m_fillStatusDisplay, 1, 0 );
  grid->setRowStretch( 0, 1 );

  connect( m_songlist, SIGNAL(lengthReady()), m_fillStatusDisplay, SLOT(update()) );
}

K3bAudioView::~K3bAudioView(){
}


void K3bAudioView::burnDialog( bool withWriting )
{
  K3bAudioBurnDialog d( (K3bAudioDoc*)getDocument(), this, "audioburndialog", true );
  d.exec( withWriting );
}







#include "k3baudioview.moc"
