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


#include "k3bprojecttabbar.h"

#include <kaction.h>
#include <klocale.h>
#include <kdebug.h>
#include <qevent.h>


K3bProjectTabBar::K3bProjectTabBar( QWidget* parent, const char* name )
  : QTabBar( parent, name )
{
  m_projectActionMenu = new KActionMenu( i18n("Project"), this );
}


K3bProjectTabBar::~K3bProjectTabBar(){
}


void K3bProjectTabBar::mousePressEvent( QMouseEvent* e )
{
  if( e->button() == Qt::RightButton ) {
    // we need change the tab because the actions work on the current tab
    QTab* clickedTab = selectTab( e->pos() );
    if( clickedTab ) {
      setCurrentTab( clickedTab );

      // show the popup menu
      m_projectActionMenu->popup( e->globalPos() );
    }
  }

  QTabBar::mousePressEvent(e);
}


void K3bProjectTabBar::insertAction( KAction* action )
{
  m_projectActionMenu->insert( action );
}

#include "k3bprojecttabbar.moc"
