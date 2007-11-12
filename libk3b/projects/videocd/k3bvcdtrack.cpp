/*
*
* $Id$
* Copyright (C) 2003-2004 Christian Kvasny <chris@k3b.org>
*
* This file is part of the K3b project.
* Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
* See the file "COPYING" for the exact licensing terms.
*/

#include <kapplication.h>
#include <kconfig.h>

#include <qstring.h>
#include <qfileinfo.h>
//Added by qt3to4:
#include <Q3PtrList>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <kdebug.h>
#include <klocale.h>

// K3b Includes
#include "k3bvcdtrack.h"
#include <k3bglobals.h>

K3bVcdTrack::K3bVcdTrack( Q3PtrList<K3bVcdTrack>* parent, const QString& filename )
        : m_pbcnumkeys( true ),
        m_pbcnumkeysuserdefined( false ),
        m_file( filename )
{
    m_parent = parent;
    m_title = QFileInfo( m_file ).baseName( true );

    m_revreflist = new Q3PtrList<K3bVcdTrack>;

    for ( int i = 0; i < K3bVcdTrack::_maxPbcTracks; i++ ) {
        m_pbctrackmap.insert( i, 0L );
        m_pbcnontrackmap.insert( i, K3bVcdTrack::DISABLED );
        m_pbcusrdefmap.insert( i, false );
    }

    m_reactivity = false;

    m_definedkeysmap.clear();

    mpeg_info = new Mpeginfo();
}


K3bVcdTrack::~K3bVcdTrack()
{}


KIO::filesize_t K3bVcdTrack::size() const
{
    return m_file.size();
}

int K3bVcdTrack::index() const
{
    int i = m_parent->find( this );
    if ( i < 0 )
        kdDebug() << "(K3bVcdTrack) I'm not part of my parent!" << endl;
    return i;
}

void K3bVcdTrack::addToRevRefList( K3bVcdTrack* revreftrack )
{
    kdDebug() << "K3bVcdTrack::addToRevRefList: track = " << revreftrack << endl;

    m_revreflist->append( revreftrack );

    kdDebug() << "K3bVcdTrack::hasRevRef count = " << m_revreflist->count() << " empty = " << m_revreflist->isEmpty() << endl;
}

void K3bVcdTrack::delFromRevRefList( K3bVcdTrack* revreftrack )
{
    if ( !m_revreflist->isEmpty() ) {
        m_revreflist->remove
        ( revreftrack );
    }
}

bool K3bVcdTrack::hasRevRef()
{
    return !m_revreflist->isEmpty() ;
}

void K3bVcdTrack::delRefToUs()
{
    for ( K3bVcdTrack * track = m_revreflist->first(); track; track = m_revreflist->next() ) {
        for ( int i = 0; i < K3bVcdTrack::_maxPbcTracks; i++ ) {
            kdDebug() << "K3bVcdTrack::delRefToUs count = " << m_revreflist->count() << " empty = " << m_revreflist->isEmpty() << " track = " << track << " this = " << this << endl;
            if ( this == track->getPbcTrack( i ) ) {
                track->setPbcTrack( i );
                track->setUserDefined( i, false );
                track->delFromRevRefList( this );
            }
        }
    }
}

void K3bVcdTrack::delRefFromUs()
{
    for ( int i = 0; i < K3bVcdTrack::_maxPbcTracks; i++ ) {
        if ( this->getPbcTrack( i ) ) {
            this->getPbcTrack( i ) ->delFromRevRefList( this );
        }
    }
}

void K3bVcdTrack::setPbcTrack( int which, K3bVcdTrack* pbctrack )
{
    kdDebug() << "K3bVcdTrack::setPbcTrack " << which << ", " << pbctrack << endl;
    m_pbctrackmap.replace( which, pbctrack );
}

void K3bVcdTrack::setPbcNonTrack( int which, int type )
{
    kdDebug() << "K3bVcdTrack::setNonPbcTrack " << which << ", " << type << endl;
    m_pbcnontrackmap.replace( which, type );
}

void K3bVcdTrack::setUserDefined( int which, bool ud )
{
    m_pbcusrdefmap.replace( which, ud );
}

K3bVcdTrack* K3bVcdTrack::getPbcTrack( const int& which )
{
    if ( m_pbctrackmap.find( which ) == m_pbctrackmap.end() )
        return 0;
    else
        return m_pbctrackmap[ which ];
}

int K3bVcdTrack::getNonPbcTrack( const int& which )
{
    if ( m_pbcnontrackmap.find( which ) == m_pbcnontrackmap.end() )
        return 0;
    else
        return m_pbcnontrackmap[ which ];
}

bool K3bVcdTrack::isPbcUserDefined( int which )
{
    return m_pbcusrdefmap[ which ];
}

const QString K3bVcdTrack::resolution()
{
    if ( mpeg_info->has_video ) {
        for ( int i = 0; i < 2; i++ ) {
            if ( mpeg_info->video[ i ].seen ) {
                return QString( "%1 x %2" ).arg( mpeg_info->video[ i ].hsize ).arg( mpeg_info->video[ i ].vsize );
            }
        }
    }

    return i18n( "n/a" );
}

const QString K3bVcdTrack::highresolution()
{
    if ( mpeg_info->has_video ) {
        if ( mpeg_info->video[ 2 ].seen ) {
            return QString( "%1 x %2" ).arg( mpeg_info->video[ 2 ].hsize ).arg( mpeg_info->video[ 2 ].vsize );
        }
    }
    return i18n( "n/a" );
}

const QString K3bVcdTrack::video_frate()
{
    if ( mpeg_info->has_video ) {
        for ( int i = 0; i < 2; i++ ) {
            if ( mpeg_info->video[ i ].seen ) {
                return QString::number( mpeg_info->video[ i ].frate );
            }
        }
    }

    return i18n( "n/a" );
}

const QString K3bVcdTrack::video_bitrate()
{
    if ( mpeg_info->has_video ) {
        for ( int i = 0; i < 2; i++ ) {
            if ( mpeg_info->video[ i ].seen ) {
                return i18n( "%1 bit/s" ).arg( mpeg_info->video[ i ].bitrate ) ;
            }
        }
    }

    return i18n( "n/a" );
}



const QString K3bVcdTrack::video_format()
{
    if ( mpeg_info->has_video ) {
        for ( int i = 0; i < 2; i++ ) {
            if ( mpeg_info->video[ i ].seen ) {
                switch ( mpeg_info->video[ i ].video_format ) {
                    case 0 :
                        return i18n( "Component" );
                        break;
                    case 1 :
                        return "PAL";
                        break;
                    case 2 :
                        return "NTSC";
                        break;
                    case 3 :
                        return "SECAM";
                        break;
                    case 4 :
                        return "MAC";
                        break;
                    case 5 :
                    default:
                        return i18n( "Unspecified" );
                        kdDebug() << "K3bVcdTrack::video_format() :" << mpeg_info->video[ i ].video_format << endl;
                        break;
                }
            }
        }
    }
    return i18n( "n/a" );
}

const QString K3bVcdTrack::video_chroma()
{
    if ( mpeg_info->has_video ) {
        // MPEG1 only supports 4:2:0 Format
        if ( version() == K3bMpegInfo::MPEG_VERS_MPEG1 )
            return QString( "4:2:0" );

        for ( int i = 0; i < 2; i++ ) {
            if ( mpeg_info->video[ i ].seen ) {
                switch ( mpeg_info->video[ i ].chroma_format ) {
                    case 1 :
                        return QString( "4:2:0" );
                        break;
                    case 2 :
                        return QString( "4:2:2" );
                        break;
                    case 3 :
                        return QString( "4:4:4" );
                        break;

                }
            }
        }
    }

    return i18n( "n/a" );
}

const QString K3bVcdTrack::audio_layer()
{
    if ( mpeg_info->has_audio ) {
        for ( int i = 0; i < 2; i++ ) {
            if ( mpeg_info->audio[ i ].seen ) {
                return QString::number( mpeg_info->audio[ i ].layer );
            }
        }
    }

    return i18n( "n/a" );
}

const QString K3bVcdTrack::audio_bitrate()
{
    if ( mpeg_info->has_audio ) {
        for ( int i = 0; i < 2; i++ ) {
            if ( mpeg_info->audio[ i ].seen ) {
                return i18n( "%1 bit/s" ).arg( mpeg_info->audio[ i ].bitrate ) ;
            }
        }
    }

    return i18n( "n/a" );
}

const QString K3bVcdTrack::audio_sampfreq()
{
    if ( mpeg_info->has_audio ) {
        for ( int i = 0; i < 2; i++ ) {
            if ( mpeg_info->audio[ i ].seen ) {
                return i18n( "%1 Hz" ).arg( mpeg_info->audio[ i ].sampfreq ) ;
            }
        }
    }

    return i18n( "n/a" );
}

const QString K3bVcdTrack::audio_mode( )
{
    if ( mpeg_info->has_audio ) {
        for ( int i = 2; i >= 0; i-- )
            if ( mpeg_info->audio[ i ].seen )
                return QString( audio_type2str( mpeg_info->audio[ i ].version, mpeg_info->audio[ i ].mode, i ) );

    }

    return i18n( "n/a" );
}

const QString K3bVcdTrack::audio_copyright( )
{
    if ( mpeg_info->has_audio ) {
        for ( int i = 2; i >= 0; i-- )
            if ( mpeg_info->audio[ i ].seen )
                if ( mpeg_info->audio[ i ].copyright )
                    return QString( "(c) " ) + ( mpeg_info->audio[ i ].original ? i18n( "original" ) : i18n( "duplicate" ) );
                else
                    return ( mpeg_info->audio[ i ].original ? i18n( "original" ) : i18n( "duplicate" ) );
    }

    return i18n( "n/a" );
}

const QString K3bVcdTrack::mpegTypeS( bool audio )
{
    if ( mpeg_info->has_video && !audio ) {
        for ( int i = 0; i < 3; i++ )
            if ( mpeg_info->video[ i ].seen ) {
                if ( i == 0 ) {
                    return QString( "MPEG%1 " ).arg( mpeg_info->version ) + i18n( "Motion Picture" );
                } else {
                    return QString( "MPEG%1 " ).arg( mpeg_info->version ) + i18n( "Still Picture" );
                }
            }
    }
    if ( mpeg_info->has_audio && audio ) {
        for ( int i = 0; i < 3; i++ )
            if ( mpeg_info->audio[ i ].seen ) {
                return QString( "MPEG%1 " ).arg( mpeg_info->audio[ i ].version ) + i18n( "Layer %1" ).arg( mpeg_info->audio[ i ].layer );
            }
    }

    return i18n( "n/a" );
}

const int K3bVcdTrack::mpegType( )
{
    if ( mpeg_info->has_video ) {
        for ( int i = 0; i < 3; i++ )
            if ( mpeg_info->video[ i ].seen ) {
                if ( i == 0 ) {
                    return 0; // MPEG_MOTION;
                } else {
                    return 1; // MPEG_STILL;
                }
            }
    }
    if ( mpeg_info->has_audio ) {
        for ( int i = 0; i < 3; i++ )
            if ( mpeg_info->audio[ i ].seen )
                return 2; // MPEG_AUDIO;
    }

    return -1; // MPEG_UNKNOWN;
}

const QString K3bVcdTrack::audio_type2str( unsigned int version, unsigned int audio_mode, unsigned int audio_type )
{
    kdDebug() << "K3bVcdTrack::audio_type2str() version:" << version << " audio_mode:" << audio_mode << " audio_type:" << audio_type << endl;

    QString audio_types[ 3 ][ 5 ] = {
                                        {
                                            i18n( "unknown" ),
                                            i18n( "invalid" ),
                                            QString::null,
                                            QString::null,
                                            QString::null
                                        },
                                        {
                                            i18n( "stereo" ),
                                            i18n( "joint stereo" ),
                                            i18n( "dual channel" ),
                                            i18n( "single channel" )
                                        },
                                        {
                                            QString::null,
                                            i18n( "dual channel" ),
                                            i18n( "surround sound" ),
                                            QString::null,
                                            QString::null
                                        }
                                    };
    switch ( version ) {
        case K3bMpegInfo::MPEG_VERS_MPEG1:
            return audio_types[ 1 ][ audio_mode ];
            break;

        case K3bMpegInfo::MPEG_VERS_MPEG2:
            if ( audio_type > 0 ) {
                return audio_types[ 2 ][ audio_type ];
            }
            return audio_types[ 1 ][ audio_mode ];
            break;
    }

    return i18n( "n/a" );
}

// convert a time in second to HH:mm:ss notation
QString K3bVcdTrack::SecsToHMS( double duration )
{
    byte hours = ( byte ) ( duration / 3600 );
    byte mins = ( byte ) ( ( duration / 60 ) - ( hours * 60 ) );
    float secs = duration - 60 * mins - 3600 * hours;
    if ( hours != 0 ) {
        return QString( "%1:" ).arg( hours ).rightJustified( 3, ' ' ) + QString( "%1:" ).arg( mins ).rightJustified( 3, '0' ) + QString::number( secs, 'f', 2 );
    }
    if ( mins != 0 ) {
        return QString( "%1:" ).arg( mins ).rightJustified( 3, '0' ) + QString::number( secs, 'f', 2 );
    }
    return QString::number( secs, 'f', 2 );
}

void K3bVcdTrack::PrintInfo()
{

    kdDebug() << "K3bVcdTrack::PrintInfo() ....................." << endl;
    kdDebug() << "  version          : MPEG" << version() << endl;
    kdDebug() << "  duration         : " << duration() << endl;
    kdDebug() << "  muxrate          : " << muxrate() << endl;
    kdDebug() << "  video ......................................" << endl;
    kdDebug() << "    type           : " << mpegTypeS() << endl;
    kdDebug() << "    resolution     : " << resolution() << endl;
    kdDebug() << "    high resolution: " << highresolution() << endl;
    kdDebug() << "    frate          : " << video_frate() << endl;
    kdDebug() << "    bitrate        : " << video_bitrate() << endl;
    kdDebug() << "    format         : " << video_format( ) << endl;
    kdDebug() << "    chroma         : " << video_chroma( ) << endl;
    kdDebug() << "  audio ......................................" << endl;
    kdDebug() << "    type           : " << mpegTypeS( true ) << endl;
    kdDebug() << "    mode           : " << audio_mode() << endl;
    kdDebug() << "    layer          : " << audio_layer() << endl;
    kdDebug() << "    bitrate        : " << audio_bitrate() << endl;
    kdDebug() << "    sampfreq       : " << audio_sampfreq() << endl;

}
