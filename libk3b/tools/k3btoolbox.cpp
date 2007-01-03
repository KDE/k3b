/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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

#include "k3btoolbox.h"

#include <kaction.h>
#include <kpopupmenu.h>
#include <ktoolbarbutton.h>
#include <kiconloader.h>

#include <qtoolbutton.h>
#include <qsizepolicy.h>
#include <qlayout.h>
#include <qwhatsthis.h>
#include <qtooltip.h>
#include <qlabel.h>
#include <qvbox.h>
#include <qstyle.h>
#include <qpainter.h>
#include <qevent.h>
#include <qobjectlist.h>


/**
 * internal class. Do not use!
 */
class K3bToolBoxSeparator : public QWidget
{
  //  Q_OBJECT

 public:
  K3bToolBoxSeparator( K3bToolBox* parent );
  
  QSize sizeHint() const;
  
 protected:
  void paintEvent( QPaintEvent * );
};


K3bToolBoxSeparator::K3bToolBoxSeparator( K3bToolBox* parent )
  : QWidget( parent )
{
  setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Minimum ) );
}


QSize K3bToolBoxSeparator::sizeHint() const
{
  int extent = style().pixelMetric( QStyle::PM_DockWindowSeparatorExtent,
				    this );
  return QSize( extent, 0 );
}


void K3bToolBoxSeparator::paintEvent( QPaintEvent* )
{
  QPainter p( this );
  QStyle::SFlags flags = QStyle::Style_Default|QStyle::Style_Horizontal;

  style().drawPrimitive( QStyle::PE_DockWindowSeparator, &p, rect(),
			 colorGroup(), flags );
}



K3bToolBoxButton::K3bToolBoxButton( KAction* action, QWidget* parent )
  : QToolButton( parent ),
    m_popupMenu(0)
{
  setSizePolicy( QSizePolicy(QSizePolicy::Fixed, sizePolicy().verData()) );
  setAutoRaise( true );

  setIconSet( action->iconSet() );
  setTextLabel( action->text() );
  setEnabled( action->isEnabled() );

  QWhatsThis::add( this, action->whatsThis() );
  if( action->toolTip().isEmpty() )
    QToolTip::add( this, action->text() );
  else
    QToolTip::add( this, action->toolTip() );

//   if( KToggleAction* ta = dynamic_cast<KToggleAction*>( action ) ) {
//     setToggleButton( true );
    
//     // initial state
//     if( ta->isChecked() )
//       toggle();
    
//     connect( ta, SIGNAL(toggled(bool)), this, SLOT(toggle()) );
//     connect( this, SIGNAL(toggled(bool)), ta, SLOT(setChecked(bool)) );
//   }

//  else
  if( KActionMenu* am = dynamic_cast<KActionMenu*>( action ) ) {
    m_popupMenu = am->popupMenu();
    connect( this, SIGNAL(pressed()), this, SLOT(slotPopupActivated()) );
    setPopup( m_popupMenu );
  }

  else {
    connect( this, SIGNAL(clicked()), action, SLOT(activate()) );
  }

  connect( action, SIGNAL(enabled(bool)), this, SLOT(setEnabled(bool)) );
}


K3bToolBoxButton::K3bToolBoxButton( const QString& text, const QString& icon, 
				    const QString& tooltip, const QString& whatsthis,
				    QObject* receiver, const char* slot,
				    QWidget* parent )
  : QToolButton( parent ),
    m_popupMenu(0)
{
  setSizePolicy( QSizePolicy(QSizePolicy::Fixed, sizePolicy().verData()) );
  setAutoRaise( true );

  setTextLabel( text );

  if( icon.isEmpty() )
    setUsesTextLabel( true );
  else
    setIconSet( SmallIconSet( icon ) );

  QWhatsThis::add( this, whatsthis );
  QToolTip::add( this, tooltip );

  if( receiver && slot )
    connect( this, SIGNAL(clicked()), receiver, slot );
}


void K3bToolBoxButton::slotPopupActivated()
{
  // force the toolbutton to open the popupmenu instantly
  openPopup();
}


void K3bToolBoxButton::resizeEvent( QResizeEvent* e )
{
  QToolButton::resizeEvent( e );

  // force icon-only buttons to be square
  if( e->oldSize().height() != e->size().height() &&
      !usesTextLabel() )
    setFixedWidth( e->size().height() );
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
  clear();
}


K3bToolBoxButton* K3bToolBox::addButton( KAction* action, bool forceText )
{
  if( action ) {
    K3bToolBoxButton* b = new K3bToolBoxButton( action, this );
    if( forceText ) {
      b->setUsesTextLabel( true );
      b->setTextPosition( QToolButton::BesideIcon );
    }
    addWidget( b );
    return b;
  }
  else
    return 0;
}


K3bToolBoxButton* K3bToolBox::addButton( const QString& text, const QString& icon, 
					 const QString& tooltip, const QString& whatsthis,
					 QObject* receiver, const char* slot,
					 bool forceText )
{
  K3bToolBoxButton* b = new K3bToolBoxButton( text, icon, tooltip, whatsthis, receiver, slot, this );
  if( forceText ) {
    b->setUsesTextLabel( true );
    b->setTextPosition( QToolButton::BesideIcon );
  }
  addWidget( b );
  return b;
}


void K3bToolBox::addSpacing()
{
  int lastStretch = m_mainLayout->colStretch( m_mainLayout->numCols()-1 );
  m_mainLayout->setColStretch( m_mainLayout->numCols()-1, 0 );
  m_mainLayout->addColSpacing( m_mainLayout->numCols()-1, 8 );
  m_mainLayout->setColStretch( m_mainLayout->numCols(), lastStretch );
}


void K3bToolBox::addSeparator()
{
  K3bToolBoxSeparator* s = new K3bToolBoxSeparator( this );
  addWidget( s );
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


K3bToolBoxButton* K3bToolBox::addToggleButton( KToggleAction* action )
{
  return addButton( action );
}


void K3bToolBox::addWidgetAction( KWidgetAction* action )
{
  addWidget( action->widget() );
  m_doNotDeleteWidgets.append( action->widget() );
}


void K3bToolBox::clear()
{
  // we do not want to delete the widgets from the widgetactions becasue they
  // might be used afterwards
  for( QPtrListIterator<QWidget> it( m_doNotDeleteWidgets ); it.current(); ++it )
    it.current()->reparent( 0L, QPoint() );

  for( QObjectListIterator it2( *children() ); it2.current(); ++it2 )
    if( it2.current()->isWidgetType() )
      delete it2.current();
}

#include "k3btoolbox.moc"
