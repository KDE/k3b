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
#include "k3bdoc.h"

#include <qevent.h>
#include <qpainter.h>
#include <qcolor.h>
#include <qrect.h>
#include <qfont.h>

K3bFillStatusDisplay::K3bFillStatusDisplay(K3bDoc* _doc, QWidget *parent, const char *name )
	 : QFrame(parent,name)
{
	doc = _doc;
//	setBackgroundColor( Qt::white );
	setMinimumHeight( (int)((double)QFont::defaultFont().pixelSize() * 1.5) );
	
	setFrameStyle( Panel | Sunken );
	setLineWidth( 2 );
}

K3bFillStatusDisplay::~K3bFillStatusDisplay()
{
}

void K3bFillStatusDisplay::drawContents( QPainter* p )
{
	int value = doc->size();

	// the maximum is 100
	// so split width() in 100 pieces!
	double one = (double)contentsRect().width() / 100.0;
	QRect rect( contentsRect() );
	rect.setWidth( (int)(one*(double)value) );
	
	p->fillRect( rect, Qt::green );
	
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
	p->drawLine( contentsRect().left() + (int)(one*74.0), contentsRect().bottom(), contentsRect().left() + (int)(one*74.0), contentsRect().top() );
	p->drawLine( contentsRect().left() + (int)(one*80.0), contentsRect().bottom(), contentsRect().left() + (int)(one*80.0), contentsRect().top() );
	p->drawLine( contentsRect().left() + (int)(one*99.0), contentsRect().bottom(), contentsRect().left() + (int)(one*99.0), contentsRect().top() );
	
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
