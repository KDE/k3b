/***************************************************************************
                          k3bdvdcontent.cpp  -  description
                             -------------------
    begin                : Sun Feb 24 2002
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

#include "k3bdvdcontent.h"

K3bDvdContent::K3bDvdContent(){
}
K3bDvdContent::~K3bDvdContent(){
}
void K3bDvdContent::setRes( const QString& str ){
    m_sres = str;
    QString s = str.stripWhiteSpace();
    int index = s.find("x");
    int width = s.left(index).toInt();
    int height = s.mid(index+1).toInt();
    m_res.setWidth( width );
    m_res.setHeight( height );
}
void K3bDvdContent::setAspect( const QString& str ){
    m_saspect = str;
    QString s = str.stripWhiteSpace();
    int index = s.find(":");
    int width = s.left(index).toInt();
    int height = s.mid(index+1).toInt();
    m_aspect.setWidth( width );
    m_aspect.setHeight( height );
}

void K3bDvdContent::setTime( const QString& str ){
    m_stime= str;  // in seconds
    QString s = str.stripWhiteSpace();
    m_time = m_time.addSecs( s.toInt() );
}
void K3bDvdContent::setFrames( const QString& str ){
    m_sframes = str;
    QString s = str.stripWhiteSpace();
    m_frames = s.toLong();
}
void K3bDvdContent::setFramerate( const QString& str ){
    m_sframerate = str;
    QString s = str.stripWhiteSpace();
    m_framerate = s.toFloat();
}
void K3bDvdContent::setAudioList( const QStringList & l ){
    m_audioList = l;
}

void K3bDvdContent::addAngle( const QString &number){
    m_selectedAngle << number;
}

bool K3bDvdContent::isAllAngle( ){
    //qDebug("must be selectedAngle count %i", m_selectedAngle.count() );
    //qDebug("maxAngle %i", m_maxAngle );
    if ( m_maxAngle == m_selectedAngle.count() ){
        return true;
    }
    return false;
}

