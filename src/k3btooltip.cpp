/* 
 *
 * $Id: sourceheader 380067 2005-01-19 13:03:46Z trueg $
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
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

#include "k3btooltip.h"
#include "k3bwidgetshoweffect.h"

#include <k3bthememanager.h>
#include <k3bapplication.h>

#include <qtimer.h>
#include <qapplication.h>
#include <qlabel.h>

#include <kdebug.h>
#include <fixx11h.h>


K3bToolTip::K3bToolTip( QWidget* widget )
  : QObject( widget ),
    m_parentWidget( widget ),
    m_currentTip( 0 ),
    m_tipTimer( new QTimer( this ) ),
    m_tipTimeout( 700 )
{
  m_parentWidget->installEventFilter( this );
  connect( m_tipTimer, SIGNAL(timeout()),
	   this, SLOT(slotCheckShowTip()) );
}


K3bToolTip::~K3bToolTip()
{
}


void K3bToolTip::tip( const QRect& rect, const QString& text, int effect )
{
  QLabel* label = new QLabel( text, parentWidget() );
  label->setMargin( 6 );
  if( K3bTheme* theme = k3bappcore->themeManager()->currentTheme() ) {
    label->setPaletteBackgroundColor( theme->backgroundColor() );
    label->setPaletteForegroundColor( theme->foregroundColor() );
  }
  tip( rect, label, (K3bWidgetShowEffect::Effect)effect );
}


void K3bToolTip::tip( const QRect& rect, const QPixmap& pix, int effect )
{
  QLabel* label = new QLabel( parentWidget() );
  label->setMargin( 6 );
  if( K3bTheme* theme = k3bappcore->themeManager()->currentTheme() ) {
    label->setPaletteBackgroundColor( theme->backgroundColor() );
    label->setPaletteForegroundColor( theme->foregroundColor() );
  }
  label->setPixmap( pix );
  tip( rect, label, (K3bWidgetShowEffect::Effect)effect );
}


void K3bToolTip::tip( const QRect& rect, QWidget* w, int effect )
{
  // stop the timer
  m_tipTimer->stop();

  // hide any previous tip
  hideTip();

  // which screen are we on?
  int scr;
  if( QApplication::desktop()->isVirtualDesktop() )
    scr = QApplication::desktop()->screenNumber( m_parentWidget->mapToGlobal( m_lastMousePos ) );
  else
    scr = QApplication::desktop()->screenNumber( m_parentWidget );

  // make sure the widget is displayed correcly
  w->reparent( QApplication::desktop()->screen( scr ),
	       WStyle_StaysOnTop | WStyle_Customize | WStyle_NoBorder | WStyle_Tool | WX11BypassWM,
	       QPoint( 0, 0 ), false );
  w->polish();
  w->adjustSize();

  // positioning code from qtooltip.cpp
  QRect screen = QApplication::desktop()->screenGeometry( scr );

  // FIXME: why (2,16) and (4,24) below? Why not use the cursors' size?

  QPoint p = m_parentWidget->mapToGlobal( m_lastMousePos ) + QPoint( 2, 16 );

  if( p.x() + w->width() > screen.x() + screen.width() )
    p.rx() -= 4 + w->width();
  if( p.y() + w->height() > screen.y() + screen.height() )
    p.ry() -= 24 + w->height();
  
  if( p.y() < screen.y() )
    p.setY( screen.y() );
  if( p.x() + w->width() > screen.x() + screen.width() )
    p.setX( screen.x() + screen.width() - w->width() );
  if( p.x() < screen.x() )
    p.setX( screen.x() );
  if( p.y() + w->height() > screen.y() + screen.height() )
    p.setY( screen.y() + screen.height() - w->height() );

  m_currentTip = w;
  m_currentTipRect = rect;
  w->move( p );
  if( effect )
    K3bWidgetShowEffect::showWidget( w, (K3bWidgetShowEffect::Effect)effect );
  else
    w->show();
  w->raise();
}


void K3bToolTip::hideTip()
{
  // just remove the tip
  delete m_currentTip;
  m_currentTip = 0;
}


bool K3bToolTip::eventFilter( QObject* o, QEvent* e )
{
  if( o == parentWidget() ) {
    switch( e->type() ) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
      // input - turn off tool tip mode
      hideTip();
      m_tipTimer->stop();
      break;

    case QEvent::MouseMove: {
      QMouseEvent* m = (QMouseEvent*)e;
      m_lastMousePos = m_parentWidget->mapFromGlobal( m->globalPos() );

      m_tipTimer->stop();
      if( m_currentTip ) {
	// see if we have to hide it
	if( !m_currentTipRect.contains( m_lastMousePos ) ) {
	  hideTip();

	  // in case we moved the mouse from one tip area to the next without leaving
	  // the widget just popup the new tip immedeately
	  m_tipTimer->start( 0, true );
	}
      }

      // if we are not showing a tip currently start the tip timer
      else
	m_tipTimer->start( m_tipTimeout, true );

      break;
    }

    case QEvent::Leave:
    case QEvent::Hide:
    case QEvent::Destroy:
    case QEvent::FocusOut:
      hideTip();
      m_tipTimer->stop();
      break;

    default:
      break;
    }
  }

  return false;
}


void K3bToolTip::slotCheckShowTip()
{
  maybeTip( m_lastMousePos );
}


#include "k3btooltip.moc"
