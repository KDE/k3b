/* 
 *
 * $Id$
 * Copyright (C) 2003-2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2005 Sebastian Trueg <trueg@k3b.org>
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
#include <qwhatsthis.h>
#include <qptrlist.h>
#include <qtoolbutton.h>

#include <kaction.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>

// application specific includes
#include "k3bview.h"
#include "k3bdoc.h"
#include "k3bfillstatusdisplay.h"
#include "k3bprojectburndialog.h"
#include "k3bprojectplugindialog.h"
#include <k3btoolbox.h>
#include <k3bpluginmanager.h>
#include <k3bprojectplugin.h>
#include <k3bcore.h>


K3bView::K3bView( K3bDoc* pDoc, QWidget *parent, const char* name )
  : QWidget( parent, name ),
    m_doc( pDoc )
{
  QGridLayout* grid = new QGridLayout( this );

  m_toolBox = new K3bToolBox( this, "toolbox" );
  m_fillStatusDisplay = new K3bFillStatusDisplay( m_doc, this );

//   QToolButton* m_buttonBurn = new QToolButton( this );
//   m_buttonBurn->setIconSet( SmallIcon("cdburn") );
//   m_buttonBurn->setTextLabel( i18n("Burn") + "..." );
//   m_buttonBurn->setAutoRaise(true);
//   m_buttonBurn->setTextPosition( QToolButton::Right ); // TODO: QT 3.2: QToolButton::BesideIcon
//   m_buttonBurn->setUsesTextLabel( true );

  grid->addMultiCellWidget( m_toolBox, 0, 0, 0, 1 );
  grid->addMultiCellWidget( m_fillStatusDisplay, 2, 2, 0, 1 );
  //  grid->addWidget( m_buttonBurn, 2, 1 );
  grid->setRowStretch( 1, 1 );
  grid->setColStretch( 0, 1 );
  grid->setSpacing( 5 );
  grid->setMargin( 2 );

  KAction* burnAction = new KAction( i18n("&Burn..."), "cdburn", CTRL + Key_B, this, SLOT(slotBurn()),
				     actionCollection(), "project_burn");
  burnAction->setToolTip( i18n("Open the burning dialog") );
  KAction* propAction = new KAction( i18n("&Properties"), "edit", CTRL + Key_P, this, SLOT(slotProperties()),
				     actionCollection(), "project_properties");
  propAction->setToolTip( i18n("Open the properties dialog") );

  m_toolBox->addButton( burnAction );
  m_toolBox->addSeparator();

  // this is just for testing (or not?)
  // most likely every project type will have it's rc file in the future
  // TODO: remove the toolbar since it only confuses with it's not-proper-configurability. Instead
  //       use the view's toolbox (which has to be added like in the audio view)
  setXML( "<!DOCTYPE kpartgui SYSTEM \"kpartgui.dtd\">"
	  "<kpartgui name=\"k3bproject\" version=\"1\">"
	  "<MenuBar>"
	  " <Menu name=\"project\"><text>&amp;Project</text>"
	  "  <Action name=\"project_burn\"/>"
	  "  <Action name=\"project_properties\"/>"
	  " </Menu>"
	  "</MenuBar>"
#if 0
	  "<ToolBar name=\"projectToolBar\" index=\"1\">"
	  "  <Action name=\"project_burn\"/>"
	  "  <Action name=\"project_properties\"/>"
	  " </ToolBar>"
#endif
	  "</kpartgui>", true );
}

K3bView::~K3bView()
{
}


void K3bView::setMainWidget( QWidget* w )
{
  static_cast<QGridLayout*>(layout())->addMultiCellWidget( w, 1, 1, 0, 1 );
}


void K3bView::slotBurn()
{
  if( m_doc->numOfTracks() == 0 || m_doc->size() == 0 ) {
    KMessageBox::information( this, i18n("Please add files to your project first."),
			      i18n("No Data to Burn"), QString::null, false );
  }
  else {
    K3bProjectBurnDialog* dlg = newBurnDialog( this );
    if( dlg ) {
      dlg->exec(true);
      delete dlg;
    }
    else {
      kdDebug() << "(K3bDoc) Error: no burndialog available." << endl;
    }
  }
}


void K3bView::slotProperties()
{
  K3bProjectBurnDialog* dlg = newBurnDialog( this );
  if( dlg ) {
    dlg->exec(false);
    delete dlg;
  }
  else {
    kdDebug() << "(K3bDoc) Error: no burndialog available." << endl;
  }
}


// KActionCollection* K3bView::actionCollection() const
// {
//   return m_actionCollection; 
// }


void K3bView::addPluginButtons( int projectType )
{
  QPtrList<K3bPlugin> pl = k3bcore->pluginManager()->plugins( "ProjectPlugin" );
  for( QPtrListIterator<K3bPlugin> it( pl ); *it; ++it ) {
    K3bProjectPlugin* pp = dynamic_cast<K3bProjectPlugin*>( *it );
    if( pp && (pp->type() & projectType) ) {
      QToolButton* button = toolBox()->addButton( pp->text(),
						  pp->icon(),
						  pp->toolTip(),
						  pp->whatsThis(),
						  this, 
						  SLOT(slotPluginButtonClicked()) );
      m_plugins.insert( static_cast<void*>(button), pp );
    }
  }
}


void K3bView::slotPluginButtonClicked()
{
  QObject* o = const_cast<QObject*>(sender());
  if( K3bProjectPlugin* p = m_plugins[static_cast<void*>(o)] ) {
    if( p->hasGUI() ) {
      K3bProjectPluginDialog dlg( p, doc(), this );
      dlg.exec();
    }
    else
      p->activate( doc(), this );
  }
}

#include "k3bview.moc"
