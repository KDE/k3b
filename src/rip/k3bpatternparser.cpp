/***************************************************************************
                          k3bpatternparser.cpp  -  description
                             -------------------
    begin                : Sun Dec 2 2001
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

#include "k3bpatternparser.h"
#include "../k3bcddb.h"

#include <qdir.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qlistview.h>

#include <kapp.h>
#include <kconfig.h>
#include <kio/global.h>
#include <klistview.h>
#include <klocale.h>

#define COLUMN_ARTIST         1

K3bPatternParser::K3bPatternParser( QStringList *dirs, QStringList *files, K3bCddb *cddb ){
    m_dirPattern = dirs;
    m_filePattern = files;
    //m_listView = view;
    m_cddb = cddb;
    m_artist = m_cddb->getArtist();
}

K3bPatternParser::~K3bPatternParser(){

}

QString K3bPatternParser::prepareFilename( QString title, int no, bool parseMixed=false ){
    QString fn[5] = { "","","","","" };
    int trackIndex=-1;
    int titleIndex=-1;
    int index=0;
    for( QStringList::Iterator it = m_filePattern->begin(); it != m_filePattern->end(); ++it ) {
        qDebug("(K3bPatternParser) Filepattern: " + (*it) );
        if( (*it).find(i18n("Artist")) >= 0){
            //fn[index] = m_cddb->getArtist(); //prepareReplaceFilename( m_cddb->getParsedArtist() );
            QString tmpArtist;
            prepareParsedName( title, m_cddb->getArtist(), tmpArtist , parseMixed);
            fn[index] = tmpArtist;
        } else if( (*it).find(i18n("Album")) >= 0 )
            fn[index] = m_cddb->getAlbum();//prepareReplaceFilename( m_cddb->getAlbum() );
        else if( (*it).find(i18n("Track No"))  >= 0 )
            trackIndex = index;
        else if( (*it).find(i18n("Title"))  >= 0 )
            titleIndex = index;
        else
             fn[index] = (*it).latin1();
        ++index;
    }
    if( titleIndex != -1){
        QString newArtist;
        fn[titleIndex] = KIO::decodeFileName( prepareParsedName( title, m_cddb->getArtist(), newArtist , parseMixed) );
    }
    if( trackIndex != -1 ){
        if (no < 10)
            fn[trackIndex] = "0" + QString::number(no);
        else
            fn[trackIndex] = QString::number(no);
    }
    //fn[titleIndex] = prepareReplaceFilename( fn[titleIndex] );
    return fn[0] + fn[1] + fn[2] + fn[3] + fn[4] + ".wav"; //prepareFilename( (*it).latin1() );
}

/*
QString K3bPatternParser::prepareParsedFilename( QString title, int no, QString& newTitle, QString& newArtist ){
    parseTitle( title, m_artist, newTitle, newArtist );
    newArtist = m_artist;
    newTitle = m_title;
    return prepareFilename( title, no, true );
}
*/
QString K3bPatternParser::prepareDirectory( QListViewItem *item ){
    QString result = 0;
        if( !m_dirPattern->isEmpty() ){
            result = (*m_dirPattern->at(0)).latin1();
            if( m_cddb->useCddb() ){
                for( int i=1; i<3; i++ ){
                    QStringList::Iterator it = m_dirPattern->at( i );
                    int index = (*it).toInt();
                    QString tmp = getRealDirectory( index, item );
                    if( !tmp.isEmpty() )
                        result += '/' + tmp;
                }
            }
        } else {
            result = QDir::homeDirPath();
        }
    //qDebug("(K3bPatternParser) Destination directory: "+ result);
    return result; //prepareReplaceDirectory( result );
}

QString K3bPatternParser::getRealDirectory( int i, QListViewItem *item ){
    QString result;
    switch( i ){
        case 0:
            result = item->text( COLUMN_ARTIST );
            break;
        case 1:
            result = m_cddb->getAlbum();
            break;
        default:
            result = "";
            break;
    }
    return result;
}

QString K3bPatternParser::prepareReplaceDirectory( QString name ){
    QString result = "";
    KConfig* c = kapp->config();
    c->setGroup("Ripping");
    bool replace = c->readBoolEntry( "spaceReplaceDir", false );
    if( m_cddb->useCddb() )
        return replaceSpaces( name, replace, true );
    return name;
}

QString K3bPatternParser::prepareReplaceName( QString title, QString newChar, bool enabled ){
    return replaceSpaces( title, enabled, false, &newChar );
}

QString K3bPatternParser::prepareReplaceFilename( QString title ){
    QString result = "";
    KConfig* c = kapp->config();
    c->setGroup("Ripping");
    bool replace = c->readBoolEntry( "spaceReplaceFile", false );
    return replaceSpaces( title, replace, false );
}

QString K3bPatternParser::replaceSpaces( QString title, bool replaceSpaces, bool isDirectory, QString *newChar ){
    QString result = "";
    if( replaceSpaces ){
        KConfig* c = kapp->config();
        c->setGroup("Ripping");
        QString replaceChar;
        if( newChar != 0 ){
            replaceChar=*newChar;
        } else {
            if( isDirectory )
                replaceChar = c->readEntry( "spaceReplaceCharDir", "" );
            else
                replaceChar = c->readEntry( "spaceReplaceCharFile", "" );
        }
        QStringList titleList = QStringList::split( " ", title );
        int count = titleList.count();
        QStringList::Iterator it;
        for( int i = 0; i<count; i++){
            it = titleList.at( i );
            if( i == count-1 ){
                result += (*it).latin1();
                break;
            }
            result += (*it).latin1() + replaceChar;
        }
    } else
        result = title;
    return result;
}

QString K3bPatternParser::prepareParsedName( const QString& title, const QString& artist, QString& newArtist, bool enabled = true ){
    QString refTitle;
    int index = title.find("/");
    if( enabled && (index > 0) ){
        newArtist = (title.left( index -1 )).stripWhiteSpace();
        refTitle = (title.right( title.length() - index -1 )).stripWhiteSpace();
    } else {
        refTitle = title;
        newArtist = artist;
    }
    //qDebug("Title:" + refTitle);
    //qDebug("Artist:" + newArtist);
    return refTitle;
}

/*
void K3bPatternParser::parseTitle( const QString& title, const QString& artist, QString& refTitle, QString& refArtist ){
    int index = title.find("/");
    if( index > 0 ){
        refArtist = (title.left( index -1 )).stripWhiteSpace();
        refTitle = (title.right( title.length() - index -1 )).stripWhiteSpace();
    } else {
        refArtist = artist;
        refTitle = title;
    }
}
*/