/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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


// include files for Qt
#include <qlayout.h>

#include <kaction.h>

// application specific includes
#include "k3bview.h"
#include "k3bdoc.h"
#include "k3bfillstatusdisplay.h"



K3bView::K3bView( K3bDoc* pDoc, QWidget *parent, const char* name )
  : QWidget( parent, name )
{
  m_doc = pDoc;
  m_actionCollection = new KActionCollection( this );

  QGridLayout* grid = new QGridLayout( this );

  m_fillStatusDisplay = new K3bFillStatusDisplay( m_doc, this );

  grid->addWidget( m_fillStatusDisplay, 1, 0 );
  grid->setRowStretch( 0, 1 );
  grid->setSpacing( 5 );
  grid->setMargin( 2 );
}

K3bView::~K3bView()
{
}


void K3bView::setMainWidget( QWidget* w )
{
  ((QGridLayout*)layout())->addWidget( w, 0, 0 );
}


KActionCollection* K3bView::actionCollection() const
{
  return m_actionCollection; 
}


void K3bView::closeEvent(QCloseEvent*)
{
  // DO NOT CALL QWidget::closeEvent(e) here !!
  // This will accept the closing by QCloseEvent::accept() by default.
  // The installed eventFilter() in K3bMainWindow takes care for closing the widget
  // or ignoring the close event
}


#include "k3bview.moc"
