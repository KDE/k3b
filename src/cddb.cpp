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

#include <config.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#define __need_timeval
#endif
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#include <netinet/in.h>

#include <errno.h>
#include <unistd.h>
#include <kdebug.h>

#define KSOCK_NO_BROKEN // we need this very very old method in this fucked piece of code!
#include <ksock.h>

#include <klocale.h>

#include <qregexp.h>

#include "cddb.h"

typedef sockaddr_in ksockaddr_in;


#ifdef __need_timeval
#undef __need_timeval

struct timeval {
   int tv_sec;
   int tv_usec;
};

#endif


CDDB::CDDB(  )
:  fd( 0 ), port( 0 ), remote( false )
{
}

CDDB::~CDDB(  )
{
    deinit(  );
}

bool CDDB::set_server( const char *hostname, unsigned short int _port )
{
    if( fd ) {
        if( h_name == hostname && port == _port )
            return true;
        deinit(  );
    }
    remote = ( hostname != 0 ) && ( *hostname != 0 );
    kdDebug( 7101 ) << "CDDB: set_server, host=" << hostname << "port=" << _port << endl;
    if( remote ) {
        ksockaddr_in addr;
        memset( &addr, 0, sizeof( addr ) );
        if( !KSocket::initSockaddr( &addr, hostname, _port ) )
            return false;
        if( ( fd =::socket( PF_INET, SOCK_STREAM, 0 ) ) < 0 ) {
            fd = 0;
            return false;
        }
        if( ::connect( fd, ( struct sockaddr * ) &addr, sizeof( addr ) ) ) {
            kdDebug( 7101 ) << "CDDB: Can't connect!" << endl;
            ::close( fd );
            fd = 0;
            return false;
        }

        h_name = hostname;
        port = _port;
        QCString r;
        readLine( r );  // the server greeting
        writeLine( "cddb hello kde-user blubb kio_audiocd 0.3" );
        readLine( r );
    }
    return true;
}

bool CDDB::deinit(  )
{
    if( fd ) {
        writeLine( "quit" );
        QCString r;
        readLine( r );
        close( fd );
    }
    h_name.resize( 0 );
    port = 0;
    remote = false;
    fd = 0;
    return true;
}

bool CDDB::readLine( QCString & ret )
{
    int read_length = 0;
    char small_b[128];
    fd_set set;
    ret.resize( 0 );
    while( read_length < 40000 ) {
        // Look for a \n in buf
        int ni = buf.find( '\n' );
        if( ni >= 0 ) {
            // Nice, so return this substring (without the \n),
            // and truncate buf accordingly
            ret = buf.left( ni );
            if( ret.length(  ) && ret[ret.length(  ) - 1] == '\r' )
                ret.resize( ret.length(  ) );
            buf.remove( 0, ni + 1 );
            kdDebug( 7101 ) << "CDDB: got  `" << ret << "'" << endl;
            return true;
        }
        // Try to refill the buffer
        FD_ZERO( &set );
        FD_SET( fd, &set );
        struct timeval tv;
        tv.tv_sec = 60;
        tv.tv_usec = 0;
        if( ::select( fd + 1, &set, 0, 0, &tv ) < 0 )
            return false;
        ssize_t l =::read( fd, &small_b, sizeof( small_b ) );
        if( l <= 0 ) {
            // l==0 normally means fd got closed, but we really need a lineend
            return false;
        }
        read_length += l;
        for( int i = 0; i < l; i++ )
            buf += small_b[i];
    }
    return false;
}

bool CDDB::writeLine( const QCString & line )
{
    const char *b = line.data(  );
    int l = line.length(  );
    kdDebug( 7101 ) << "CDDB: send `" << line << "'" << endl;
    while( l ) {
        ssize_t wl =::write( fd, b, l );
        if( wl < 0 && errno != EINTR )
            return false;
        if( wl < 0 )
            wl = 0;
        l -= wl;
        b += wl;
    }
    l = line.length(  );
    if( l && line.data(  )[l - 1] != '\n' ) {
        char c = '\n';
        ssize_t wl;
        do {
            wl =::write( fd, &c, 1 );
        } while( wl <= 0 && errno == EINTR );
        if( wl <= 0 && errno != EINTR )
            return false;
    }
    return true;
}

unsigned int CDDB::get_discid( QValueList < int >&track_ofs )
{
    unsigned int id = 0;
    int num_tracks = track_ofs.count(  ) - 2;

    // the last two track_ofs[] are disc begin and disc end

    for( int i = num_tracks - 1; i >= 0; i-- ) {
        int n = track_ofs[i];
        n /= 75;
        while( n > 0 ) {
            id += n % 10;
            n /= 10;
        }
    }
    unsigned int l = track_ofs[num_tracks + 1];
    l -= track_ofs[num_tracks];
    l /= 75;
    id = ( ( id % 255 ) << 24 ) | ( l << 8 ) | num_tracks;
    return id;
}

static int get_code( const QCString & s )
{
    bool ok;
    int code = s.left( 3 ).toInt( &ok );
    if( !ok )
        code = -1;
    return code;
}

static void parse_query_resp( const QCString & _r, QCString & catg, QCString & d_id, QCString & title )
{
    QCString r = _r.stripWhiteSpace(  );
    int i = r.find( ' ' );
    if( i ) {
        catg = r.left( i ).stripWhiteSpace(  );
        r.remove( 0, i + 1 );
        r = r.stripWhiteSpace(  );
    }
    i = r.find( ' ' );
    if( i ) {
        d_id = r.left( i ).stripWhiteSpace(  );
        r.remove( 0, i + 1 );
        r = r.stripWhiteSpace(  );
    }
    title = r;
}

QString CDDB::track( int i ) const
{
    if( i < 0 || i >= m_names.count(  ) )
        return QString(  );
    return m_names[i];
}

bool CDDB::parse_read_resp(  )
{
    /* Note, that m_names and m_title should be empty */
    QCString end = ".";

    /* Fill table, so we can index it below.  */
    for( int i = 0; i < m_tracks; i++ ) {
        m_names.append( "" );
    }
    while( 1 ) {
        QCString r;
        if( !readLine( r ) )
            return false;
        if( r == end )
            break;
        r = r.stripWhiteSpace(  );
        if( r.isEmpty(  ) || r[0] == '#' )
            continue;
        if( r.left( 7 ) == "DTITLE=" ) {
            r.remove( 0, 7 );
            m_title += r.stripWhiteSpace(  );
        } else if( r.left( 6 ) == "TTITLE" ) {
            r.remove( 0, 6 );
            int e = r.find( '=' );
            if( e ) {
                bool ok;
                int i = r.left( e ).toInt( &ok );
                if( ok && i >= 0 && i < m_tracks ) {
                    r.remove( 0, e + 1 );
                    m_names[i] += r;
                }
            }
        }
    }

    /* XXX We should canonicalize the strings ("\n" --> '\n' e.g.) */

    int si = m_title.find( '/' );
    if( si > 0 ) {
        m_artist = m_title.left( si ).stripWhiteSpace(  );
        m_title.remove( 0, si + 1 );
        m_title = m_title.stripWhiteSpace(  );
    }

    if( m_title.isEmpty(  ) )
        m_title = i18n( "No Title" );
    else
        m_title.replace( QRegExp( "/" ), "%2f" );
    if( m_artist.isEmpty(  ) )
        m_artist = i18n( "Unknown" );
    else
        m_artist.replace( QRegExp( "/" ), "%2f" );

    kdDebug( 7101 ) << "CDDB: found Title: `" << m_title << "'" << endl;
    for( int i = 0; i < m_tracks; i++ ) {
        if( m_names[i].isEmpty(  ) )
            m_names[i] += i18n( "Track %1" ).arg( i );
        m_names[i].replace( QRegExp( "/" ), "%2f" );
        kdDebug( 7101 ) << "CDDB: found Track " << i + 1 << ": `" << m_names[i]
            << "'" << endl;
    }
    return true;
}

bool CDDB::queryCD( QValueList < int >&track_ofs, unsigned int id ){
    int num_tracks = track_ofs.count(  ) - 2;
    if( !remote || fd == 0 || num_tracks < 1 )
        return false;
    m_tracks = num_tracks;
    m_title = "";
    m_artist = "";
    m_names.clear(  );
    unsigned int length = track_ofs[num_tracks + 1] - track_ofs[num_tracks];
    QCString q;
    q.sprintf( "cddb query %08x %d", id, num_tracks );
    QCString num;
    for( int i = 0; i < num_tracks; i++ )
        q += " " + num.setNum( track_ofs[i] );
    q += " " + num.setNum( length / 75 );
    if( !writeLine( q ) )
        return false;
    QCString r;
    if( !readLine( r ) )
        return false;
    r = r.stripWhiteSpace(  );
    int code = get_code( r );
    if( code == 200 ) {
        QCString catg, d_id, title;
        /* an exact match */
        r.remove( 0, 3 );
        parse_query_resp( r, catg, d_id, title );
        kdDebug( 7101 ) << "CDDB: found exact CD: category=" << catg <<
            " DiscId=" << d_id << " Title=`" << title << "'" << endl;
        q = "cddb read " + catg + " " + d_id;
        if( !writeLine( q ) )
            return false;
        if( !readLine( r ) )
            return false;
        r = r.stripWhiteSpace(  );
        code = get_code( r );
        if( code != 210 )
            return false;
        if( !parse_read_resp(  ) )
            return false;
    } else if( code == 211 ) {
        QCString end = ".";
        /* some close matches */
        //XXX may be try to find marker based on r
        while( 1 ) {
            if( !readLine( r ) )
                return false;
            r = r.stripWhiteSpace(  );
            if( r == end )
                return false;
            QCString catg, d_id, title;
            parse_query_resp( r, catg, d_id, title );
            kdDebug( 7101 ) << "CDDB: found close CD: category=" << catg <<
                " DiscId=" << d_id << " Title=`" << title << "'" << endl;
        }
    } else {
        /* 202 - no match found
         * 403 - Database entry corrupt
         * 409 - no handshake */
        kdDebug( 7101 ) << "CDDB: query returned code " << code << endl;
        return false;
    }

    return true;
}

bool CDDB::queryCD( QValueList < int >&track_ofs, QStringList &multipleEntries )
{
    int num_tracks = track_ofs.count(  ) - 2;
    if( !remote || fd == 0 || num_tracks < 1 )
        return false;
    unsigned int id = get_discid( track_ofs );
    if( id == m_discid )
        return true;

    m_tracks = num_tracks;
    m_title = "";
    m_artist = "";
    m_names.clear(  );
    m_discid = id;
    unsigned int length = track_ofs[num_tracks + 1] - track_ofs[num_tracks];
    QCString q;
    q.sprintf( "cddb query %08x %d", id, num_tracks );
    QCString num;
    for( int i = 0; i < num_tracks; i++ )
        q += " " + num.setNum( track_ofs[i] );
    q += " " + num.setNum( length / 75 );
    if( !writeLine( q ) )
        return false;
    QCString r;
    if( !readLine( r ) )
        return false;
    r = r.stripWhiteSpace(  );
    int code = get_code( r );
    if( code == 200 ) {
        QCString catg, d_id, title;
        /* an exact match */
        r.remove( 0, 3 );
        parse_query_resp( r, catg, d_id, title );
        kdDebug( 7101 ) << "CDDB: found exact CD: category=" << catg <<
            " DiscId=" << d_id << " Title=`" << title << "'" << endl;
        q = "cddb read " + catg + " " + d_id;
        if( !writeLine( q ) )
            return false;
        if( !readLine( r ) )
            return false;
        r = r.stripWhiteSpace(  );
        code = get_code( r );
        if( code != 210 )
            return false;
        if( !parse_read_resp(  ) )
            return false;
    } else if( code == 211 ) {
        QCString end = ".";
        /* some close matches */
        //XXX may be try to find marker based on r
        while( 1 ) {
            if( !readLine( r ) )
                return false;
            r = r.stripWhiteSpace(  );
            multipleEntries.append( r );
            if( r == end )
                return false;
            QCString catg, d_id, title;
            parse_query_resp( r, catg, d_id, title );
            kdDebug( 7101 ) << "CDDB: found close CD: category=" << catg <<
                " DiscId=" << d_id << " Title=`" << title << "'" << endl;
        }
    } else {
        /* 202 - no match found
         * 403 - Database entry corrupt
         * 409 - no handshake */
        kdDebug( 7101 ) << "CDDB: query returned code " << code << endl;
        return false;
    }

    return true;
}
