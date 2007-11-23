/*
 *
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
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
#include <q3whatsthis.h>
#include <q3ptrlist.h>
#include <qtoolbutton.h>
//Added by qt3to4:
#include <Q3GridLayout>

#include <kaction.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <ktoolbar.h>

// application specific includes
#include "k3bview.h"
#include "k3bdoc.h"
#include "k3bfillstatusdisplay.h"
#include "k3bprojectburndialog.h"
#include "k3bprojectplugindialog.h"
#include <k3bpluginmanager.h>
#include <k3bprojectplugin.h>
#include <k3bcore.h>
#include "k3baction.h"


K3bView::K3bView( K3bDoc* pDoc, QWidget *parent )
  : QWidget( parent ),
    m_doc( pDoc )
{
  Q3GridLayout* grid = new Q3GridLayout( this );

  m_toolBox = new KToolBar( this );
  m_fillStatusDisplay = new K3bFillStatusDisplay( m_doc, this );

  grid->addMultiCellWidget( m_toolBox, 0, 0, 0, 1 );
  grid->addMultiCellWidget( m_fillStatusDisplay, 2, 2, 0, 1 );
  //  grid->addWidget( m_buttonBurn, 2, 1 );
  grid->setRowStretch( 1, 1 );
  grid->setColStretch( 0, 1 );
  grid->setSpacing( 5 );
  grid->setMargin( 2 );

  KAction* burnAction = K3b::createAction(this,i18n("&Burn"), "tools-media-optical-burn", Qt::CTRL + Qt::Key_B, this, SLOT(slotBurn()),
				     actionCollection(), "project_burn");
  burnAction->setToolTip( i18n("Open the burn dialog for the current project") );
  KAction* propAction = K3b::createAction(this, i18n("&Properties"), "document-properties", Qt::CTRL + Qt::Key_P, this, SLOT(slotProperties()),
				     actionCollection(), "project_properties");
  propAction->setToolTip( i18n("Open the properties dialog") );

  m_toolBox->addAction( burnAction/*, true*/ );
  m_toolBox->addSeparator();

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
	  "</kpartgui>", true );
}

K3bView::~K3bView()
{
}


void K3bView::setMainWidget( QWidget* w )
{
  static_cast<Q3GridLayout*>(layout())->addMultiCellWidget( w, 1, 1, 0, 1 );
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
      dlg->execBurnDialog(true);
      delete dlg;
    }
    else {
      kDebug() << "(K3bDoc) Error: no burndialog available.";
    }
  }
}


void K3bView::slotProperties()
{
  K3bProjectBurnDialog* dlg = newBurnDialog( this );
  if( dlg ) {
    dlg->execBurnDialog(false);
    delete dlg;
  }
  else {
    kDebug() << "(K3bDoc) Error: no burndialog available.";
  }
}


// KActionCollection* K3bView::actionCollection() const
// {
//   return m_actionCollection;
// }


void K3bView::addPluginButtons( int projectType )
{
  QList<K3bPlugin*> pl = k3bcore->pluginManager()->plugins( "ProjectPlugin" );
  for( QList<K3bPlugin*>::const_iterator it = pl.begin();
       it != pl.end(); ++it ) {
    K3bProjectPlugin* pp = dynamic_cast<K3bProjectPlugin*>( *it );
    if( pp && (pp->type() & projectType) ) {
      QAction* button = toolBox()->addAction(     pp->text(),
						  this,
						  SLOT(slotPluginButtonClicked()) );
      button->setIcon(QIcon(pp->icon()));
      button->setToolTip (pp->toolTip());
      button->setWhatsThis(pp->whatsThis());
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


void K3bView::addUrl( const KUrl& url )
{
  KUrl::List urls(url);
  addUrls( urls );
}


void K3bView::addUrls( const KUrl::List& urls )
{
  doc()->addUrls( urls );
}

#include "k3bview.moc"
