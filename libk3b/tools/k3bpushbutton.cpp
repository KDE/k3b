/* 
 *
 * $Id$
 * Copyright (C) 2004 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bpushbutton.h"

#include <qtimer.h>
#include <q3popupmenu.h>
#include <qevent.h>
//Added by qt3to4:
#include <QMouseEvent>

#include <kglobalsettings.h>
#include <kapplication.h>



class K3bPushButton::Private
{
public:
  Private() 
    : popupTimer(0) {
  }

  QTimer* popupTimer;
  QPoint mousePressPos;
};



K3bPushButton::K3bPushButton( QWidget* parent, const char* name )
  : KPushButton( parent, name )
{
  d = new Private();
  installEventFilter(this);
}


K3bPushButton::K3bPushButton( const QString& text, QWidget* parent, const char* name )
  : KPushButton( text, parent, name )
{
  d = new Private();
  installEventFilter(this);
}


K3bPushButton::K3bPushButton( const QIcon& icon, const QString& text,
			      QWidget* parent, const char* name )
  : KPushButton( icon, text, parent, name )
{
  d = new Private();
  installEventFilter(this);
}


K3bPushButton::K3bPushButton( const KGuiItem& item, QWidget* parent, const char* name )
  : KPushButton( item, parent, name )
{
  d = new Private();
  installEventFilter(this);
}


K3bPushButton::~K3bPushButton()
{
  delete d;
}


void K3bPushButton::setDelayedPopupMenu( Q3PopupMenu* popup )
{
  if( !d->popupTimer ) {
    d->popupTimer = new QTimer( this );
    connect( d->popupTimer, SIGNAL(timeout()), this, SLOT(slotDelayedPopup()) );
  }

  setPopup( popup );

  // we need to do the popup handling ourselves so we cheat a little
  // QPushButton connects a popup slot to the pressed signal which we disconnect here
  disconnect( this );
}


bool K3bPushButton::eventFilter( QObject* o, QEvent* ev )
{
  if( dynamic_cast<K3bPushButton*>(o) == this ) {

    // Popup the menu when the left mousebutton is pressed and the mouse
    // is moved by a small distance.
    if( popup() ) {
      if( ev->type() == QEvent::MouseButtonPress ) {
        QMouseEvent* mev = static_cast<QMouseEvent*>(ev);
        d->mousePressPos = mev->pos();
	d->popupTimer->start( QApplication::startDragTime() );
      }
      else if( ev->type() == QEvent::MouseMove ) {
        QMouseEvent* mev = static_cast<QMouseEvent*>(ev);
        if( ( mev->pos() - d->mousePressPos).manhattanLength() > KGlobalSettings::dndEventDelay() ) {
	  d->popupTimer->stop();
	  slotDelayedPopup();
          return true;
        }
      }
    }
  }

  return KPushButton::eventFilter( o, ev );
}


void K3bPushButton::slotDelayedPopup()
{
  d->popupTimer->stop();

  if( isDown() ) {
    // popup the menu.
    // this has been taken from the QPushButton code
    if( mapToGlobal( QPoint( 0, rect().bottom() ) ).y() + popup()->sizeHint().height() <= qApp->desktop()->height() )
      popup()->exec( mapToGlobal( rect().bottomLeft() ) );
    else
      popup()->exec( mapToGlobal( rect().topLeft() - QPoint( 0, popup()->sizeHint().height() ) ) );
    setDown( false );
  }
}

#include "k3bpushbutton.moc"
