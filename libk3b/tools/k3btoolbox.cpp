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

#include "k3btoolbox.h"

#include <kaction.h>
#include <kpopupmenu.h>
#include <ktoolbarbutton.h>

#include <qtoolbutton.h>
#include <qsizepolicy.h>
#include <qlayout.h>
#include <qwhatsthis.h>
#include <qlabel.h>



K3bToolBoxButton::K3bToolBoxButton( KAction* action, QWidget* parent )
  : QToolButton( parent ),
    m_popupMenu(0)
{
  setSizePolicy( QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed) );
  setIconSet( action->iconSet() );
  setTextLabel( action->toolTip(), true );
  setTextLabel( action->text() );
  setAutoRaise( true );
  setEnabled( action->isEnabled() );

  QWhatsThis::add( this, action->whatsThis() );

  if( KToggleAction* ta = dynamic_cast<KToggleAction*>( action ) ) {
    setToggleButton( true );
    
    // initial state
    if( ta->isChecked() )
      toggle();
    
    connect( ta, SIGNAL(toggled(bool)), this, SLOT(toggle()) );
    connect( this, SIGNAL(toggled(bool)), ta, SLOT(setChecked(bool)) );
  }

  else if( KActionMenu* am = dynamic_cast<KActionMenu*>( action ) ) {
    m_popupMenu = am->popupMenu();
    connect( this, SIGNAL(pressed()), this, SLOT(slotPopupActivated()) );
    setPopup( m_popupMenu );
  }

  else {
    connect( this, SIGNAL(clicked()), action, SLOT(activate()) );
  }

  connect( action, SIGNAL(enabled(bool)), this, SLOT(setEnabled(bool)) );
}


void K3bToolBoxButton::slotPopupActivated()
{
  // force the toolbutton to open the popupmenu instantly
  openPopup();
}







K3bToolBox::K3bToolBox( QWidget* parent, const char* name )
  : QFrame( parent, name )
{
  setSizePolicy( QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed) );

  m_mainLayout = new QGridLayout( this );
  m_mainLayout->setMargin( 1 );
  m_mainLayout->setSpacing( 0 );
}


K3bToolBox::~K3bToolBox()
{
  // we do not want to delete the widgets from the widgetactions becasue they
  // might be used afterwards
  for( QPtrListIterator<QWidget> it( m_doNotDeleteWidgets ); it.current(); ++it )
    it.current()->reparent( 0L, QPoint() );
}


void K3bToolBox::addButton( KAction* action )
{
  addWidget( new K3bToolBoxButton( action, this ) );
}


void K3bToolBox::addSpacing()
{
  int lastStretch = m_mainLayout->colStretch( m_mainLayout->numCols()-1 );
  m_mainLayout->setColStretch( m_mainLayout->numCols()-1, 0 );
  m_mainLayout->addColSpacing( m_mainLayout->numCols()-1, 8 );
  m_mainLayout->setColStretch( m_mainLayout->numCols(), lastStretch );
}


void K3bToolBox::addLineSpacing()
{
  QFrame* line = new QFrame( this );
  line->setFrameStyle( QFrame::VLine|QFrame::Sunken );
  addWidget( line );
}


void K3bToolBox::addStretch()
{
  // add an empty widget
  addWidget( new QWidget( this ) );
  m_mainLayout->setColStretch( m_mainLayout->numCols(), 1 );
}


void K3bToolBox::addLabel( const QString& text )
{
  QLabel* label = new QLabel( text, this );
  label->setSizePolicy( QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed) );

  addWidget( label );
}


void K3bToolBox::addWidget( QWidget* w )
{
  w->reparent( this, QPoint() );

  m_mainLayout->setColStretch( m_mainLayout->numCols()-1, 0 );

  m_mainLayout->addWidget( w, 0, m_mainLayout->numCols()-1 );

  if( w->sizePolicy().horData() == QSizePolicy::Fixed || w->sizePolicy().horData() == QSizePolicy::Maximum )
    m_mainLayout->setColStretch( m_mainLayout->numCols(), 1 );
  else {
    m_mainLayout->setColStretch( m_mainLayout->numCols()-1, 1 );
    m_mainLayout->setColStretch( m_mainLayout->numCols(), 0 );
  }
}


void K3bToolBox::addToggleButton( KToggleAction* action )
{
  addButton( action );
}


void K3bToolBox::addWidgetAction( KWidgetAction* action )
{
  addWidget( action->widget() );
  m_doNotDeleteWidgets.append( action->widget() );
}


#include "k3btoolbox.moc"
