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
     lineTop->setPoints( 1, 1, can->width(), can->height() );
     QPen pen( Qt::black, 12 );
     lineTop->setPen( pen );
     lineTop->setX( 5 );
     lineTop->setY( 35 );
     lineTop->setZ( 55 );
     lineTop->show( );

    //setCanvas( canvas );
    setBaseSize(300, 150 );
    //QCanvasPixmap *preview = new QCanvasPixmap( canvas
}

K3bDvdPreview::~K3bDvdPreview(){
}

void K3bDvdPreview::drawContents( QPainter* p ){
    //p->drawText( contentsRect(), Qt::AlignLeft | Qt::AlignVCenter,
//        " blubMB" );
     //p->setPen( QPen( Qt::red, 6 ) );
     //p->drawLine( 100,1,100,100 );
     qDebug("drawContents");
     /*lineTop = new QCanvasLine( can );
     lineTop->setPoints( 1, 1, 10, 200 );
     QPen pen( Qt::red, 12 );
     lineTop->setPen( pen );
     lineTop->setX( 5 );
     lineTop->setY( 35 );
     lineTop->setZ( 55 );
     lineTop->show( );
     */
     //can->update();
     //can->drawArea( QRect(0,0,1000,600), p);
     //lineTop->drawArea( QRect(0,0,300,150), p );
     //drawContents( p, 0,0,300,150);
}

#include "k3bdvdpreview.moc"
