/*
*
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

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <kdebug.h>
#include <klocale.h>

// K3b Includes
#include "k3bvcdtrack.h"
#include <k3bglobals.h>

K3bVcdTrack::K3bVcdTrack( QList<K3bVcdTrack*>* parent, const QString& filename )
        : m_pbcnumkeys( true ),
        m_pbcnumkeysuserdefined( false ),
        m_file( filename )
{
    m_parent = parent;
    m_title = QFileInfo( m_file ).completeBaseName();

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
    // (trueg): I have no idea why I need to const cast here!
    int i = m_parent->indexOf( const_cast<K3bVcdTrack*>( this ) );
    if ( i < 0 )
        kDebug() << "(K3bVcdTrack) I'm not part of my parent!";
    return i;
}

void K3bVcdTrack::addToRevRefList( K3bVcdTrack* revreftrack )
{
    kDebug() << "K3bVcdTrack::addToRevRefList: track = " << revreftrack;

    m_revreflist.append( revreftrack );

    kDebug() << "K3bVcdTrack::hasRevRef count = " << m_revreflist.count() << " empty = " << m_revreflist.isEmpty();
}

void K3bVcdTrack::delFromRevRefList( K3bVcdTrack* revreftrack )
{
    m_revreflist.removeAll( revreftrack );
}

bool K3bVcdTrack::hasRevRef()
{
    return !m_revreflist.isEmpty() ;
}

void K3bVcdTrack::delRefToUs()
{
    Q_FOREACH( K3bVcdTrack* track, m_revreflist ) {
        for ( int i = 0; i < K3bVcdTrack::_maxPbcTracks; i++ ) {
            kDebug() << "K3bVcdTrack::delRefToUs count = " << m_revreflist.count() << " empty = " << m_revreflist.isEmpty() << " track = " << track << " this = " << this;
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
    kDebug() << "K3bVcdTrack::setPbcTrack " << which << ", " << pbctrack;
    m_pbctrackmap.replace( which, pbctrack );
}

void K3bVcdTrack::setPbcNonTrack( int which, int type )
{
    kDebug() << "K3bVcdTrack::setNonPbcTrack " << which << ", " << type;
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

QString K3bVcdTrack::resolution()
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

QString K3bVcdTrack::highresolution()
{
    if ( mpeg_info->has_video ) {
        if ( mpeg_info->video[ 2 ].seen ) {
            return QString( "%1 x %2" ).arg( mpeg_info->video[ 2 ].hsize ).arg( mpeg_info->video[ 2 ].vsize );
        }
    }
    return i18n( "n/a" );
}

QString K3bVcdTrack::video_frate()
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

QString K3bVcdTrack::video_bitrate()
{
    if ( mpeg_info->has_video ) {
        for ( int i = 0; i < 2; i++ ) {
            if ( mpeg_info->video[ i ].seen ) {
                return i18n( "%1 bit/s" , mpeg_info->video[ i ].bitrate ) ;
            }
        }
    }

    return i18n( "n/a" );
}



QString K3bVcdTrack::video_format()
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
                        kDebug() << "K3bVcdTrack::video_format() :" << mpeg_info->video[ i ].video_format;
                        break;
                }
            }
        }
    }
    return i18n( "n/a" );
}

QString K3bVcdTrack::video_chroma()
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

QString K3bVcdTrack::audio_layer()
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

QString K3bVcdTrack::audio_bitrate()
{
    if ( mpeg_info->has_audio ) {
        for ( int i = 0; i < 2; i++ ) {
            if ( mpeg_info->audio[ i ].seen ) {
                return i18n( "%1 bit/s" , mpeg_info->audio[ i ].bitrate ) ;
            }
        }
    }

    return i18n( "n/a" );
}

QString K3bVcdTrack::audio_sampfreq()
{
    if ( mpeg_info->has_audio ) {
        for ( int i = 0; i < 2; i++ ) {
            if ( mpeg_info->audio[ i ].seen ) {
                return i18n( "%1 Hz" , mpeg_info->audio[ i ].sampfreq ) ;
            }
        }
    }

    return i18n( "n/a" );
}

QString K3bVcdTrack::audio_mode( )
{
    if ( mpeg_info->has_audio ) {
        for ( int i = 2; i >= 0; i-- )
            if ( mpeg_info->audio[ i ].seen )
                return QString( audio_type2str( mpeg_info->audio[ i ].version, mpeg_info->audio[ i ].mode, i ) );

    }

    return i18n( "n/a" );
}

QString K3bVcdTrack::audio_copyright( )
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

QString K3bVcdTrack::mpegTypeS( bool audio )
{
    if ( mpeg_info->has_video && !audio ) {
        for ( int i = 0; i < 3; i++ )
            if ( mpeg_info->video[ i ].seen ) {
                if ( i == 0 ) {
                    return QString( "MPEG%1 ").arg(mpeg_info->version ) + i18n( "Motion Picture" );
                } else {
                    return QString( "MPEG%1 ").arg(mpeg_info->version ) + i18n( "Still Picture" );
                }
            }
    }
    if ( mpeg_info->has_audio && audio ) {
        for ( int i = 0; i < 3; i++ )
            if ( mpeg_info->audio[ i ].seen ) {
                return QString( "MPEG%1 ").arg(mpeg_info->audio[ i ].version ) + i18n( "Layer %1" , mpeg_info->audio[ i ].layer );
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

QString K3bVcdTrack::audio_type2str( unsigned int version, unsigned int audio_mode, unsigned int audio_type )
{
    kDebug() << "K3bVcdTrack::audio_type2str() version:" << version << " audio_mode:" << audio_mode << " audio_type:" << audio_type;

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

    kDebug() << "K3bVcdTrack::PrintInfo() .....................";
    kDebug() << "  version          : MPEG" << version();
    kDebug() << "  duration         : " << duration();
    kDebug() << "  muxrate          : " << muxrate();
    kDebug() << "  video ......................................";
    kDebug() << "    type           : " << mpegTypeS();
    kDebug() << "    resolution     : " << resolution();
    kDebug() << "    high resolution: " << highresolution();
    kDebug() << "    frate          : " << video_frate();
    kDebug() << "    bitrate        : " << video_bitrate();
    kDebug() << "    format         : " << video_format( );
    kDebug() << "    chroma         : " << video_chroma( );
    kDebug() << "  audio ......................................";
    kDebug() << "    type           : " << mpegTypeS( true );
    kDebug() << "    mode           : " << audio_mode();
    kDebug() << "    layer          : " << audio_layer();
    kDebug() << "    bitrate        : " << audio_bitrate();
    kDebug() << "    sampfreq       : " << audio_sampfreq();

}
