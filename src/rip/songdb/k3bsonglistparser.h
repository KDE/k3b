/***************************************************************************
                          k3bsonglistparser.h  -  description
                             -------------------
    begin                : Sun Dec 30 2001
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

#ifndef K3BSONGLISTPARSER_H
#define K3BSONGLISTPARSER_H

#include <qxml.h>
#include <qstring.h>
class K3bSongManager;
class K3bSongContainer;
class K3bSong;

/**
  *@author Sebastian Trueg
  */

class K3bSongListParser : public QXmlDefaultHandler  {
public: 
    K3bSongListParser( K3bSongManager *manager );
    ~K3bSongListParser();
    bool startDocument();
    bool startElement( const QString&, const QString&, const QString&, const QXmlAttributes& );
    bool endElement( const QString&, const QString&, const QString& );
    bool characters( const QString& content );
    /*void setDocumentLocator( QXmlLocator* locator );
    bool endDocument();
    bool startPrefixMapping( const QString&, const QString&);
    bool endPrefixMapping( const QString& );
    bool ignorableWhitespace( const QString& );
    bool processingInstruction( const QString&, const QString&);
    bool skippedEntity( const QString& );
    QString errorString();
    */
private:
    int m_level;
    QString m_containerPath;
    QString m_contentTag;
    K3bSongManager *m_manager;
    K3bSongContainer *m_container;
    K3bSong *m_song;

};

#endif
