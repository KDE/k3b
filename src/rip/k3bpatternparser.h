/***************************************************************************
                          k3bpatternparser.h  -  description
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

#ifndef K3BPATTERNPARSER_H
#define K3BPATTERNPARSER_H

#include <qstring.h>

#include "../k3bcddb.h"

class QStringList;
class QListViewItem;

class KListView;

/**
  *@author Sebastian Trueg
  */
class K3bPatternParser 
{
 public: 
  K3bPatternParser();
  ~K3bPatternParser();

  /**
   * returns filename according to configured pattern without extension
   */
  QString prepareFilename( const K3bCddbEntry& entry, int no );

  static QString prepareParsedName( const QString& title, const QString& artist, QString& newArtist, bool enabled = true);

  QString prepareDirectory( const K3bCddbEntry& );
  static QString prepareReplaceName( QString title, QString newChar, bool enabled );
  static QString prepareReplaceFilename( const QString& title );
  QString prepareReplaceDirectory( const QString& name );

  static QString parsePattern( const K3bCddbEntry& entry, 
			       unsigned int trackNumber,
			       const QString& pattern, 
			       bool replace = false, 
			       const QString& replaceString = "_" );

 private:
  QStringList m_filePattern;
  QStringList m_dirPattern;
  QStringList *m_titles;
  QString m_title;
  QString m_artist;
  K3bCddbEntry m_cddb;
  QString getRealDirectory( int i, const K3bCddbEntry& );
  static QString replaceSpaces( QString title, bool replaceSpaces, bool isDirectory, QString *newChar=0 );
  //static void parseTitle( const QString& title, const QString& artist, QString& refTitle, QString &refArtist );
    
};

#endif
