/***************************************************************************
                          k3bdvdpreview.cpp  -  description
                             -------------------
    begin                : Tue Apr 2 2002
    copyright            : (C) 2002 by Sebastian Trueg
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

#include "k3bdvdpreview.h"

#include <qcanvas.h>
#include <qpainter.h>

K3bDvdPreview::K3bDvdPreview(QCanvas* c, QWidget *parent, const char *name ) : QCanvasView( c, parent,name) {
     can = c;
     lineTop = new QCanvasLine( can );
     lineTop->setPoints( 1, 1, width(), height() );
     QPen pen( Qt::black, 2 );
     lineTop->setPen( pen );
     lineTop->setX( 0 );
     lineTop->setY( 0 );
     lineTop->setZ( 55 );
     lineTop->show( );

    //setCanvas( canvas );
    setBaseSize(300, 150 );
    //QCanvasPixmap *preview = new QCanvasPixmap( canvas
}

K3bDvdPreview::~K3bDvdPreview(){
}

void K3bDvdPreview::drawContents( QPainter* p ){
    can->resize( width(), height() );
}

#include "k3bdvdpreview.moc"
