/* 
 *
 * $Id$
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bjobprogressosd.h"

#include <k3bthememanager.h>
#include <k3bapplication.h>

#include <kwin.h>
#include <kiconloader.h>
#include <kdebug.h>
#include <kcursor.h>
#include <kconfig.h>

#include <qpixmap.h>
#include <qpainter.h>
#include <qapplication.h>

#include <X11/Xlib.h>


K3bJobProgressOSD::K3bJobProgressOSD( QWidget* parent, const char* name )
  : QWidget( parent, name, WType_TopLevel | WNoAutoErase | WStyle_Customize | WX11BypassWM | WStyle_StaysOnTop ),
    m_dirty(true),
    m_dragging(false),
    m_screen(0),
    m_offset(s_outerMargin),
    m_alignment(TOP)
{
  setFocusPolicy( NoFocus );
  setBackgroundMode( NoBackground );

  // dummy size
  resize( 20, 20 );

  // make sure we are always visible
  KWin::setOnAllDesktops( winId(), true );
}


K3bJobProgressOSD::~K3bJobProgressOSD()
{
}


void K3bJobProgressOSD::show()
{
  // start with 0 progress
  setProgress(0);

  if( m_dirty )
    renderOSD();
  
  QWidget::show();
}


void K3bJobProgressOSD::setText( const QString& text )
{
  if( m_text != text ) {
    m_text = text;
    refresh();
  }
}


void K3bJobProgressOSD::setProgress( int p )
{
  if( m_progress != p ) {
    m_progress = p;
    refresh();
  }
}


void K3bJobProgressOSD::setOffset( int o )
{
  m_offset = o;
  reposition();
}


void K3bJobProgressOSD::setAlignment( K3bJobProgressOSD::Alignment a )
{
  m_alignment = a;
  reposition();
}


void K3bJobProgressOSD::refresh()
{
  if( isVisible() )
    renderOSD();
  else
    m_dirty = true;
}


void K3bJobProgressOSD::renderOSD()
{
  // ----------------------------------------
  // |        Copying CD                    |
  // |  K3B   ========== 40%                |
  // |                                      |
  // ----------------------------------------

  // calculate needed size 
  if( K3bTheme* theme = k3bappcore->themeManager()->currentTheme() ) {
    QPixmap icon = KGlobal::iconLoader()->loadIcon( "k3b", KIcon::NoGroup, 32 );
    int margin = 10;
    int textWidth = fontMetrics().width( m_text );

    // do not change the size everytime the text changes, just in case we are too small
    QSize newSize( QMAX( QMAX( 2*margin + icon.width() + margin + textWidth, 100 ), width() ),
		   QMAX( 2*margin + icon.height(), 2*margin + fontMetrics().height()*2 ) );
    
    m_osdBuffer.resize( newSize );
    QPainter p( &m_osdBuffer );
    
    p.setPen( theme->foregroundColor() );
    
    // draw the background and the frame
    QRect thisRect( 0, 0, newSize.width(), newSize.height() );
    p.fillRect( thisRect, theme->backgroundColor() );
    p.drawRect( thisRect );
    
    // draw the k3b icon
    p.drawPixmap( margin, (newSize.height()-icon.height())/2, icon );
    
    // draw the text
    QSize textSize = fontMetrics().size( 0, m_text );
    int textX = 2*margin + icon.width();
    int textY = margin + fontMetrics().ascent();
    p.drawText( textX, textY, m_text );
    
    // draw the progress
    textY += fontMetrics().descent() + 4;
    QRect progressRect( textX, textY, newSize.width()-textX-margin, newSize.height()-textY-margin );
    p.drawRect( progressRect );
    progressRect.setWidth( m_progress > 0 ? m_progress*progressRect.width()/100 : 0 );
    p.fillRect( progressRect, theme->foregroundColor() );

    // reposition the osd
    reposition( newSize );

    m_dirty = false;

    update();
  }
}


void K3bJobProgressOSD::setScreen( int screen )
{
  const int n = QApplication::desktop()->numScreens();
  m_screen = (screen >= n) ? n-1 : screen;
  reposition();
}


void K3bJobProgressOSD::reposition( QSize newSize )
{
  if( !newSize.isValid() )
    newSize = size();

  QPoint newPos( s_outerMargin, s_outerMargin );
  const QRect screen = QApplication::desktop()->screenGeometry( m_screen );

  // make sure m_offset is valid
  if( m_alignment == TOP || m_alignment == BOTTOM ) {
    if( m_offset > screen.width() - 2*s_outerMargin - newSize.width() )
      m_offset = screen.width() - 2*s_outerMargin - newSize.width();
  }
  else {
    if( m_offset > screen.height() - 2*s_outerMargin - newSize.height() )
      m_offset = screen.height() - 2*s_outerMargin - newSize.height();
  }

  switch( m_alignment ) {
  case RIGHT:
    newPos.rx() = screen.width() - s_outerMargin - newSize.width();
    // fallthrough

  case LEFT:
    newPos.ry() = m_offset + s_outerMargin;
    break;

  case BOTTOM:
    newPos.ry() = screen.height() - s_outerMargin - newSize.height();
    // fallthrough

  case TOP:
    newPos.rx() = m_offset + s_outerMargin;
    break;
  }
 
  // correct for screen position
  newPos += screen.topLeft();
  
  // ensure we are painted before we move
  if( isVisible() )
    paintEvent( 0 );

  // fancy X11 move+resize, reduces visual artifacts
  XMoveResizeWindow( x11Display(), winId(), newPos.x(), newPos.y(), newSize.width(), newSize.height() );
}


void K3bJobProgressOSD::paintEvent( QPaintEvent* )
{
  bitBlt( this, 0, 0, &m_osdBuffer );
}


void K3bJobProgressOSD::mousePressEvent( QMouseEvent* e )
{
  m_dragOffset = e->pos();

  if( e->button() == LeftButton && !m_dragging ) {
    grabMouse( KCursor::sizeAllCursor() );
    m_dragging = true;
  }
}


void K3bJobProgressOSD::mouseReleaseEvent( QMouseEvent* )
{
  if( m_dragging ) {
    m_dragging = false;
    releaseMouse();

    // compute current Position && offset
    QDesktopWidget *desktop = QApplication::desktop();
    int currentScreen = desktop->screenNumber( pos() );
    
    if( currentScreen != -1 ) {
      // set new data
      m_screen = currentScreen;
    }
  }
}


void K3bJobProgressOSD::mouseMoveEvent( QMouseEvent* e )
{
  // FIXME: allow movement along the edges of the screen
  if( m_dragging && this == mouseGrabber() ) {
    const QRect screen      = QApplication::desktop()->screenGeometry( m_screen );
    const uint  hcenter     = screen.width() / 2;
    const uint  eGlobalPosX = e->globalPos().x() - screen.left();
    const uint  snapZone    = screen.width() / 8;
    
    QPoint destination = e->globalPos() - m_dragOffset - screen.topLeft();
    int maxY = screen.height() - height() - s_outerMargin;
    if( destination.y() < s_outerMargin ) destination.ry() = s_outerMargin;
    if( destination.y() > maxY ) destination.ry() = maxY;

    if( eGlobalPosX < (hcenter-snapZone) ) {
      m_alignment = LEFT;
      destination.rx() = s_outerMargin;
    }
    else if( eGlobalPosX > (hcenter+snapZone) ) {
      m_alignment = RIGHT;
      destination.rx() = screen.width() - s_outerMargin - width();
    }
    else {
      const uint eGlobalPosY = e->globalPos().y() - screen.top();
      const uint vcenter     = screen.height()/2;

      destination.rx() = hcenter - width()/2;

      if( eGlobalPosY < (vcenter-snapZone) ) {
	m_alignment = TOP;
	destination.ry() = s_outerMargin;
      }
      else {
 	m_alignment = BOTTOM;
	destination.ry() = screen.height() - s_outerMargin - height();
      }
    }

    destination += screen.topLeft();

    m_offset = ( m_alignment == TOP || m_alignment == BOTTOM ? destination.x() : destination.y() ) - s_outerMargin;

    move( destination );
  }
}


void K3bJobProgressOSD::readSettings( KConfig* c )
{
  QString oldGroup = c->group();
  c->setGroup( "OSD Position" );

  setOffset( c->readNumEntry( "Offset", 0 ) );
  setScreen( c->readNumEntry( "Screen", 0 ) );

  QString alignS = c->readEntry( "Alignment", "top" );
  if( alignS == "left" )
    setAlignment( LEFT );
  else if( alignS == "right" )
    setAlignment( RIGHT );
  else if( alignS == "bottom" )
    setAlignment( BOTTOM );
  else
    setAlignment( TOP );
    
  c->setGroup( oldGroup );
}


void K3bJobProgressOSD::saveSettings( KConfig* c )
{
  QString oldGroup = c->group();
  c->setGroup( "OSD Position" );

  c->writeEntry( "Offset", m_offset );
  c->writeEntry( "Screen", m_screen );

  QString alignS;
  switch( m_alignment ) {
  case TOP:
    alignS = "top";
    break;
  case BOTTOM:
    alignS = "bottom";
    break;
  case LEFT:
    alignS = "tleft";
    break;
  case RIGHT:
    alignS = "right";
    break;
  }
  c->writeEntry( "Alignment", alignS );

  c->setGroup( oldGroup );
}

#include "k3bjobprogressosd.moc"
