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
#include "k3bdivxcodecdata.h"

#include <qobject.h>
#include <kdebug.h>

K3bDivXProjectParser::K3bDivXProjectParser( K3bDivxCodecData *data ) : QXmlDefaultHandler() {
    m_data = data;
}

K3bDivXProjectParser::~K3bDivXProjectParser(){
}

bool K3bDivXProjectParser::startDocument(){
    m_level = 0;
    return TRUE;
}
bool K3bDivXProjectParser::startElement( const QString&, const QString&, const QString& qName, const QXmlAttributes& attr){
    bool result = true;
    switch( m_level ) {
        case 0: {
            if( qName != "k3bDVDTitles" ){
                kdDebug() << "(K3bDivXProjectParser) No K3b DVD Ripping Project File" << endl;
                result=false;
            }
            break;
        }
        case 1: {
            m_data->setTitle( attr.value("number") );
            kdDebug() << "(K3bDivXProjectParser) Title number: " << attr.value("number") << endl;
            break;
        }
        case 2: {
            kdDebug() << "(K3bDivXProjectParser) Read data of: " << qName << endl;
            m_contentTag = qName;
            break;
        }
        default:
            break;
    }
    m_level++;
    return result;
}

bool K3bDivXProjectParser::endElement( const QString&, const QString&, const QString& ) {
    m_level--;
    return TRUE;
}

bool K3bDivXProjectParser::characters( const QString& content ) {
    QString con = content.stripWhiteSpace();
    if( !con.isEmpty() ){
        kdDebug() << "(K3bDivXProjectParser) Data: " << con << endl;
        if( m_contentTag == "frames" ){
            m_data->setFrames( con );
        } else if( m_contentTag == "fps" ){
            m_data->setFramerate( con );
        } else if( m_contentTag == "time" ){
            m_data->setLength( con );
        } else if( m_contentTag == "audiogain" ){
            m_data->setAudioGain( con );
        } else if( m_contentTag == "aspectratio" ){
            m_data->setAspectRatio( con );
        } else if( m_contentTag == "aspectratioAnamorph" ){
            m_data->setAspectRatioAnamorph( con );
        } else if( m_contentTag == "aspectratioExtension" ){
            m_data->setAspectRatioExtension( con );
        } else if( m_contentTag == "width" ){
            m_data->setWidth( con );
        } else if( m_contentTag == "height" ){
            m_data->setHeight( con );
        } else if( m_contentTag == "chapters" ){
            m_data->setChapters( con );
        } else if( m_contentTag == "audiolanguage" ){
            m_data->addLanguage( con );
        }
    }
    return TRUE;
}
