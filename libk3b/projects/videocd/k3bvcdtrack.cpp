/*
*
* $Id$
* Copyright (C) 2003-2004 Christian Kvasny <chris@k3b.org>
*
* This file is part of the K3b project.
* Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
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

K3bVcdTrack::K3bVcdTrack( QPtrList<K3bVcdTrack>* parent, const QString& filename )
        : m_file( filename )
{
    m_parent = parent;
    m_title = QFileInfo( m_file ).baseName( true );

    m_revreflist = new QPtrList<K3bVcdTrack>;

    for ( int i = 0; i < K3bVcdTrack::_maxPbcTracks; i++ ) {
        m_pbctrackmap.insert( i, 0L );
        m_pbcnontrackmap.insert( i, K3bVcdTrack::DISABLED );
        m_pbcusrdefmap.insert( i, false );
    }

    m_segment = false;
    m_reactivity = false;

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
                return QString( " %1 x %2" ).arg( mpeg_info->video[ i ].hsize ).arg( mpeg_info->video[ i ].vsize );
            }
        }
    }

    return " " + i18n( "n/a" );
}

const QString K3bVcdTrack::highresolution()
{
    if ( mpeg_info->has_video ) {
        if ( mpeg_info->video[ 2 ].seen ) {
            return QString( " %1 x %2" ).arg( mpeg_info->video[ 2 ].hsize ).arg( mpeg_info->video[ 2 ].vsize );
        }
    }
    return " " + i18n( "n/a" );
}

const QString K3bVcdTrack::video_frate()
{
    if ( mpeg_info->has_video ) {
        for ( int i = 2; i >= 0; i-- ) {
            if ( mpeg_info->video[ i ].seen ) {
                return QString::number( mpeg_info->video[ i ].frate );
            }
        }
    }

    return " " + i18n( "n/a" );
}

const QString K3bVcdTrack::video_bitrate()
{
    if ( mpeg_info->has_video ) {
        for ( int i = 2; i >= 0; i-- ) {
            if ( mpeg_info->video[ i ].seen ) {
                return QString::number( mpeg_info->video[ i ].bitrate ) ;
            }
        }
    }

    return " " + i18n( "n/a" );
}

const QString K3bVcdTrack::audio_layer()
{
    if ( mpeg_info->has_audio ) {
        for ( int i = 2; i >= 0; i-- ) {
            if ( mpeg_info->audio[ i ].seen ) {
                return QString::number( mpeg_info->audio[ i ].layer );
            }
        }
    }

    return " " + i18n( "n/a" );
}

const QString K3bVcdTrack::audio_bitrate()
{
    if ( mpeg_info->has_audio ) {
        for ( int i = 2; i >= 0; i-- ) {
            if ( mpeg_info->audio[ i ].seen ) {
                return QString::number( mpeg_info->audio[ i ].bitrate ) ;
            }
        }
    }

    return " " + i18n( "n/a" );
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
                if ( mpeg_info->audio[ i ].version == MPEG_VERS_MPEG2 ) {
                    if ( mpeg_info->audio[ i ].seen ) {
                        return "MPEG2 " + QString( i18n( "Layer %1" ) ).arg( mpeg_info->audio[ i ].layer ) + QString( audio_type2str( MPEG_VERS_MPEG2, i + 1 ) );
                    }
                } else
                    switch ( mpeg_info->audio[ i ].mode ) {
                            case MPEG_SINGLE_CHANNEL:
                            return "MPEG1 " + QString( i18n( "Layer %1" ) ).arg( mpeg_info->audio[ i ].layer ) + QString( audio_type2str( MPEG_VERS_MPEG1, 1 ) );
                            break;
                            case MPEG_STEREO:
                            case MPEG_JOINT_STEREO:
                            return "MPEG1 " + QString( i18n( "Layer %1" ) ).arg( mpeg_info->audio[ i ].layer ) + QString( audio_type2str( MPEG_VERS_MPEG1, 2 ) );
                            break;
                            case MPEG_DUAL_CHANNEL:
                            return "MPEG1 " + QString( i18n( "Layer %1" ) ).arg( mpeg_info->audio[ i ].layer ) + QString( audio_type2str( MPEG_VERS_MPEG1, 3 ) );
                            break;
                    }
            }
    }

    return " " + i18n( "n/a" );
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

const QString K3bVcdTrack::audio_type2str( unsigned int version, unsigned int audio_type )
{
    const QString audio_types[ 3 ][ 5 ] = {
                                              /* INVALID, VCD 1.0, or VCD 1.1 */
                                              { i18n( "unknown" ), i18n( "invalid" ), "", "", "" },

                                              /*VCD 2.0 */
                                              { i18n( "no audio" ), i18n( "single channel" ), i18n( "stereo" ), i18n( "dual channel" ), i18n( "error" ) },

                                              /* SVCD, HQVCD */
                                              { i18n( "no stream" ), i18n( "1 stream" ), i18n( "2 streams" ),
                                                i18n( "1 multi-channel stream (surround sound)" ), i18n( "error" ) },
                                          };

    unsigned int first_index = 0;

    /* Get first index entry into above audio_type array from vcd_type */
    switch ( version ) {

            case MPEG_VERS_MPEG1:
            first_index = 1;
            break;

            case MPEG_VERS_MPEG2:
            first_index = 2;
            break;
            case MPEG_VERS_INVALID:
            default:
            audio_type = 4;
    }

    /* We should also check that the second index is in range too. */
    if ( audio_type > 3 ) {
        first_index = 0;
        audio_type = 1;
    }

    return audio_types[ first_index ][ audio_type ];
}

// convert a time in second to HH:mm:ss notation
QString K3bVcdTrack::SecsToHMS( double duration )
{
    byte hours = ( byte ) ( duration / 3600 );
    byte mins = ( byte ) ( ( duration / 60 ) - ( hours * 60 ) );
    float secs = duration - 60 * mins - 3600 * hours;
    if ( hours != 0 ) {
        return QString( "%1:" ).arg( hours ).rightJustify( 3, ' ' ) + QString( "%1:" ).arg( mins ).rightJustify( 3, '0' ) + QString::number( secs, 'f', 2 );
    }
    if ( mins != 0 ) {
        return QString( "%1:" ).arg( mins ).rightJustify( 3, '0' ) + QString::number( secs, 'f', 2 );
    }
    return QString::number( secs, 'f', 2 );
}

void K3bVcdTrack::PrintInfo()
{
    kdDebug() << "K3bVcdTrack::PrintInfo()" << endl;

    if ( mpeg_info->has_video ) {
        kdDebug() << "Video Info:" << endl;
        for ( int i = 0; i < 3; i++ )
            if ( mpeg_info->video[ i ].seen ) {
                if ( i == 0 ) {
                    kdDebug() << QString( "Motion Video: MPEG%1" ).arg( mpeg_info->version ) << endl;
                } else {
                    kdDebug() << QString( "Still Video: MPEG%1" ).arg( mpeg_info->version ) << endl;
                }

                if ( i == 1 )
                    kdDebug() << "Low Resolution Picture" << endl;
                else if ( i == 2 )
                    kdDebug() << "High Resolution Picture" << endl;

                kdDebug() << QString( "hsize: %1" ).arg( mpeg_info->video[ i ].hsize ) << endl;
                kdDebug() << QString( "vsize: %1" ).arg( mpeg_info->video[ i ].vsize ) << endl;
                kdDebug() << QString( "aratio: %1" ).arg( mpeg_info->video[ i ].aratio ) << endl;
                kdDebug() << QString( "frate: %1" ).arg( mpeg_info->video[ i ].frate ) << endl;
                kdDebug() << QString( "bitrate: %1" ).arg( mpeg_info->video[ i ].bitrate ) << endl;
                kdDebug() << QString( "vbvsize: %1" ).arg( mpeg_info->video[ i ].vbvsize ) << endl;
                kdDebug() << QString( "constrained: %1" ).arg( mpeg_info->video[ i ].constrained_flag ) << endl;
            }
    }
    if ( mpeg_info->has_audio ) {

        kdDebug() << "Audio Info:" << endl;
        for ( int i = 0; i < 3; i++ )
            if ( mpeg_info->audio[ i ].seen ) {
                kdDebug() << QString( "Audio # %1" ).arg( i ) << endl;
                kdDebug() << QString( "version: %1" ).arg( mpeg_info->audio[ i ].version ) << endl;
                kdDebug() << QString( "layer: %1" ).arg( mpeg_info->audio[ i ].layer ) << endl;
                kdDebug() << QString( "bitrate: %1" ).arg( mpeg_info->audio[ i ].bitrate ) << endl;
                kdDebug() << QString( "byterate: %1" ).arg( mpeg_info->audio[ i ].byterate ) << endl;
                kdDebug() << QString( "sampfreq: %1" ).arg( mpeg_info->audio[ i ].sampfreq ) << endl;

                if ( mpeg_info->audio[ i ].version == MPEG_VERS_MPEG2 ) {
                    if ( mpeg_info->audio[ i ].seen ) {
                        kdDebug() << QString( "mode: %1" ).arg( audio_type2str( MPEG_VERS_MPEG2, i + 1 ) ) << endl;
                    }
                } else
                    switch ( mpeg_info->audio[ i ].mode ) {
                            case MPEG_SINGLE_CHANNEL:
                            kdDebug() << QString( "mode: %1" ).arg( audio_type2str( MPEG_VERS_MPEG1, 1 ) ) << endl;
                            break;
                            case MPEG_STEREO:
                            case MPEG_JOINT_STEREO:
                            kdDebug() << QString( "mode: %1" ).arg( audio_type2str( MPEG_VERS_MPEG1, 2 ) ) << endl;
                            break;
                            case MPEG_DUAL_CHANNEL:
                            kdDebug() << QString( "mode: %1" ).arg( audio_type2str( MPEG_VERS_MPEG1, 3 ) ) << endl;
                            break;
                    }
            }
    }
}
