/* 
 *
 * $Id: sourceheader 380067 2005-01-19 13:03:46Z trueg $
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * Based on the effects in popupMessage.cpp 
 * Copyright (C) 2005 by Max Howell <max.howell@methylblue.com>
 *               2005 by Seb Ruiz <me@sebruiz.net>
 *
 * Dissolve Mask (c) Kicker Authors kickertip.cpp, 2005/08/17
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bwidgetshoweffect.h"

#include <qpainter.h>
#include <qwidget.h>


K3bWidgetShowEffect::K3bWidgetShowEffect( QWidget* widget, Effect e )
  : QObject( widget ),
    m_effect( e ),
    m_widget( widget ),
    m_dissolveSize( 0 ),
    m_dissolveDelta( -1 ),
    m_offset( 0 ),
    m_deleteSelf( false ),
    m_bEffectOnly( false )
{
}


K3bWidgetShowEffect::~K3bWidgetShowEffect()
{
}


void K3bWidgetShowEffect::hide( bool effectOnly )
{
  m_bEffectOnly = effectOnly;
  m_bShow = false;
  m_offset = m_widget->height();
  killTimer( m_timerId );
  m_timerId = startTimer( 6 );
}


void K3bWidgetShowEffect::show( bool effectOnly )
{
  m_bShow = true;
  m_offset = 0;
  m_dissolveSize = 24;
  m_dissolveDelta = -1;

  m_widget->polish();

  if( m_effect == Dissolve ) {
    // necessary to create the mask
    m_mask.resize( m_widget->width(), m_widget->height() );
    // make the mask empty and hence will not show widget with show() called below
    dissolveMask();
    m_timerId = startTimer( 1000 / 30 );
  }
  else {
    m_widget->move( 0, m_widget->parentWidget()->height() );
    m_timerId = startTimer( 6 );
  }
  
  if( !effectOnly )
    m_widget->show();
}


void K3bWidgetShowEffect::timerEvent( QTimerEvent* )
{
  switch( m_effect ) {
  case Slide:
    slideMask();
    break;
    
  case Dissolve:
    dissolveMask();
    break;
  }
}


void K3bWidgetShowEffect::dissolveMask()
{
  if( m_bShow ) {
    m_widget->repaint( false );
    QPainter maskPainter(&m_mask);

    m_mask.fill(Qt::black);

    maskPainter.setBrush(Qt::white);
    maskPainter.setPen(Qt::white);
    maskPainter.drawRect( m_mask.rect() );
    
    m_dissolveSize += m_dissolveDelta;

    if( m_dissolveSize > 0 ) {
      maskPainter.setRasterOp( Qt::EraseROP );

      int x, y, s;
      const int size = 16;

      for( y = 0; y < m_widget->height() + size; y += size ) {
	x = m_widget->width();
	s = m_dissolveSize * x / 128;

	for( ; x > size; x -= size, s -= 2 ) {
	  if( s < 0 )
	    break;

	  maskPainter.drawEllipse(x - s / 2, y - s / 2, s, s);
	}
      }
    }
    else if( m_dissolveSize < 0 ) {
      m_dissolveDelta = 1;
      killTimer( m_timerId );

      emit widgetShown( m_widget );

      if( m_deleteSelf )
	deleteLater();
    }

    m_widget->setMask( m_mask );
  }

  else {
    // just hide it for now
    emit widgetHidden( m_widget );
    if( !m_bEffectOnly )
      m_widget->hide();

    if( m_deleteSelf )
      deleteLater();
  }
}


void K3bWidgetShowEffect::slideMask()
{
  if( m_bShow ) {
    m_widget->move( 0, m_widget->parentWidget()->height() - m_offset );
    
    m_offset++;
    if( m_offset > m_widget->height() ) {
      killTimer( m_timerId );

      emit widgetShown( m_widget );
      
      if( m_deleteSelf )
	deleteLater();
    }
  }
  else {
    m_offset--;
    m_widget->move( 0, m_widget->parentWidget()->height() - m_offset );
    
    if( m_offset < 0 ) {
      // finally hide the widget
      emit widgetHidden( m_widget );
      if( !m_bEffectOnly )
	m_widget->hide();

      if( m_deleteSelf )
	deleteLater();
    }
  }
}



K3bWidgetShowEffect* K3bWidgetShowEffect::showWidget( QWidget* w, Effect m )
{
  K3bWidgetShowEffect* e = new K3bWidgetShowEffect( w, m );
  e->m_deleteSelf = true;
  e->show();
  return e;
}


K3bWidgetShowEffect* K3bWidgetShowEffect::hideWidget( QWidget* w, Effect m )
{
  K3bWidgetShowEffect* e = new K3bWidgetShowEffect( w, m );
  e->m_deleteSelf = true;
  e->hide();
  return e;
}

#include "k3bwidgetshoweffect.moc"
