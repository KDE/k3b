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

K3bPatternParser::K3bPatternParser()
{
//   m_dirPattern = dirs;
//   m_filePattern = files;
  //m_listView = view;

  KConfig* c = kapp->config();
  c->setGroup("Ripping");
  //m_useFilePattern = c->readBoolEntry("useFilePattern", false);
  //  m_useConfigDirectoryPattern = c->readBoolEntry("usePattern", false);
  m_filePattern = c->readListEntry("filePattern");
  m_dirPattern = c->readEntry("dirBasePath");
  m_dirPattern += c->readEntry("dirGroup1");
  m_dirPattern += c->readEntry("dirGroup2");
//   m_spaceDir = c->readBoolEntry("spaceReplaceDir", false);
//   m_spaceFile = c->readBoolEntry("spaceReplaceFile", false);
//   m_parseMixed = c->readBoolEntry("checkSlashFile", true);
//   m_editFile = c->readEntry("spaceReplaceCharFile", "");
//   m_editDir = c->readEntry("spaceReplaceCharDir", "");
}

K3bPatternParser::~K3bPatternParser(){

}

QString K3bPatternParser::prepareFilename( const K3bCddbEntry& entry, int no )
{
  KConfig* c = kapp->config();
  c->setGroup("Ripping");
  m_filePattern = c->readListEntry("filePattern");

  QString fn[5] = { "","","","","" };
  int trackIndex=-1;
  int titleIndex=-1;
  int index=0;
  for( QStringList::Iterator it = m_filePattern.begin(); it != m_filePattern.end(); ++it ) {
    
    if( (*it).find(i18n("Artist")) >= 0)
      fn[index] = entry.artists[no-1];
    else if( (*it).find(i18n("Album")) >= 0 )
      fn[index] = entry.cdTitle;
    else if( (*it).find(i18n("Track No"))  >= 0 )
      fn[index] = QString::number(no).rightJustify( 2, '0' );
    else if( (*it).find(i18n("Title"))  >= 0 )
      fn[index] = entry.titles[no-1];
    else
      fn[index] = (*it).latin1();
    ++index;
  }
  
  return fn[0] + fn[1] + fn[2] + fn[3] + fn[4];
}


QString K3bPatternParser::prepareDirectory( const K3bCddbEntry& entry )
{
  QString result;
  if( !m_dirPattern.isEmpty() ) {
    result = (*m_dirPattern.at(0)).latin1();


    // FIXME: we have to save the cddb flag somewhere
    
    if( !entry.cdTitle.isEmpty() ) {
      for( int i=1; i<3; i++ ){
	QStringList::Iterator it = m_dirPattern.at( i );
	int index = (*it).toInt();
	QString tmp = getRealDirectory( index, entry );
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

QString K3bPatternParser::getRealDirectory( int i, const K3bCddbEntry& entry ){
  QString result;
  switch( i ){
  case 0:
    result = entry.cdArtist;//item->text( COLUMN_ARTIST );
    break;
  case 1:
    result = entry.cdTitle;
    break;
  default:
    result = "";
    break;
  }
  return result;
}

QString K3bPatternParser::prepareReplaceDirectory( const QString& name ){
    QString result = "";
    KConfig* c = kapp->config();
    c->setGroup("Ripping");
    bool replace = c->readBoolEntry( "spaceReplaceDir", false );


    // FIXME: we have to save the cddb flag somewhere
    
    if( !m_cddb.cdTitle.isEmpty() )
        return replaceSpaces( name, replace, true );
    return name;
}

QString K3bPatternParser::prepareReplaceName( QString title, QString newChar, bool enabled ){
    return replaceSpaces( title, enabled, false, &newChar );
}

QString K3bPatternParser::prepareReplaceFilename( const QString& title )
{
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
