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
class QStringList;
class QListViewItem;

class KListView;

class K3bCddb;

/**
  *@author Sebastian Trueg
  */

class K3bPatternParser {
public: 
    K3bPatternParser( QStringList *dirs, QStringList *files, KListView *view, K3bCddb *cddb );
    ~K3bPatternParser();
    QString prepareFilename( QString title, int no );
    QString prepareDirectory( QListViewItem *item );
    static QString prepareReplaceName( QString title, QString newChar, bool enabled );
    static QString prepareReplaceFilename( QString title );
    QString prepareReplaceDirectory( QString name );
private:
    QStringList *m_filePattern;
    QStringList *m_dirPattern;
    QStringList *m_titles;
    K3bCddb *m_cddb;
    KListView *m_listView;
    QString getRealDirectory( int i, QListViewItem *item );
    static QString replaceSpaces( QString title, bool replaceSpaces, bool isDirectory, QString *newChar=0 );

};

#endif
