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
#include <qimage.h>
#include <qpoint.h>
#include <qpixmap.h>

K3bDvdPreview::K3bDvdPreview(QCanvas* c, QWidget *parent, const char *name ) : QCanvasView( c, parent,name) {
     can = c;
     //setResizePolicy( Manual );
     setVScrollBarMode( AlwaysOff );
     setHScrollBarMode( AlwaysOff );
/*     lineTop = new QCanvasLine( can );
     lineTop->setPoints( 1, 1, width(), height() );
     QPen pen( Qt::black, 2 );
     lineTop->setPen( pen );
     lineTop->setX( 0 );
     lineTop->setY( 0 );
     lineTop->setZ( 55 );
     lineTop->show( );
*/
    //setCanvas( canvas );
    //setBaseSize(800, 600 );
    //QCanvasPixmap *preview = new QCanvasPixmap( canvas
}

K3bDvdPreview::~K3bDvdPreview(){
}

void K3bDvdPreview::drawContents( QPainter* p ){
    can->resize( width(), height() );
    m_sprite->move( (double) (visibleWidth()/2 - 180), (double)(visibleHeight()/2 -144 ));
    //repaintContents( visibleWidth()/2 - 180, visibleHeight()/2 -144, 360, 288 );
    updateLines();
    repaintContents( 0,0, visibleWidth(), visibleHeight() );
}

void K3bDvdPreview::updateLines(){
     m_lineTop->setPoints( 1, 1, width(), 1 );
     m_lineBottom->setPoints( 1, 1, width(), 1 );
     m_lineLeft->setPoints( 1, 1, 1, height() );
     m_lineRight->setPoints( 1, 1, 1, height() );
     m_lineTop->setY( m_sprite->topEdge() + m_offsetTop );
     m_lineLeft->setX( m_sprite->leftEdge() + m_offsetLeft );
     m_lineBottom->setY( m_sprite->bottomEdge() - m_offsetBottom );
     m_lineRight->setX( m_sprite->rightEdge() - m_offsetRight );
}

void K3bDvdPreview::setPreviewPicture( const QString &image ){
    QImage i( image );
    QImage preview = i.scale( 360, 288 );
    QCanvasPixmap *pix = new QCanvasPixmap( preview );
    QCanvasPixmapArray *a = new QCanvasPixmapArray();
    a->setImage(0, pix);
    m_sprite = new QCanvasSprite ( a, can );
    m_sprite->setX( visibleWidth()/2 - 180);
    m_sprite->setY( visibleHeight()/2 -144);
    m_sprite->show();
    setCroppingLines();
}

void K3bDvdPreview::setCroppingLines(){
     QPen pen( Qt::red, 1 );
     m_lineTop = new QCanvasLine( can );
     m_lineTop->setPoints( 1, 1, width(), 1 );
     m_lineTop->setPen( pen );
     m_lineTop->setX( 1 );
     m_lineTop->setY( m_sprite->topEdge() );
     m_lineTop->setZ( 2 );
     m_lineLeft = new QCanvasLine( can );
     m_lineLeft->setPoints( 1, 1, 1, height() );
     m_lineLeft->setPen( pen );
     m_lineLeft->setX( m_sprite->leftEdge() );
     m_lineLeft->setY( 1 );
     m_lineLeft->setZ( 2 );
     m_lineBottom = new QCanvasLine( can );
     m_lineBottom->setPoints( 1, 1, width(), 1 );
     m_lineBottom->setPen( pen );
     m_lineBottom->setX( 1 );
     m_lineBottom->setY( m_sprite->bottomEdge() );
     m_lineBottom->setZ( 2 );
     m_lineRight = new QCanvasLine( can );
     m_lineRight->setPoints( 1, 1, 1, height() );
     m_lineRight->setPen( pen );
     m_lineRight->setX( m_sprite->rightEdge() );
     m_lineRight->setY( 1 );
     m_lineRight->setZ( 2 );

     updateLines();
     m_lineTop->show( );
     m_lineLeft->show( );
     m_lineBottom->show( );
     m_lineRight->show( );
}

void K3bDvdPreview::setTopLine( int offset ){
    offset = offset/2;
    int old = (int) m_lineTop->y();
    m_lineTop->setY( m_sprite->topEdge() + offset );
    repaintContents( 0, old, visibleWidth(), 2 ); // old line
    repaintContents( 0, m_sprite->topEdge()+offset, visibleWidth(), 2 ); // new line
    m_offsetTop = m_lineTop->y() - m_sprite->topEdge();
}

void K3bDvdPreview::setLeftLine( int offset ){
    offset = offset/2;
    int old = (int) m_lineLeft->x();
    m_lineLeft->setX( m_sprite->leftEdge() + offset );
    repaintContents( old, 0, 2, visibleHeight() ); // old line
    repaintContents( m_sprite->leftEdge()+offset, 0, 2, visibleHeight() ); // new line
    m_offsetLeft = m_lineLeft->x() - m_sprite->leftEdge();
}

void K3bDvdPreview::setBottomLine( int offset ){
    offset = offset/2;
    int old = (int) m_lineBottom->y();
    m_lineBottom->setY( m_sprite->bottomEdge() - offset );
    repaintContents( 0, old, visibleWidth(), 2 ); // old line
    repaintContents( 0, m_sprite->bottomEdge()-offset, visibleWidth(), 2 ); // new line
    m_offsetBottom = m_sprite->bottomEdge() - m_lineBottom->y();
}

void K3bDvdPreview::setRightLine( int offset ){
    offset = offset/2;
    int old = m_lineRight->x();
    m_lineRight->setX( m_sprite->rightEdge() - offset );
    repaintContents( old, 0, 2, visibleHeight() ); // old line
    repaintContents( m_sprite->rightEdge()-offset, 0, 2, visibleHeight() ); // new line
    m_offsetRight = m_sprite->rightEdge() - m_lineRight->x();
}

#include "k3bdvdpreview.moc"
