/***************************************************************************
                          k3bprojecttabwidget.cpp  -  description
                             -------------------
    begin                : Wed Dec 12 2001
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

#include "k3bprojecttabwidget.h"
#include "k3bprojecttabbar.h"

#include <kaction.h>



K3bProjectTabWidget::K3bProjectTabWidget( QWidget *parent, const char *name, WFlags f )
  : QTabWidget( parent, name, f )
{
  m_tabBar = new K3bProjectTabBar( this, "k3bprojecttabbar" );
  setTabBar( m_tabBar );
}


K3bProjectTabWidget::~K3bProjectTabWidget()
{
}


void K3bProjectTabWidget::insertAction( KAction* action )
{
  m_tabBar->insertAction( action );
}

#include "k3bprojecttabwidget.moc"

