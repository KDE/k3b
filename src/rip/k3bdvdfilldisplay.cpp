/* 
 *
 * $Id$
 * Copyright (C) 2003 Thomas Froescher <tfroescher@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bdvdfilldisplay.h"

#include <qpainter.h>
#include <qrect.h>
#include <qstring.h>

#include <klocale.h>
#include <kdebug.h>

K3bDvdFillDisplay::K3bDvdFillDisplay(QWidget *parent, const char *name ) : QFrame(parent,name) {
    setSizePolicy( QSizePolicy( QSizePolicy::Minimum, QSizePolicy::Preferred ) );
    setFrameStyle( Panel | Sunken );
    m_size = 0;
    m_available = 0;
    m_used = 0;
}

K3bDvdFillDisplay::~K3bDvdFillDisplay(){
}

void K3bDvdFillDisplay::setupGui(){

}

void K3bDvdFillDisplay::drawContents( QPainter* p ){
    QColor color( Qt::green );
    QRect rect( contentsRect() );
    int maxWidth = rect.width();
    float full = (float) m_used / (float) m_size;
    m_dvdFull = (float) ( m_used+m_dvd ) / (float) m_size;
    if( m_dvdFull > 1.0 ) m_dvdFull = 1.0;
    m_aviFull = (float) ( m_used+m_dvd + 700000 ) / m_size;
    if( m_aviFull > 1.0 ) m_aviFull = 1.0;
    if( m_dvdFull > 0.95 ){
        rect.setWidth( maxWidth * 95 / 100 );
        QRect rectY( (int) maxWidth * 95 /100, 
		     0, 
		     (int)((float)maxWidth * (m_dvdFull-0.95)), 
		     rect.height() );
        p->fillRect( rectY, Qt::red );
    } 
    else {
        rect.setWidth ((int)( (float)maxWidth * full ) );
        //kdDebug() << "full " << full << endl;
        // dvd size
        QRect rectB( (int)( (float)maxWidth * full), 
		     0, 
		     (int)( (float)maxWidth * (m_dvdFull - full) ),
		     rect.height() );
        p->fillRect( rectB, Qt::blue );
        // avi Cds size
        //kdDebug() << "full " << m_aviFull << endl;
        QRect rectO( (int)( (float)maxWidth * m_dvdFull ),
		     0, 
		     (int)( (float)maxWidth * ( m_aviFull-m_dvdFull) ),
		     rect.height() );
        p->fillRect( rectO, Qt::yellow );
    }
    p->fillRect( rect, color );    
}

QString K3bDvdFillDisplay::freeWithDvdAvi(){
    return QString().sprintf( "%.2f %% ",  (m_aviFull*100) ) + i18n("(full)");
}
QString K3bDvdFillDisplay::freeWithDvd(){
    m_dvdFull = (float) ( m_used+m_dvd ) / (float) m_size;
    if( m_dvdFull > 1.0 ) m_dvdFull = 1.0;
    return QString().sprintf( "%.2f %% ",  (float)(m_dvdFull*100) )+ i18n("(full)");
}
