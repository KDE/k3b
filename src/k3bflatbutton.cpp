/*
 *
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

#include "k3bflatbutton.h"
#include "k3bthememanager.h"
#include "k3bapplication.h"

#include <kaction.h>
#include <kiconloader.h>
#include <kglobal.h>

#include <qpainter.h>
#include <qtooltip.h>
#include <qfontmetrics.h>
#include <qpixmap.h>
//Added by qt3to4:
#include <QEvent>
#include <QMouseEvent>
#include <Q3Frame>


K3bFlatButton::K3bFlatButton( QWidget *parent, const char *name )
  : Q3Frame( parent/*, WNoAutoErase*/ ),
    m_pressed(false)
{
  init();
}


K3bFlatButton::K3bFlatButton( const QString& text, QWidget *parent, const char *name )
  : Q3Frame( parent/*, WNoAutoErase*/ ),
    m_pressed(false)
{
  init();
  setText( text );
}


K3bFlatButton::K3bFlatButton( QAction* a, QWidget *parent, const char *name )
  : Q3Frame( parent/*, WNoAutoErase*/ ),
    m_pressed(false)
{
  init();

  setText( a->text() );
  this->setToolTip( a->toolTip() );
  //setPixmap( KIconLoader::global()->loadIcon( a->icon(), KIconLoader::NoGroup, KIconLoader::SizeMedium ) );
  connect( this, SIGNAL(clicked()), a, SLOT(activate()) );
}


K3bFlatButton::~K3bFlatButton() {}


void K3bFlatButton::init()
{
  setHover(false);
  setMargin(5);
  setFrameStyle( Q3Frame::Box|Q3Frame::Plain );

  connect( k3bappcore->themeManager(), SIGNAL(themeChanged()), this, SLOT(slotThemeChanged()) );
  connect( kapp, SIGNAL(appearanceChanged()), this, SLOT(slotThemeChanged()) );
  slotThemeChanged();
}


void K3bFlatButton::setText( const QString& s )
{
  m_text = s;
  m_text.remove( '&' );

  update();
}


void K3bFlatButton::setPixmap( const QPixmap& p )
{
  m_pixmap = p;
  update();
}


void K3bFlatButton::enterEvent( QEvent* )
{
  setHover(true);
}


void K3bFlatButton::leaveEvent( QEvent* )
{
  setHover(false);
}


void K3bFlatButton::mousePressEvent( QMouseEvent* e )
{
  if( e->button() == Qt::LeftButton ) {
    emit pressed();
    m_pressed = true;
  }
  else
    e->ignore();
}


void K3bFlatButton::mouseReleaseEvent( QMouseEvent* e )
{
  if( e->button() == Qt::LeftButton ) {
    if( m_pressed  )
      emit clicked();
    m_pressed = false;
  }
  else
    e->ignore();
}


void K3bFlatButton::setHover( bool b )
{
  if( b ) {
    setPaletteBackgroundColor( m_foreColor );
    setPaletteForegroundColor( m_backColor );
  } else {
    setPaletteBackgroundColor( m_backColor );
    setPaletteForegroundColor( m_foreColor );
  }

  m_hover = b;

  update();
}


QSize K3bFlatButton::sizeHint() const
{
  // height: pixmap + 5 spacing + font height + frame width
  // width: max( pixmap, text) + frame width
  return QSize( qMax( m_pixmap.width(), fontMetrics().width( m_text ) ) + frameWidth()*2, 
		m_pixmap.height() + fontMetrics().height() + 5 + frameWidth()*2 );
}


void K3bFlatButton::drawContents( QPainter* p )
{
  QRect rect = contentsRect();

//   if( m_hover )
//     p->fillRect( rect, m_foreColor );
//   else if( parentWidget() ) {
//     QRect r( mapToParent( QPoint(lineWidth(), lineWidth()) ), 
// 	     mapToParent( QPoint(width()-2*lineWidth(), height()-2*lineWidth() )) );
    
//     parentWidget()->repaint( r );
//   }

  p->save();

  QRect textRect = fontMetrics().boundingRect( m_text );
  int textX = qMax( 0, ( rect.width() - textRect.width() ) / 2 );
  int textY = textRect.height();

  if( !m_pixmap.isNull() ) {
    p->translate( rect.left(), rect.top() );
    textX = qMax( textX, (m_pixmap.width() - textRect.width()) / 2 );
    textY += 5 + m_pixmap.height();

    int pixX = qMax( qMax( 0, (textRect.width() - m_pixmap.width()) / 2 ), 
		     ( rect.width() - m_pixmap.width() ) / 2 );
    p->drawPixmap( pixX, 0, m_pixmap );
    p->drawText( textX, textY, m_text ); 
  }
  else
    p->drawText( rect, Qt::AlignCenter, m_text );

  p->restore();
}


void K3bFlatButton::setColors( const QColor& fore, const QColor& back )
{
  m_foreColor = fore;
  m_backColor = back;

  setHover( m_hover );
}


void K3bFlatButton::slotThemeChanged()
{
  if( K3bTheme* theme = k3bappcore->themeManager()->currentTheme() ) {
    setColors( theme->foregroundColor(), theme->backgroundColor() );
  }
}

#include "k3bflatbutton.moc"
