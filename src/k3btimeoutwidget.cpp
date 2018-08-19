/* 
 *
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

#include "k3btimeoutwidget.h"
#include "k3bthememanager.h"
#include "k3bapplication.h"

#include <QTimer>
#include <QResizeEvent>
#include <QPainter>
#include <QPaintEvent>


class K3b::TimeoutWidget::Private
{
public:
  int timeout;
  int padding;
  int margin;

  QTimer timer;
  int currentTime;

  bool paused;
};


K3b::TimeoutWidget::TimeoutWidget( QWidget* parent )
  : QWidget( parent )
{
  d = new Private;
  d->timeout = 10000;
  d->padding = 4;
  d->margin = 1;
  d->paused = false;
  connect( &d->timer, SIGNAL(timeout()), this, SLOT(timeStep()) );
}


K3b::TimeoutWidget::~TimeoutWidget()
{
  delete d;
}


void K3b::TimeoutWidget::start()
{
  d->paused = false;
  d->currentTime = 0;
  startTimer();
}


void K3b::TimeoutWidget::stop()
{
  d->paused = false;
  d->timer.stop();
  d->currentTime = 0;
}


void K3b::TimeoutWidget::pause()
{
  d->paused = true;
  d->timer.stop();
}


void K3b::TimeoutWidget::resume()
{
  d->paused = false;
  startTimer();
}


void K3b::TimeoutWidget::timeStep()
{
  d->currentTime += 100;
  update();
  if( d->currentTime >= d->timeout ) {
    stop();
    emit timeout();
  }
}


QSize K3b::TimeoutWidget::sizeHint() const
{
  return minimumSizeHint();
}


QSize K3b::TimeoutWidget::minimumSizeHint() const
{
  int fw = fontMetrics().width( QString::number( d->timeout/1000 ) );
  int fh = fontMetrics().height();

  int diam = qMax( fw, fh ) + 2*d->padding + 2*d->margin;

  return QSize( diam, diam );
}


void K3b::TimeoutWidget::setTimeout( int msecs )
{
  d->timeout = msecs;
}


void K3b::TimeoutWidget::startTimer()
{
  d->timer.start( 100 );
}


void K3b::TimeoutWidget::paintEvent( QPaintEvent* )
{
  if( d->timer.isActive() || d->paused ) {
    QPainter p(this);
    p.setRenderHint( QPainter::Antialiasing );
    
    if( K3b::Theme* theme = k3bappcore->themeManager()->currentTheme() ) {
      p.setBrush( theme->backgroundColor() );
      p.setPen( theme->backgroundColor() );
    }

    QRect r;
    r.setSize( minimumSizeHint() - QSize(2*d->margin,2*d->margin) );
    r.moveCenter( rect().center() );

    p.drawArc( r, 0, 360*16 );
    p.drawPie( r, 90*16, 360*16*d->currentTime/d->timeout );
    
    p.setPen( Qt::black );
    p.drawText( rect(), Qt::AlignCenter, QString::number( (d->timeout - d->currentTime + 500)/1000 ) );
  }
}


void K3b::TimeoutWidget::resizeEvent( QResizeEvent* e )
{
  QWidget::resizeEvent( e );
}



