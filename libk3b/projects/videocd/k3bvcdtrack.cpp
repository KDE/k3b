/*
 *
 * Copyright (C) 2003-2004 Christian Kvasny <chris@k3b.org>
 * Copyright (C) 2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2010 Sebastian Trueg <trueg@k3b.org>
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
#include "k3bglobals.h"

K3b::VcdTrack::VcdTrack( QList<K3b::VcdTrack*>* parent, const QString& filename )
        : m_pbcnumkeys( true ),
        m_pbcnumkeysuserdefined( false ),
        m_file( filename )
{
    m_parent = parent;
    m_title = QFileInfo( m_file ).completeBaseName();
    
    Q_FOREACH( PbcTracks playback, trackPlaybackValues() ) {
        m_pbctrackmap.insert( playback, 0L );
        m_pbcnontrackmap.insert( playback, DISABLED );
        m_pbcusrdefmap.insert( playback, false );
    }

    m_reactivity = false;

    m_definedkeysmap.clear();

    mpeg_info = new Mpeginfo();
}


K3b::VcdTrack::~VcdTrack()
{
}


KIO::filesize_t K3b::VcdTrack::size() const
{
    return m_file.size();
}

int K3b::VcdTrack::index() const
{
    // (trueg): I have no idea why I need to const cast here!
    int i = m_parent->indexOf( const_cast<K3b::VcdTrack*>( this ) );
    if ( i < 0 )
        kDebug() << "(K3b::VcdTrack) I'm not part of my parent!";
    return i;
}


QList<K3b::VcdTrack::PbcTracks> K3b::VcdTrack::trackPlaybackValues()
{
    QList<PbcTracks> playbacks;
    playbacks << PREVIOUS << NEXT << RETURN << DEFAULT << AFTERTIMEOUT;
    return playbacks;
}


void K3b::VcdTrack::addToRevRefList( K3b::VcdTrack* revreftrack )
{
    kDebug() << "K3b::VcdTrack::addToRevRefList: track = " << revreftrack;

    m_revreflist.append( revreftrack );

    kDebug() << "K3b::VcdTrack::hasRevRef count = " << m_revreflist.count() << " empty = " << m_revreflist.isEmpty();
}

void K3b::VcdTrack::delFromRevRefList( K3b::VcdTrack* revreftrack )
{
    m_revreflist.removeAll( revreftrack );
}

bool K3b::VcdTrack::hasRevRef()
{
    return !m_revreflist.isEmpty() ;
}

void K3b::VcdTrack::delRefToUs()
{
    Q_FOREACH( K3b::VcdTrack* track, m_revreflist ) {
        Q_FOREACH( PbcTracks playback, trackPlaybackValues() ) {
            kDebug() << "K3b::VcdTrack::delRefToUs count = " << m_revreflist.count() << " empty = " << m_revreflist.isEmpty() << " track = " << track << " this = " << this;
            if( this == track->getPbcTrack( playback ) ) {
                track->setPbcTrack( playback );
                track->setUserDefined( playback, false );
                track->delFromRevRefList( this );
            }
        }
    }
}

void K3b::VcdTrack::delRefFromUs()
{
    Q_FOREACH( PbcTracks playback, trackPlaybackValues() ) {
        if ( this->getPbcTrack( playback ) ) {
            this->getPbcTrack( playback ) ->delFromRevRefList( this );
        }
    }
}

void K3b::VcdTrack::setPbcTrack( PbcTracks which, K3b::VcdTrack* pbctrack )
{
    kDebug() << "K3b::VcdTrack::setPbcTrack " << which << ", " << pbctrack;
    m_pbctrackmap[which] = pbctrack;
}

void K3b::VcdTrack::setPbcNonTrack( PbcTracks which, PbcTypes type )
{
    kDebug() << "K3b::VcdTrack::setNonPbcTrack " << which << ", " << type;
    m_pbcnontrackmap[which] = type;
}

void K3b::VcdTrack::setUserDefined( PbcTracks which, bool ud )
{
    m_pbcusrdefmap[which] = ud;
}

K3b::VcdTrack* K3b::VcdTrack::getPbcTrack( PbcTracks which )
{
    if ( m_pbctrackmap.find( which ) == m_pbctrackmap.end() )
        return 0;
    else
        return m_pbctrackmap[ which ];
}

int K3b::VcdTrack::getNonPbcTrack( PbcTracks which )
{
    if ( m_pbcnontrackmap.find( which ) == m_pbcnontrackmap.end() )
        return 0;
    else
        return m_pbcnontrackmap[ which ];
}

bool K3b::VcdTrack::isPbcUserDefined( PbcTracks which )
{
    return m_pbcusrdefmap[ which ];
}

QString K3b::VcdTrack::resolution()
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

QString K3b::VcdTrack::highresolution()
{
    if ( mpeg_info->has_video ) {
        if ( mpeg_info->video[ 2 ].seen ) {
            return QString( "%1 x %2" ).arg( mpeg_info->video[ 2 ].hsize ).arg( mpeg_info->video[ 2 ].vsize );
        }
    }
    return i18n( "n/a" );
}

QString K3b::VcdTrack::video_frate()
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

QString K3b::VcdTrack::video_bitrate()
{
    if ( mpeg_info->has_video ) {
        for ( int i = 0; i < 2; i++ ) {
            if ( mpeg_info->video[ i ].seen ) {
                return i18np( "1 bit/s", "%1 bits/s" , mpeg_info->video[ i ].bitrate ) ;
            }
        }
    }

    return i18n( "n/a" );
}



QString K3b::VcdTrack::video_format()
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
                        kDebug() << "K3b::VcdTrack::video_format() :" << mpeg_info->video[ i ].video_format;
                        break;
                }
            }
        }
    }
    return i18n( "n/a" );
}

QString K3b::VcdTrack::video_chroma()
{
    if ( mpeg_info->has_video ) {
        // MPEG1 only supports 4:2:0 Format
        if ( version() == K3b::MpegInfo::MPEG_VERS_MPEG1 )
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

QString K3b::VcdTrack::audio_layer()
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

QString K3b::VcdTrack::audio_bitrate()
{
    if ( mpeg_info->has_audio ) {
        for ( int i = 0; i < 2; i++ ) {
            if ( mpeg_info->audio[ i ].seen ) {
                return i18np( "1 bit/s", "%1 bits/s" , mpeg_info->audio[ i ].bitrate ) ;
            }
        }
    }

    return i18n( "n/a" );
}

QString K3b::VcdTrack::audio_sampfreq()
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

QString K3b::VcdTrack::audio_mode( )
{
    if ( mpeg_info->has_audio ) {
        for ( int i = 2; i >= 0; i-- )
            if ( mpeg_info->audio[ i ].seen )
                return QString( audio_type2str( mpeg_info->audio[ i ].version, mpeg_info->audio[ i ].mode, i ) );

    }

    return i18n( "n/a" );
}

QString K3b::VcdTrack::audio_copyright( )
{
    if ( mpeg_info->has_audio ) {
        for ( int i = 2; i >= 0; i-- ) {
            if ( mpeg_info->audio[ i ].seen )
            {
                if ( mpeg_info->audio[ i ].copyright )
                    return QString( "(c) " ) + ( mpeg_info->audio[ i ].original ? i18n( "original" ) : i18n( "duplicate" ) );
                else
                    return ( mpeg_info->audio[ i ].original ? i18n( "original" ) : i18n( "duplicate" ) );
            }
        }
    }

    return i18n( "n/a" );
}

QString K3b::VcdTrack::mpegTypeS( bool audio )
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

int K3b::VcdTrack::mpegType( )
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

QString K3b::VcdTrack::audio_type2str( unsigned int version, unsigned int audio_mode, unsigned int audio_type )
{
    kDebug() << "K3b::VcdTrack::audio_type2str() version:" << version << " audio_mode:" << audio_mode << " audio_type:" << audio_type;

    QString audio_types[ 3 ][ 5 ] = {
                                        {
                                            i18n( "unknown" ),
                                            i18n( "invalid" ),
                                            QString(),
                                            QString(),
                                            QString()
                                        },
                                        {
                                            i18n( "stereo" ),
                                            i18n( "joint stereo" ),
                                            i18n( "dual channel" ),
                                            i18n( "single channel" )
                                        },
                                        {
                                            QString(),
                                            i18n( "dual channel" ),
                                            i18n( "surround sound" ),
                                            QString(),
                                            QString()
                                        }
                                    };
    switch ( version ) {
        case K3b::MpegInfo::MPEG_VERS_MPEG1:
            return audio_types[ 1 ][ audio_mode ];
            break;

        case K3b::MpegInfo::MPEG_VERS_MPEG2:
            if ( audio_type > 0 ) {
                return audio_types[ 2 ][ audio_type ];
            }
            return audio_types[ 1 ][ audio_mode ];
            break;
    }

    return i18n( "n/a" );
}

// convert a time in second to HH:mm:ss notation
QString K3b::VcdTrack::SecsToHMS( double duration )
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

void K3b::VcdTrack::PrintInfo()
{

    kDebug() << "K3b::VcdTrack::PrintInfo() .....................";
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
