/***************************************************************************
                          k3bdvdcodecdata.cpp  -  description
                             -------------------
    begin                : Mon Apr 1 2002
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

#include "k3bdvdcodecdata.h"
#include "k3bdivxprojectparser.h"

#include "kio/global.h"

K3bDvdCodecData::K3bDvdCodecData(){
    m_iWidth=0;
    m_iHeight=0;
    m_resizeHeight=0;
    m_resizeWidth=0;
    m_cropTop=0;
    m_cropLeft=0;
    m_cropBottom=0;
    m_cropRight=0;
}

K3bDvdCodecData::~K3bDvdCodecData(){
}

void K3bDvdCodecData::setProjectFile( const QString& file ){
    m_projectFile = file;
    int index = file.findRev("/");
    m_projectDir = file.left( index );
    qDebug("ProjectDir: %s", m_projectDir.latin1() );
    loadData();
}

void K3bDvdCodecData::loadData( ){
    K3bDivXProjectParser handler( this );
    QFile xmlFile( m_projectFile );
    QXmlInputSource source( xmlFile );
    QXmlSimpleReader reader;
    reader.setContentHandler( &handler );
    reader.parse( source );
}

void K3bDvdCodecData::setLength( const QString& l){
    m_length = l;
    m_timeLength = QTime::fromString( l );
    qDebug( "Time %s", m_timeLength.toString().latin1() );
}
void K3bDvdCodecData::setAspectRatio( const QString& a){
    m_aspectRatio = a;
    int index = a.find(":");
    QString tmp = a.mid(0, index );
    float w = tmp.toFloat();
    tmp = a.mid( index+1 );
    float h = tmp.toFloat();
    if( h > 0 ){
        m_fAspectRatio =  w / h;
    } else{
         qDebug("(K3bDvdCodecData) error in aspect ratio.");
         m_fAspectRatio = 1.0;
    }
}
void K3bDvdCodecData::setWidth( const QString& w){
    m_width = w;
    m_iWidth = w.toInt();
}
void K3bDvdCodecData::setHeight( const QString& h){
     m_height = h;
     m_iHeight = h.toInt();
    qDebug("Height %d", m_iHeight );
}
void K3bDvdCodecData::addLanguage( const QString& l){
    m_listAudio << l;
}

QString K3bDvdCodecData::getSize(){
    return m_width + "x" + m_height;
}
