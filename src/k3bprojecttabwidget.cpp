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



#include "k3bprojecttabwidget.h"
#include "k3bprojecttabbar.h"

#include <k3bview.h>
#include <k3bdoc.h>

#include <kaction.h>
#include <kiconloader.h>



K3bProjectTabWidget::K3bProjectTabWidget( QWidget *parent, const char *name, WFlags f )
  : QTabWidget( parent, name, f )
{
  m_tabBar = new K3bProjectTabBar( this, "k3bprojecttabbar" );
  setTabBar( m_tabBar );
  connect( m_tabBar, SIGNAL(urlsDropped(int, const KURL::List&)),
	   this, SLOT(slotUrlsDropped(int, const KURL::List&)) );
}


K3bProjectTabWidget::~K3bProjectTabWidget()
{
}


void K3bProjectTabWidget::insertTab( K3bDoc* doc )
{
  QTabWidget::insertTab( doc->view(), doc->view()->caption(), 0 );
  connect( doc, SIGNAL(saved(K3bDoc*)), this, SLOT(slotDocSaved(K3bDoc*)) );
  connect( doc, SIGNAL(changed(K3bDoc*)), this, SLOT(slotDocChanged(K3bDoc*)) );
}


void K3bProjectTabWidget::slotUrlsDropped( int id, const KURL::List& urls )
{
  QWidget* w = page( m_tabBar->indexOf( id ) );
  if( K3bView* v = dynamic_cast<K3bView*>(w) )
    v->doc()->addUrls( urls );
}


void K3bProjectTabWidget::insertAction( KAction* action )
{
  m_tabBar->insertAction( action );
}


void K3bProjectTabWidget::slotDocChanged( K3bDoc* doc )
{
  setTabIconSet( doc->view(), SmallIconSet( "filesave" ) );
}


void K3bProjectTabWidget::slotDocSaved( K3bDoc* doc )
{
  setTabIconSet( doc->view(), QIconSet() );
}

#include "k3bprojecttabwidget.moc"

