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

#include "k3bdivxpreview.h"

#include <qcanvas.h>
#include <qpainter.h>
#include <qimage.h>
#include <qpoint.h>
#include <qpixmap.h>
#include <qvaluelist.h>

#include <kdebug.h>

K3bDivxPreview::K3bDivxPreview(QCanvas* c, QWidget *parent, const char *name ) : QCanvasView( c, parent,name) {
    can = c;
    m_initialized = false;
    m_imageScale = 2.0;
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

K3bDivxPreview::~K3bDivxPreview(){
}

void K3bDivxPreview::drawContents( QPainter* p ){
    if( !m_initialized )
         return;
    can->resize( width(), height() );
    bool imageChange=false;
    if( visibleWidth() > 540 && visibleHeight() > 432 ){
        if( m_imageScale == 2.0 ){
            updateLineOffsets( true);
            imageChange = true;
        }
        m_imageScale = 1.333333;
    } else {
        if( m_imageScale != 2.0 ){
            updateLineOffsets( false );
            imageChange = true;
        }
        m_imageScale = 2.0;
    }
    if( imageChange ){
        updatePreviewPicture( m_imageSource );
        updateLines();
    }
    m_sprite->move( (double) (visibleWidth()/2 - 360/m_imageScale), (double)(visibleHeight()/2 -288/m_imageScale ));
    //repaintContents( visibleWidth()/2 - 180, visibleHeight()/2 -144, 360, 288 );
    updateLines();
    repaintContents( 0,0, visibleWidth(), visibleHeight() );
}

void K3bDivxPreview::updateLines(){
     m_lineTop->setPoints( 1, 1, width(), 1 );
     m_lineBottom->setPoints( 1, 1, width(), 1 );
     m_lineLeft->setPoints( 1, 1, 1, height() );
     m_lineRight->setPoints( 1, 1, 1, height() );
     m_lineTop->setY( m_sprite->topEdge() + m_offsetTop );
     m_lineLeft->setX( m_sprite->leftEdge() + m_offsetLeft );
     m_lineBottom->setY( m_sprite->bottomEdge() - m_offsetBottom );
     m_lineRight->setX( m_sprite->rightEdge() - m_offsetRight );
}
void K3bDivxPreview::updateLineOffsets(bool upScale){
    float scale;
    if( upScale ){
        scale = 1.5;
    } else {
        scale = 0.66666;
    }
    m_offsetTop = m_offsetTop * scale;
    m_offsetLeft = m_offsetLeft * scale;
    m_offsetBottom = m_offsetBottom * scale;
    m_offsetRight = m_offsetRight * scale;
}
void K3bDivxPreview::setPreviewPicture( const QString &image ){
    m_imageSource = image;
    if( m_initialized ){
        delete m_sprite;
    }
    QImage i( image );
    QImage preview = i.scale( 720/m_imageScale, 576/m_imageScale );
    m_previewPixmap = new QCanvasPixmap( preview );
    m_previewPixmapArray = new QCanvasPixmapArray();
    m_previewPixmapArray->setImage(0, m_previewPixmap);
    m_sprite = new QCanvasSprite ( m_previewPixmapArray, can );
    m_sprite->setX( visibleWidth()/2 - 360/m_imageScale);
    m_sprite->setY( visibleHeight()/2 -288/m_imageScale);
    m_sprite->show();
    if( !m_initialized ) {
        setCroppingLines();
    } else {
        updateLines();
    }
    repaintContents( 0,0, visibleWidth(), visibleHeight() );
    m_initialized = true;
}

void K3bDivxPreview::resetView(){
    if( m_initialized ){
        setTopLine(0);
        setLeftLine(0);
        setBottomLine(0);
        setRightLine(0);
    }
}

void K3bDivxPreview::updatePreviewPicture( const QString &image ){
    delete m_sprite;
    QValueList<QCanvasItem*> cl = can->allItems();
    QImage i( image );
    QImage preview = i.scale( 720/m_imageScale, 576/m_imageScale );
    m_previewPixmap = new QCanvasPixmap( preview );
    m_previewPixmapArray = new QCanvasPixmapArray();
    m_previewPixmapArray->setImage(0, m_previewPixmap);
    m_sprite = new QCanvasSprite ( m_previewPixmapArray, can );
    m_sprite->setX( visibleWidth()/2 - 360/m_imageScale);
    m_sprite->setY( visibleHeight()/2 -288/m_imageScale);
    m_sprite->show();
    repaintContents( 0,0, visibleWidth(), visibleHeight() );
    updateLines();
}
void K3bDivxPreview::setCroppingLines(){
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

     m_lineTop->show( );
     m_lineLeft->show( );
     m_lineBottom->show( );
     m_lineRight->show( );
     updateLines();
}

void K3bDivxPreview::setTopLine( int offset ){
    offset = offset/m_imageScale;
    int old = (int) m_lineTop->y();
    m_lineTop->setY( m_sprite->topEdge() + offset );
    repaintContents( 0, old, visibleWidth(), 2 ); // old line
    repaintContents( 0, m_sprite->topEdge()+offset, visibleWidth(), 2 ); // new line
    m_offsetTop = m_lineTop->y() - m_sprite->topEdge();
}

void K3bDivxPreview::setLeftLine( int offset ){
    offset = offset/m_imageScale;
    int old = (int) m_lineLeft->x();
    m_lineLeft->setX( m_sprite->leftEdge() + offset );
    repaintContents( old, 0, 2, visibleHeight() ); // old line
    repaintContents( m_sprite->leftEdge()+offset, 0, 2, visibleHeight() ); // new line
    m_offsetLeft = m_lineLeft->x() - m_sprite->leftEdge();
}

void K3bDivxPreview::setBottomLine( int offset ){
    offset = offset/m_imageScale;
    int old = (int) m_lineBottom->y();
    m_lineBottom->setY( m_sprite->bottomEdge() - offset );
    repaintContents( 0, old, visibleWidth(), 2 ); // old line
    repaintContents( 0, m_sprite->bottomEdge()-offset, visibleWidth(), 2 ); // new line
    m_offsetBottom = m_sprite->bottomEdge() - m_lineBottom->y();
}

void K3bDivxPreview::setRightLine( int offset ){
    offset = offset/m_imageScale;
    int old = m_lineRight->x();
    m_lineRight->setX( m_sprite->rightEdge() - offset );
    repaintContents( old, 0, 2, visibleHeight() ); // old line
    repaintContents( m_sprite->rightEdge()-offset, 0, 2, visibleHeight() ); // new line
    m_offsetRight = m_sprite->rightEdge() - m_lineRight->x();
}

#include "k3bdivxpreview.moc"
