/* 
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 * Copyright (C) 2006 Sebastian Trueg <trueg@k3b.org>
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

#include "k3btoolbutton.h"

#include <qstyle.h>
#include <qpainter.h>
#include <qevent.h>

#include <kglobalsettings.h>
#include <kapplication.h>


class K3bToolButton::Private
{
public:
  QPoint mousePressPos;
  bool instantMenu;
};


K3bToolButton::K3bToolButton( QWidget* parent )
  : QToolButton( parent )
{
  d = new Private;
  d->instantMenu = false;
  installEventFilter(this);
}


K3bToolButton::~K3bToolButton()
{
  delete d;
}


void K3bToolButton::setInstantMenu( bool b )
{
  d->instantMenu = b;
}


void K3bToolButton::drawButton( QPainter* p )
{
  QToolButton::drawButton( p );

  //
  // code below comes from ktoolbarbutton.cpp from the kdelibs sources
  // see the file for copyright information
  //
  if( QToolButton::popup() ) {
    QStyle::SFlags arrowFlags = QStyle::Style_Default;
    
    if( isDown() )
      arrowFlags |= QStyle::Style_Down;
    if( isEnabled() )
      arrowFlags |= QStyle::Style_Enabled;
    
    style().drawPrimitive(QStyle::PE_ArrowDown, p,
			  QRect(width()-7, height()-7, 7, 7), colorGroup(),
			  arrowFlags, QStyleOption() );
  }
}


bool K3bToolButton::eventFilter( QObject* o, QEvent* ev )
{
  if( dynamic_cast<K3bToolButton*>(o) == this ) {

    // Popup the menu when the left mousebutton is pressed and the mouse
    // is moved by a small distance.
    if( QToolButton::popup() ) {
      if( ev->type() == QEvent::MouseButtonPress ) {
	QMouseEvent* mev = static_cast<QMouseEvent*>(ev);

	if( d->instantMenu ) {
	  setDown(true);
	  openPopup();
	  return true;
	}
	else {
	  d->mousePressPos = mev->pos();
	}
      }
      else if( ev->type() == QEvent::MouseMove ) {
        QMouseEvent* mev = static_cast<QMouseEvent*>(ev);
        if( !d->instantMenu &&
	    ( mev->pos() - d->mousePressPos).manhattanLength() > KGlobalSettings::dndEventDelay() ) {
	  openPopup();
          return true;
        }
      }
    }
  }

  return QToolButton::eventFilter( o, ev );
}
