/*
 * Copyright (C) 2000 Michael Matz <matz@kde.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef _CDDB_H_
#define _CDDB_H_

#include <qcstring.h>
#include <qvaluelist.h>
#include <qstringlist.h>

class CDDB {
  public:
    CDDB(  );
    ~CDDB(  );
    bool set_server( const char *hostname = 0, unsigned short int port = 0 );
    unsigned int get_discid( QValueList < int >&track_ofs );
    void set_discid( unsigned int id ){ m_discid = id; };
    /*
    * Fills the list of tracks with title. If an error code of 211 is return by cddb server ( inexact match, more entries )
    * the multipleEntries will be include all return values of the server ( e.g <category> <discid> <title> ).
    * Parse this list and set the rigth disc id and try again.
    */
    bool queryCD( QValueList < int >&track_ofs, QStringList& multipleEntries );
    bool queryCD( QValueList < int >&track_ofs, unsigned int id );

    QString title(  ) const {
        return m_title;
    } QString artist(  ) const {
        return m_artist;
    } int trackCount(  ) const {
        return m_tracks;
    } QString track( int i ) const;
  private:
    bool readLine( QCString & s );
    bool writeLine( const QCString & s );
    bool deinit(  );
    bool parse_read_resp(  );
    int fd;
    QCString h_name;
    unsigned short int port;
    bool remote;
    QCString buf;
    unsigned int m_discid;

    int m_tracks;
    QString m_title;
    QString m_artist;
    QStringList m_names;
};

#endif
