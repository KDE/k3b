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
#include <qtoolbutton.h>
#include <qtooltip.h>

#include <kaction.h>
#include <kiconloader.h>
#include <klocale.h>

// application specific includes
#include "k3bview.h"
#include "k3bdoc.h"
#include "k3bfillstatusdisplay.h"



K3bView::K3bView( K3bDoc* pDoc, QWidget *parent, const char* name )
  : QWidget( parent, name )
{
  m_doc = pDoc;
  //  m_actionCollection = new KActionCollection( this );

  QGridLayout* grid = new QGridLayout( this );

  m_fillStatusDisplay = new K3bFillStatusDisplay( m_doc, this );

  connect( m_doc, SIGNAL(changed()), m_fillStatusDisplay, SLOT(update()) );

  QToolButton* m_buttonBurn = new QToolButton( this );
  m_buttonBurn->setIconSet( SmallIcon("cdwriter_unmount") );
  m_buttonBurn->setTextLabel( i18n("Burn") + "..." );
  m_buttonBurn->setAutoRaise(true);
  m_buttonBurn->setTextPosition( QToolButton::Right ); // TODO: QT 3.2: QToolButton::BesideIcon
  m_buttonBurn->setUsesTextLabel( true );
  connect( m_buttonBurn, SIGNAL(clicked()),
	   m_doc, SLOT(slotBurn()) );

  grid->addWidget( m_fillStatusDisplay, 1, 0 );
  grid->addWidget( m_buttonBurn, 1, 1 );
  grid->setRowStretch( 0, 1 );
  grid->setColStretch( 0, 1 );
  grid->setSpacing( 5 );
  grid->setMargin( 2 );

  QToolTip::add( m_buttonBurn, i18n("Open the burning dialog") );


  // merge doc actions
  actionCollection()->addDocCollection( pDoc->actionCollection() );

  // this is just for testing (or not?)
  // most likely every project type will have it's rc file in the future
  setXML( "<!DOCTYPE kpartgui SYSTEM \"kpartgui.dtd\">"
	  "<kpartgui name=\"k3bproject\" version=\"1\">"
	  "<MenuBar>"
	  " <Menu name=\"project\"><text>&amp;Project</text>"
	  "  <Action name=\"project_burn\"/>"
	  "  <Action name=\"project_properties\"/>"
	  " </Menu>"
	  "</MenuBar>"
	  "<ToolBar name=\"projectToolBar\" index=\"1\">"
	  "  <Action name=\"project_burn\"/>"
	  "  <Action name=\"project_properties\"/>"
	  " </ToolBar>"
	  "</kpartgui>", true );
}

K3bView::~K3bView()
{
}


void K3bView::setMainWidget( QWidget* w )
{
  ((QGridLayout*)layout())->addMultiCellWidget( w, 0, 0, 0, 1 );
}


// KActionCollection* K3bView::actionCollection() const
// {
//   return m_actionCollection; 
// }


#include "k3bview.moc"
