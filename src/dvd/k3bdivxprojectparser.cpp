/***************************************************************************
                          k3bdivxprojectparser.cpp  -  description
                             -------------------
    begin                : Sun Apr 21 2002
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

#include "k3bdivxprojectparser.h"
#include "k3bdvdcodecdata.h"

#include <qobject.h>

K3bDivXProjectParser::K3bDivXProjectParser( K3bDvdCodecData *data ) : QXmlDefaultHandler() {
    m_datas = data;
}

K3bDivXProjectParser::~K3bDivXProjectParser(){
}

bool K3bDivXProjectParser::startDocument(){
    m_level = 0;
    return TRUE;
}
bool K3bDivXProjectParser::startElement( const QString&, const QString&, const QString& qName, const QXmlAttributes& attr){
    switch( m_level ) {
        case 0:
            break;
        case 1: {
            m_datas->setTitle( attr.value("number") );
            qDebug("(K3bDivXProjectParser) Title number: " + attr.value("number") );
            break;
        }
        case 2: {
            qDebug("(K3bDivXProjectParser) Read data of: " + qName );
            m_contentTag = qName;
            break;
        }
        default:
            break;
    }
    m_level++;
    return TRUE;
}

bool K3bDivXProjectParser::endElement( const QString&, const QString&, const QString& ) {
    m_level--;
    return TRUE;
}

bool K3bDivXProjectParser::characters( const QString& content ) {
    QString con = content.stripWhiteSpace();
    if( !con.isEmpty() ){
        qDebug("(K3bDivXProjectParser) Data: " + con );
        if( m_contentTag == "frames" ){
            m_datas->setFrames( con );
        } else if( m_contentTag == "time" ){
            m_datas->setLength( con );
        } else if( m_contentTag == "audiogain" ){
            m_datas->setAudioGain( con );
        } else if( m_contentTag == "aspectratio" ){
            m_datas->setAspectRatio( con );
        } else if( m_contentTag == "width" ){
            m_datas->setWidth( con );
        } else if( m_contentTag == "height" ){
            m_datas->setHeight( con );
        } else if( m_contentTag == "chapters" ){
            m_datas->setChapters( con );
        } else if( m_contentTag == "audiolanguage" ){
            m_datas->addLanguage( con );
        }
    }
    return TRUE;
}
