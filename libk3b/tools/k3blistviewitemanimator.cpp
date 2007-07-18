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

#include "k3blistviewitemanimator.h"

#include <qtimer.h>
#include <qlistview.h>

#include <kpixmap.h>
#include <kpixmapeffect.h>


K3bListViewItemAnimator::K3bListViewItemAnimator( QObject* parent, const char* name )
  : QObject( parent, name )
{
  init();
}


K3bListViewItemAnimator::K3bListViewItemAnimator( QListViewItem* item, int col, QObject* parent, const char* name )
  : QObject( parent, name )
{
  init();
  setItem( item, col );
}


K3bListViewItemAnimator::~K3bListViewItemAnimator()
{
}


QListViewItem* K3bListViewItemAnimator::item() const
{
    return m_item;
}


void K3bListViewItemAnimator::init()
{
  m_item = 0;
  m_column = 0;
  m_timer = new QTimer( this );
  connect( m_timer, SIGNAL(timeout()), this, SLOT(slotAnimate()) );
}


void K3bListViewItemAnimator::start()
{
  if( m_item && !m_pixmap.isNull() ) {
    m_animationStep = 0;
    m_animationBack = false;
    m_timer->start( 150 );
  }
  else
    stop();
}


void K3bListViewItemAnimator::stop()
{
  m_timer->stop();
}


void K3bListViewItemAnimator::setItem( QListViewItem* item, int col )
{
  m_item = item;
  m_column = col;
  m_pixmap = *item->pixmap(col);
  m_fadeColor = item->listView()->colorGroup().base();
  start();
}


void K3bListViewItemAnimator::setPixmap( const QPixmap& p )
{
  m_pixmap = p;
  start();
}


void K3bListViewItemAnimator::setColumn( int col )
{
  m_column = col;
  start();
}


void K3bListViewItemAnimator::setFadeColor( const QColor& c )
{
  m_fadeColor = c;
  start();
}


void K3bListViewItemAnimator::slotAnimate()
{
  if( m_item->isVisible() ) {
    double val = (double)m_animationStep;
    val /= 10.0;
    // we need a temp pixmap since KPixmapEffect changes our pixmap
    KPixmap pix( m_pixmap );
    m_item->setPixmap( m_column, KPixmapEffect::fade( pix, val, m_fadeColor ) );;
  }
  
  if( m_animationBack ) {
    --m_animationStep;
    if( m_animationStep < 0 ) {
      // two steps full
      m_animationStep = 0;
      m_animationBack = false;
    }
  }
  else {
    ++m_animationStep;
    // do not fade it completely
    if( m_animationStep > 9 ) {
      m_animationStep = 8;
      m_animationBack = true;
    }
  }
}

#include "k3blistviewitemanimator.moc"
