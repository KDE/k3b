/***************************************************************************
                          k3bfillstatusdisplay.cpp  -  description
                             -------------------
    begin                : Tue Apr 10 2001
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

#include "k3bfillstatusdisplay.h"
#include "audio/k3baudiodoc.h"
#include "k3bglobals.h"

#include <qevent.h>
#include <qpainter.h>
#include <qcolor.h>
#include <qrect.h>
#include <qfont.h>

#include <kaction.h>
#include <kpopupmenu.h>
#include <klocale.h>

K3bFillStatusDisplay::K3bFillStatusDisplay(K3bDoc* _doc, QWidget *parent, const char *name )
  : QFrame(parent,name)
{
  // defaults to megabytes
  m_showTime = false;

  doc = _doc;
	
  setMinimumHeight( (int)((double)QFont::defaultFont().pixelSize() * 1.5) );
  setFrameStyle( Panel | Sunken );
  setupPopupMenu();
}

K3bFillStatusDisplay::~K3bFillStatusDisplay()
{
}


void K3bFillStatusDisplay::setupPopupMenu()
{
  m_popup = new KPopupMenu( this, "popup" );
  m_popup->insertTitle( i18n("Show size in...") );
  m_showMinutes = new KAction( i18n("&Minutes"), "kmidi", 0, this, SLOT(showTime()), this );
  m_showMegs = new KAction( i18n("&Megabytes"), "kwikdisk", 0, this, SLOT(showSize()), this );
  
  m_showMinutes->plug( m_popup );
  m_showMegs->plug( m_popup );
}


void K3bFillStatusDisplay::mousePressEvent( QMouseEvent* e )
{
  if( e->button() == Qt::RightButton )
    m_popup->popup( e->globalPos() );
}


void K3bFillStatusDisplay::drawContents( QPainter* p )
{
  if(m_showTime)
    drawTime(p);
  else
    drawSize(p);
}

void K3bFillStatusDisplay::drawSize(QPainter* p)
{
  // calculate MB
  int value = doc->size()/1024/1024/10;

  // the maximum is 800
  // so split width() in 80 pieces!
  double one = (double)contentsRect().width() / 90.0;
  QRect rect( contentsRect() );
  rect.setWidth( (int)(one*(double)value) );
	
  p->fillRect( rect, Qt::green );
	
  p->drawText( contentsRect(), Qt::AlignLeft | Qt::AlignVCenter, 
	       QString().sprintf( " %.2f MB", ((float)doc->size())/1024.0/1024.0 ) );
	
  // draw yellow if value > 650
  if( value > 65 ) {
    rect.setLeft( rect.left() + (int)(one*65.0) );
    p->fillRect( rect, Qt::yellow );
  }
	
  // draw red if value > 80
  if( value > 70 ) {
    rect.setLeft( rect.left() + (int)(one*5.0) );
    p->fillRect( rect, Qt::red );
  }
	
  // now draw the 650, 700, and 800 marks
  p->drawLine( contentsRect().left() + (int)(one*65.0), contentsRect().bottom(), 
	       contentsRect().left() + (int)(one*65.0), contentsRect().top() );
  p->drawLine( contentsRect().left() + (int)(one*70.0), contentsRect().bottom(), 
	       contentsRect().left() + (int)(one*70.0), contentsRect().top() );
  p->drawLine( contentsRect().left() + (int)(one*80.0), contentsRect().bottom(), 
	       contentsRect().left() + (int)(one*80.0), contentsRect().top() );
	
  // draw the text marks
  rect = contentsRect();
  rect.setRight( (int)(one*65) );
  p->drawText( rect, Qt::AlignRight | Qt::AlignVCenter, "650" );

  rect = contentsRect();
  rect.setRight( (int)(one*70) );
  p->drawText( rect, Qt::AlignRight | Qt::AlignVCenter, "700" );

  rect = contentsRect();
  rect.setRight( (int)(one*80) );
  p->drawText( rect, Qt::AlignRight | Qt::AlignVCenter, "800" );
	
}

void K3bFillStatusDisplay::drawTime(QPainter* p)
{
  int value = doc->length() / 75 / 60;

  // the maximum is 100
  // so split width() in 100 pieces!
  double one = (double)contentsRect().width() / 100.0;
  QRect rect( contentsRect() );
  rect.setWidth( (int)(one*(double)value) );
	
  p->fillRect( rect, Qt::green );
	
  p->drawText( contentsRect(), Qt::AlignLeft | Qt::AlignVCenter, " " + K3b::framesToString( doc->length(), false ) + " min" );
	
  // draw yellow if value > 74
  if( value > 74 ) {
    rect.setLeft( rect.left() + (int)(one*74.0) );
    p->fillRect( rect, Qt::yellow );
  }
	
  // draw red if value > 80
  if( value > 80 ) {
    rect.setLeft( rect.left() + (int)(one*6.0) );
    p->fillRect( rect, Qt::red );
  }
	
  // now draw the 74, 80, and 99 marks
  p->drawLine( contentsRect().left() + (int)(one*74.0), contentsRect().bottom(), 
	       contentsRect().left() + (int)(one*74.0), contentsRect().top() );
  p->drawLine( contentsRect().left() + (int)(one*80.0), contentsRect().bottom(), 
	       contentsRect().left() + (int)(one*80.0), contentsRect().top() );
  p->drawLine( contentsRect().left() + (int)(one*99.0), contentsRect().bottom(), 
	       contentsRect().left() + (int)(one*99.0), contentsRect().top() );
	
  // draw the text marks
  rect = contentsRect();
  rect.setRight( (int)(one*74) );
  p->drawText( rect, Qt::AlignRight | Qt::AlignVCenter, "74" );

  rect = contentsRect();
  rect.setRight( (int)(one*80) );
  p->drawText( rect, Qt::AlignRight | Qt::AlignVCenter, "80" );

  rect = contentsRect();
  rect.setRight( (int)(one*99) );
  p->drawText( rect, Qt::AlignRight | Qt::AlignVCenter, "99" );
}

void K3bFillStatusDisplay::showSize()
{
  m_showTime = false;
  update();
}
	
void K3bFillStatusDisplay::showTime()
{
  m_showTime = true;
  update();
}
