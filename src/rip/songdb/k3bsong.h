/***************************************************************************
                          k3bsong.h  -  description
                             -------------------
    begin                : Sat Dec 29 2001
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

#ifndef K3BSONG_H
#define K3BSONG_H

#include <qstring.h>

/**
  *@author Sebastian Trueg
  */

class K3bSong {
public: 
    K3bSong( const QString& filename, const QString& album, const QString& artist, const QString& title, unsigned int discId, int trackNumber);
    K3bSong();
    ~K3bSong();
    const QString& getFilename() const { return m_filename; };
    const QString& getTitle() const { return m_title; };
    const QString& getArtist() const { return m_artist; };
    const QString& getAlbum() const { return m_album; };
    const int getDiscId() const { return m_discId; };
    const int getTrackNumber() const { return m_trackNumber; };

    void setDiscId( unsigned int discid );
    void setTrackNumber( int tn );
    void setFilename( const QString& filename );
    void addContent( const QString& tag, const QString& content );

private:
    QString m_filename;
    QString m_album;
    QString m_artist;
    QString m_title;
    unsigned int m_discId;
    int m_trackNumber;
};

#endif
