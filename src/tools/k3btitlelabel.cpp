/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3btitlelabel.h"

#include <qpainter.h>
#include <qevent.h>
#include <qfontmetrics.h>
#include <qfont.h>


class K3bTitleLabel::Private
{
public:
  Private() {
    titleLength = subTitleLength = 0;
    margin = 2;
  }

  QString title;
  QString subTitle;

  int titleLength;
  int subTitleLength;
  int titleBaseLine;
  int subTitleBaseLine;
  int margin;
};


K3bTitleLabel::K3bTitleLabel( QWidget* parent, const char* name )
  : QFrame( parent, name )
{
  d = new Private();
}


K3bTitleLabel::~K3bTitleLabel()
{
  delete d;
}


void K3bTitleLabel::setTitle( const QString& title, const QString& subTitle )
{
  d->title = title;
  d->subTitle = subTitle;
  updatePositioning();
  update();
}

QSize K3bTitleLabel::sizeHint() const
{
  return minimumSizeHint();
}

QSize K3bTitleLabel::minimumSizeHint() const
{
  return QSize( d->titleLength + d->subTitleLength + 2*d->margin, d->titleBaseLine );
}

void K3bTitleLabel::resizeEvent( QResizeEvent* e )
{
  QFrame::resizeEvent( e );
  updatePositioning();
  update();
}

void K3bTitleLabel::drawContents( QPainter* p )
{
  p->save();

  QRect r = contentsRect();
  p->fillRect( r, QColor( 205, 210, 255 ) );

  QFont f(font());
  f.setBold(true);
  f.setPointSize( 14 );

  p->setFont(f);

  // paint title
  p->drawText( r.left() + d->margin, r.top() + d->titleBaseLine, d->title );

  if( !d->subTitle.isEmpty() ) {
    f.setBold(false);
    f.setPointSize( 10 );
    p->setFont(f);
    p->drawText( r.left() + d->margin + d->titleLength, r.top() + d->subTitleBaseLine, d->subTitle );
  }

  p->restore();
}


void K3bTitleLabel::setMargin( int m )
{
  d->margin = m;
  updatePositioning();
  update();
}


void K3bTitleLabel::updatePositioning()
{
  QFont f(font());
  f.setBold(true);
  f.setPointSize( 14 );
  QFontMetrics titleFm(f);

  f.setBold(false);
  f.setPointSize(10);
  QFontMetrics subTitleFm(f);

  d->titleBaseLine = contentsRect().height()/2 + titleFm.height()/2 - titleFm.descent();
  d->titleLength = titleFm.width( d->title ) + 5;  // a fixed distance between title and subtitle is not good. :(

  d->subTitleBaseLine = d->titleBaseLine - titleFm.underlinePos() + subTitleFm.underlinePos();

  d->subTitleLength = ( d->subTitle.isEmpty() ? 0 : subTitleFm.width( d->subTitle ) );
}

#include "k3btitlelabel.moc"
