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

// kde includes
#include <klocale.h>

// k3b includes
#include "k3bmpeginfo.h"

#ifdef Q_OS_WIN32
#define ftello ftell
#define fseeko fseek
#endif

static const double frame_rates[ 16 ] =
{
    0.0, 24000.0 / 1001, 24.0, 25.0,
    30000.0 / 1001, 30.0, 50.0, 60000.0 / 1001,
    60.0, 0.0,
};

K3b::MpegInfo::MpegInfo( const char* filename )
    : m_mpegfile( 0 ),
      m_filename( filename ),
      m_done( false ),
      m_buffstart( 0 ),
      m_buffend( 0 ),
      m_buffer( 0 ),
      m_initial_TS( 0.0 )
{

    mpeg_info = new Mpeginfo();

    m_mpegfile = fopen( filename, "rb" );

    if ( m_mpegfile == 0 ) {
        kDebug() << QString( "Unable to open %1" ).arg( m_filename );
        return ;
    }

    if ( fseeko( m_mpegfile, 0, SEEK_END ) ) {
        kDebug() << QString( "Unable to seek in file %1" ).arg( m_filename );
        return ;
    }

    llong lof = ftello( m_mpegfile );

    if ( lof == -1 ) {
        kDebug() << QString( "Seeking to end of input file %1 failed." ).arg( m_filename );
        //give up..
        return ;
    } else
        m_filesize = lof;

    // nothing to do on an empty file
    if ( !m_filesize ) {
        kDebug() << QString( "File %1 is empty." ).arg( m_filename );
        m_error_string = i18n( "File %1 is empty." , m_filename );
        return ;
    }

    m_buffer = new byte[ BUFFERSIZE ];

    MpegParsePacket ( );

}

K3b::MpegInfo::~MpegInfo()
{
    if ( m_buffer ) {
        delete[] m_buffer;
    }
    if ( m_mpegfile ) {
        fclose( m_mpegfile );
    }

    delete mpeg_info;
}


bool K3b::MpegInfo::MpegParsePacket ()
{

    /* verify the packet begins with a pack header */
    if ( !EnsureMPEG( 0, MPEG_PACK_HEADER_CODE ) ) {
        llong code = GetNBytes( 0, 4 );

        kDebug() << QString( "(K3b::MpegInfo::mpeg_parse_packet ()) pack header code 0x%1 expected, but 0x%2 found" ).arg( 0x00000100 + MPEG_PACK_HEADER_CODE, 0, 16 ).arg( code, 0, 16 );

        if ( code == 0x00000100 + MPEG_SEQUENCE_CODE ) {
            kDebug() << "...this looks like an elementary video stream but a multiplexed program stream was required.";
            m_error_string = i18n( "This looks like an elementary video stream but a multiplexed program stream was required." );
        }

        if ( ( 0xfff00000 & code ) == 0xfff00000 ) {
            kDebug() << "...this looks like an elementary audio stream but a multiplexed program stream was required.";
            m_error_string = i18n( "This looks like an elementary audio stream but a multiplexed program stream was required." );
        }

        if ( code == 0x52494646 ) {
            kDebug() << "...this looks like a RIFF header but a plain multiplexed program stream was required.";
            m_error_string = i18n( "This looks like a RIFF header but a plain multiplexed program stream was required." );
        }

        return false;
    }


    /* take a look at the pack header */
    int offset = 0;
    while ( GetByte( offset ) == 0x00 )
        offset ++;
    //here we're on the first non null byte let's get back to leave two zeros (packet start code)
    offset -= 2;

    if ( offset != 0 ) {
        // we actually skipped some zeroes
        kDebug() << QString( "Skipped %1 zeroes at start of file" ).arg( offset );
    }

    // here while schleife
    while ( offset != -1 ) {
        offset = MpegParsePacket( offset );
    }

    /*
      int pkt = 0;
      offset = FindNextMarker( 0, MPEG_PACK_HEADER_CODE );

      while ( offset != -1 ) {
      pkt++;
      offset = FindNextMarker( offset+1, MPEG_PACK_HEADER_CODE );
      }

      kDebug() << "Pkt found: " << pkt;
    */

    //seek the file duration by fetching the last PACK
    //and reading its timestamp
    llong last_pack = bdFindNextMarker( m_filesize - 13, MPEG_PACK_HEADER_CODE );
    // -12 because a PACK is at least 12 bytes
    double duration;
    last_pack += 4;
    int bits = GetByte( last_pack ) >> 4;

    if ( bits == 0x2 )                /* %0010 ISO11172-1 */
    {
        duration = ReadTS( last_pack );
    } else if ( bits >> 2 == 0x1 )                /* %01xx ISO13818-1 */
    {
        duration = ReadTSMpeg2( last_pack );
    } else {
        kDebug() << QString( "no timestamp found" );
        duration = ReadTS( last_pack );
    }

    mpeg_info->playing_time = duration - m_initial_TS;


    if ( !mpeg_info->has_video )
        for ( int i = 0; i < 2; i++ )
            if ( mpeg_info->video[ i ].seen )
                mpeg_info->has_video = true;

    if ( !mpeg_info->has_audio )
        for ( int i = 0; i < 2; i++ )
            if ( mpeg_info->audio[ i ].seen )
                mpeg_info->has_audio = true;

    return true;
}

llong K3b::MpegInfo::MpegParsePacket ( llong offset )
{
    byte mark = 0;
    uint size = 0;

    /* continue until start code seen */
    offset = FindNextMarker( offset, &mark );

    if ( offset < 0 )
        return offset;

    switch ( mark ) {
        int bits;

    case MPEG_PACK_HEADER_CODE:
        // kDebug() << QString( "MPEG_PACK_HEADER_CODE @ %1" ).arg( offset );

        offset += 4;

        if ( mpeg_info->version != MPEG_VERS_INVALID )
            break;

        bits = GetByte( offset ) >> 4;

        if ( bits == 0x2 )                /* %0010 ISO11172-1 */
        {
            mpeg_info->version = MPEG_VERS_MPEG1;

            unsigned long muxrate = 0;

            muxrate = ( GetByte( offset + 5 ) & 0x7F ) << 15;
            muxrate |= ( GetByte( offset + 6 ) << 7 );
            muxrate |= ( GetByte( offset + 7 ) >> 1 );

            mpeg_info->muxrate = muxrate * 50 * 8;

            if ( m_initial_TS == 0.0 )
            {
                m_initial_TS = ReadTS( offset );
                kDebug() << QString( "Initial TS = %1" ).arg( m_initial_TS );
            }

        } else if ( bits >> 2 == 0x1 )                /* %01xx ISO13818-1 */
        {
            mpeg_info->version = MPEG_VERS_MPEG2;

            unsigned long muxrate = 0;
            muxrate = GetByte( offset + 6 ) << 14;
            muxrate |= GetByte( offset + 7 ) << 6;
            muxrate |= GetByte( offset + 8 ) >> 2;

            mpeg_info->muxrate = muxrate * 50 * 8;

            if ( m_initial_TS == 0.0 )
            {
                m_initial_TS = ReadTSMpeg2( offset );
                kDebug() << QString( "Initial TS = %1" ).arg( m_initial_TS );
            }

        } else {
            kDebug() << QString( "packet not recognized as either version 1 or 2 (%1)" ).arg( bits );
            mpeg_info->version = MPEG_VERS_INVALID;
            return -1;
        }
        break;

    case MPEG_SYSTEM_HEADER_CODE:
    case MPEG_PAD_CODE:
    case MPEG_PRIVATE_1_CODE:
    case MPEG_VIDEO_E0_CODE:
    case MPEG_VIDEO_E1_CODE:
    case MPEG_VIDEO_E2_CODE:
    case MPEG_AUDIO_C0_CODE:
    case MPEG_AUDIO_C1_CODE:
    case MPEG_AUDIO_C2_CODE:

        offset += 4;
        size = GetSize( offset );
        offset += 2;
        // kDebug() << QString( "offset = %1, size = %2" ).arg( offset ).arg( size );

        switch ( mark ) {
        case MPEG_SYSTEM_HEADER_CODE:
            // kDebug() << QString( "Systemheader: %1" ).arg( m_code, 0, 16 );
            break;

        case MPEG_VIDEO_E0_CODE:
        case MPEG_VIDEO_E1_CODE:
        case MPEG_VIDEO_E2_CODE:
            ParseVideo( offset, mark );
            // _analyze_video_pes (code & 0xff, buf + pos, size, !parse_pes, ctx);
            if ( mpeg_info->has_video && mpeg_info->has_audio ) {
                return -1;
            } else if ( mark == MPEG_VIDEO_E0_CODE || (mpeg_info->version == MPEG_VERS_MPEG2 && mark == MPEG_VIDEO_E1_CODE) || (mpeg_info->version == MPEG_VERS_MPEG1 && mark == MPEG_VIDEO_E2_CODE) ) {
                mpeg_info->has_video = true;
                offset = FindNextAudio( offset );
            }
            break;
        case MPEG_AUDIO_C0_CODE:
        case MPEG_AUDIO_C1_CODE:
        case MPEG_AUDIO_C2_CODE:
            offset = SkipPacketHeader( offset - 6 );
            ParseAudio( offset, mark );
            // audio packet doesn't begin with 0xFFF
            if ( !mpeg_info->audio[ GetAudioIdx( mark ) ].seen ) {
                int a_idx = GetAudioIdx( mark );
                while ( ( offset < m_filesize - 10 ) && !mpeg_info->audio[ a_idx ].seen ) {
                    if ( ( GetByte( offset ) == 0xFF ) && ( GetByte( offset + 1 ) & 0xF0 ) == 0xF0 )
                        ParseAudio( offset, mark );
                    offset++;
                }
            }

            mpeg_info->has_audio = true;
            if ( mpeg_info->has_video )
                return -1;

            offset = FindNextVideo( offset );
            break;

        case MPEG_PRIVATE_1_CODE:
            kDebug() << QString( "PrivateCode: %1" ).arg( mark, 0, 16 );
            break;
        }
        break;

    case MPEG_PROGRAM_END_CODE:
        kDebug() << QString( "ProgramEndCode: %1" ).arg( mark, 0, 16 );
        offset += 4;
        break;

    case MPEG_PICTURE_CODE:
        kDebug() << QString( "PictureCode: %1" ).arg( mark, 0, 16 );
        offset += 3;
        break;

    default:
        offset += 4;
        break;
    }

    return offset;
}

byte K3b::MpegInfo::GetByte( llong offset )
{
    unsigned long nread;
    if ( ( offset >= m_buffend ) || ( offset < m_buffstart ) ) {

        if ( fseeko( m_mpegfile, offset, SEEK_SET ) ) {
            kDebug() << QString( "could not get seek to offset (%1) in file %2 (size:%3)" ).arg( offset ).arg( m_filename ).arg( m_filesize );
            return 0x11;
        }
        nread = fread( m_buffer, 1, BUFFERSIZE, m_mpegfile );
        m_buffstart = offset;
        m_buffend = offset + nread;
        if ( ( offset >= m_buffend ) || ( offset < m_buffstart ) ) {
            // weird
            kDebug() << QString( "could not get offset %1 in file %2 [%3]" ).arg( offset ).arg( m_filename ).arg( m_filesize );
            return 0x11;
        }
    }
    return m_buffer[ offset - m_buffstart ];
}

// same as above but improved for backward search
byte K3b::MpegInfo::bdGetByte( llong offset )
{
    unsigned long nread;
    if ( ( offset >= m_buffend ) || ( offset < m_buffstart ) ) {
        llong start = offset - BUFFERSIZE + 1 ;
        start = start >= 0 ? start : 0;

        fseeko( m_mpegfile, start, SEEK_SET );

        nread = fread( m_buffer, 1, BUFFERSIZE, m_mpegfile );
        m_buffstart = start;
        m_buffend = start + nread;
        if ( ( offset >= m_buffend ) || ( offset < m_buffstart ) ) {
            // weird
            kDebug() << QString( "could not get offset %1 in file %2 [%3]" ).arg( offset ).arg( m_filename ).arg( m_filesize );

            return 0x11;
        }
    }
    return m_buffer[ offset - m_buffstart ];
}


llong K3b::MpegInfo::GetNBytes( llong offset, int n )
{
    llong nbytes = 0;
    n--;
    for ( int i = 0; i < n; i++ )
        ( ( char* ) & nbytes ) [ n - i ] = GetByte( offset + i );

    return nbytes;

}

// get a two byte size
unsigned short int K3b::MpegInfo::GetSize( llong offset )
{
    return GetByte( offset ) * 256 + GetByte( offset + 1 );
    // return GetNBytes( offset, 2 );

}

bool K3b::MpegInfo::EnsureMPEG( llong offset, byte mark )
{
    if ( ( GetByte( offset ) == 0x00 ) &&
         ( GetByte( offset + 1 ) == 0x00 ) &&
         ( GetByte( offset + 2 ) == 0x01 ) &&
         ( GetByte( offset + 3 ) == mark ) )
        return true;
    else
        return false;
}


// find next 0x 00 00 01 xx sequence, returns offset or -1 on err
llong K3b::MpegInfo::FindNextMarker( llong from )
{
    llong offset;
    for ( offset = from; offset < ( m_filesize - 4 ); offset++ ) {
        if (
            ( GetByte( offset + 0 ) == 0x00 ) &&
            ( GetByte( offset + 1 ) == 0x00 ) &&
            ( GetByte( offset + 2 ) == 0x01 ) ) {
            return offset;
        }
    }
    return -1;
}

// find next 0x 00 00 01 xx sequence, returns offset or -1 on err and
// change mark to xx
llong K3b::MpegInfo::FindNextMarker( llong from, byte* mark )
{
    llong offset = FindNextMarker( from );
    if ( offset >= 0 ) {
        *mark = GetByte( offset + 3 );
        return offset;
    } else {
        return -1;
    }
}

// find next 0X00 00 01 mark
llong K3b::MpegInfo::FindNextMarker( llong from, byte mark )
{
    llong offset = from;
    while ( offset >= 0 ) {
        offset = FindNextMarker( offset );
        if ( offset < 0 ) {
            return -1;
        }
        if ( EnsureMPEG( offset, mark ) ) {
            return offset;
        } else
            offset++;
    }

    //shouldn't be here
    return -1;
}

llong K3b::MpegInfo::bdFindNextMarker( llong from, byte mark )
{
    llong offset;
    for ( offset = from; offset >= 0; offset-- ) {
        if (
            ( bdGetByte( offset ) == 0x00 ) &&
            ( bdGetByte( offset + 1 ) == 0x00 ) &&
            ( bdGetByte( offset + 2 ) == 0x01 ) &&
            ( bdGetByte( offset + 3 ) == mark ) ) {
            return offset;
        }
    }
    return -1;
}

llong K3b::MpegInfo::bdFindNextMarker( llong from, byte* mark )
{
    llong offset;
    for ( offset = from; offset >= 0; offset-- ) {
        if ( ( bdGetByte( offset ) == 0x00 ) &&
             ( bdGetByte( offset + 1 ) == 0x00 ) &&
             ( bdGetByte( offset + 2 ) == 0x01 ) ) {
            *mark = bdGetByte( offset + 3 );
            return offset;
        }
    }
    return -1;

}

llong K3b::MpegInfo::FindNextVideo( llong from )
{
    llong offset = from;
    while ( offset >= 0 ) {
        offset = FindNextMarker( offset );
        if ( offset < 0 ) {
            return -1;
        }
        if ( EnsureMPEG( offset, MPEG_VIDEO_E0_CODE ) || EnsureMPEG( offset, MPEG_VIDEO_E1_CODE ) || EnsureMPEG( offset, MPEG_VIDEO_E2_CODE ) ) {
            return offset;
        } else
            offset++;
    }

    //shouldn't be here
    return -1;
}

llong K3b::MpegInfo::FindNextAudio( llong from )
{
    llong offset = from;
    while ( offset >= 0 ) {
        offset = FindNextMarker( offset );
        if ( offset < 0 ) {
            return -1;
        }
        if ( EnsureMPEG( offset, MPEG_AUDIO_C0_CODE ) || EnsureMPEG( offset, MPEG_AUDIO_C1_CODE ) || EnsureMPEG( offset, MPEG_AUDIO_C2_CODE ) ) {
            return offset;
        } else
            offset++;
    }

    return -1;
}


int K3b::MpegInfo::GetVideoIdx ( byte marker )
{
    switch ( marker ) {
    case MPEG_VIDEO_E0_CODE:
        return 0;
        break;

    case MPEG_VIDEO_E1_CODE:
        return 1;
        break;

    case MPEG_VIDEO_E2_CODE:
        return 2;
        break;

    default:
        kDebug() << "VideoCode not reached";
        break;
    }

    return -1;
}

int K3b::MpegInfo::GetAudioIdx ( byte marker )
{
    switch ( marker ) {
    case MPEG_AUDIO_C0_CODE:
        return 0;
        break;

    case MPEG_AUDIO_C1_CODE:
        return 1;
        break;

    case MPEG_AUDIO_C2_CODE:
        return 2;
        break;

    default:
        kDebug() << "VideoCode not reached";
        break;
    }

    return -1;
}

llong K3b::MpegInfo::SkipPacketHeader( llong offset )
{
    byte tmp_byte;
    if ( mpeg_info->version == MPEG_VERS_MPEG1 ) {
        // skip startcode and packet size
        offset += 6;
        //remove stuffing bytes
        tmp_byte = GetByte( offset );
        while ( tmp_byte & 0x80 )
            tmp_byte = GetByte( ++offset );

        if ( ( tmp_byte & 0xC0 ) == 0x40 )                // next two bits are 01
            offset += 2;

        tmp_byte = GetByte( offset );
        if ( ( tmp_byte & 0xF0 ) == 0x20 )
            offset += 5;
        else if ( ( tmp_byte & 0xF0 ) == 0x30 )
            offset += 10;
        else
            offset++;

        return offset;
    } else if ( mpeg_info->version == MPEG_VERS_MPEG2 ) {
        return ( offset + 9 + GetByte( offset + 8 ) );
    } else
        return ( offset + 10 );
}

void K3b::MpegInfo::ParseAudio ( llong offset, byte marker )
{
    unsigned brate, srate;
    bool mpeg2_5 = false;

    const int a_idx = GetAudioIdx( marker );

    if ( mpeg_info->audio[ a_idx ].seen )                /* we have it already */
        return ;

    if ( ( GetByte( offset ) != 0xFF ) || ( ( GetByte( offset + 1 ) & 0xF0 ) != 0xF0 ) ) {
        // doesn't start with 12 bits set
        if ( ( GetByte( offset ) != 0xFF ) || ( ( GetByte( offset + 1 ) & 0xE0 ) != 0xE0 ) ) {
            // doesn't start with 11 bits set
            return ;
        } else {
            // start with 11 bits set
            mpeg2_5 = true;
        }
    }

    // Find mpeg version 1.0 or 2.0
    if ( GetByte( offset + 1 ) & 0x08 ) {
        if ( !mpeg2_5 )
            mpeg_info->audio[ a_idx ].version = 1;
        else
            return ; // invalid 01 encountered
    } else {
        if ( !mpeg2_5 )
            mpeg_info->audio[ a_idx ].version = 2;
        else
            mpeg_info->audio[ a_idx ].version = 3; //for mpeg 2.5
    }

    // Find Layer
    mpeg_info->audio[ a_idx ].layer = ( GetByte( offset + 1 ) & 0x06 ) >> 1;
    switch ( mpeg_info->audio[ a_idx ].layer ) {
    case 0:
        mpeg_info->audio[ a_idx ].layer = 0;
        break;
    case 1:
        mpeg_info->audio[ a_idx ].layer = 3;
        break;
    case 2:
        mpeg_info->audio[ a_idx ].layer = 2;
        break;
    case 3:
        mpeg_info->audio[ a_idx ].layer = 1;
        break;
    }

    // Protection Bit
    mpeg_info->audio[ a_idx ].protect = GetByte( offset + 1 ) & 0x01;
    if ( mpeg_info->audio[ a_idx ].protect )
        mpeg_info->audio[ a_idx ].protect = 0;
    else
        mpeg_info->audio[ a_idx ].protect = 1;

    const unsigned bit_rates[ 4 ][ 16 ] = {
        {
            0,
        },
        {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0},
        {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 0},
        {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0}
    };


    /*  const unsigned bit_rates [ 3 ][ 3 ][ 16 ] = {
        {
        {0, },
        {0, },
        {0, },
        },
        {
        {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0},
        {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384, 0},
        {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0}
        },
        {
        {0, 32, 48, 56, 64, 80 , 96 , 112, 128, 144, 160, 176, 192, 224, 256, 0},
        {0, 8, 16, 24, 32, 40, 48, 56, 64 , 80 , 96 , 112, 128, 144, 160, 0},
        {0, 8, 16, 24, 32, 40, 48, 56, 64 , 80 , 96 , 112, 128, 144, 160, 0}
        }
        };
    */

    const unsigned sampling_rates[ 4 ][ 4 ] = {
        {
            0,
        },
        {44100, 48000, 32000, 0},                //mpeg 1
        {22050, 24000, 16000, 0},                //mpeg 2
        {11025, 12000, 8000, 0}   //mpeg 2.5
    };


    // Bitrate index and sampling index to pass through the array
    brate = GetByte( offset + 2 ) >> 4;
    srate = ( GetByte( offset + 2 ) & 0x0f ) >> 2;

    mpeg_info->audio[ a_idx ].bitrate = 1024 * bit_rates[ mpeg_info->audio[ a_idx ].layer ][ brate ];
    mpeg_info->audio[ a_idx ].byterate = ( float ) ( mpeg_info->audio[ a_idx ].bitrate / 8.0 );
    mpeg_info->audio[ a_idx ].sampfreq = sampling_rates[ mpeg_info->audio[ a_idx ].version ][ srate ];

    // Audio mode
    mpeg_info->audio[ a_idx ].mode = 1 + ( GetByte( offset + 3 ) >> 6 ) ;

    // Copyright bit
    if ( GetByte( offset + 3 ) & 0x08 )
        mpeg_info->audio[ a_idx ].copyright = true;
    else
        mpeg_info->audio[ a_idx ].copyright = false;

    // Original/Copy bit
    if ( GetByte( offset + 3 ) & 0x04 )
        mpeg_info->audio[ a_idx ].original = true;
    else
        mpeg_info->audio[ a_idx ].original = false;


    mpeg_info->audio[ a_idx ].seen = true;
}

void K3b::MpegInfo::ParseVideo ( llong offset, byte marker )
{
    unsigned long aratio, frate, brate;

    const int v_idx = GetVideoIdx( marker );

    const double aspect_ratios[ 16 ] = {
        0.0000, 1.0000, 0.6735, 0.7031,
        0.7615, 0.8055, 0.8437, 0.8935,
        0.9375, 0.9815, 1.0255, 1.0695,
        1.1250, 1.1575, 1.2015, 0.0000
    };

    if ( mpeg_info->video[ v_idx ].seen )                /* we have it already */
        return ;

    offset = FindNextMarker( offset + 1, MPEG_SEQUENCE_CODE );

    if ( !offset )
        return ;

    offset += 4;

    mpeg_info->video[ v_idx ].hsize = GetSize( offset ) >> 4;
    mpeg_info->video[ v_idx ].vsize = GetSize( offset + 1 ) & 0x0FFF;

    // Get picture rate
    offset += 3; // after picture sizes

    aratio = ( GetByte( offset ) & 0x0F ) >> 4;
    mpeg_info->video[ v_idx ].aratio = aspect_ratios[ aratio ];

    //    offset += 3; // after picture sizes
    frate = GetByte( offset ) & 0x0F;
    mpeg_info->video[ v_idx ].frate = frame_rates[ frate ];

    offset += 1; // after picture rate

    // 18 following bytes are the bitrate /400

    //read first 16 bytes
    brate = GetSize( offset );
    // scale
    brate <<= 2;
    byte lasttwo = GetByte( offset + 2 );
    lasttwo >>= 6;
    brate |= lasttwo;

    mpeg_info->video[ v_idx ].bitrate = 400 * brate;

    byte mark;
    while ( true ) {
        offset = FindNextMarker( offset, &mark );
        if ( mark == MPEG_GOP_CODE )
            break;
        switch ( GetByte( offset + 3 ) ) {
        case MPEG_EXT_CODE :
            // Extension
            offset += 4;
            switch ( GetByte( offset ) >> 4 ) {
            case 1:
                //SequenceExt
                if ( GetByte( offset + 1 ) & 0x08 )
                    mpeg_info->video[ v_idx ].progressive = true;
                mpeg_info->video[ v_idx ].chroma_format = ( GetByte( offset + 1 ) & 0x06 ) >> 1;
                break;
            case 2:
                // SequenceDisplayExt
                mpeg_info->video[ v_idx ].video_format = ( GetByte( offset ) & 0x0E ) >> 1;
                break;
            }

            break;
        case MPEG_USER_CODE :
            // UserData
            break;
        }
        offset++;
    }

    mpeg_info->video[ v_idx ].seen = true;
}

double K3b::MpegInfo::ReadTS( llong offset )
{
    byte highbit;
    unsigned long low4Bytes;
    double TS;

    highbit = ( GetByte( offset ) >> 3 ) & 0x01;

    low4Bytes = ( ( GetByte( offset ) >> 1 ) & 0x03 ) << 30;
    low4Bytes |= GetByte( offset + 1 ) << 22;
    low4Bytes |= ( GetByte( offset + 2 ) >> 1 ) << 15;
    low4Bytes |= GetByte( offset + 3 ) << 7;
    low4Bytes |= GetByte( offset + 4 ) >> 1;


    TS = ( double ) ( highbit * FLOAT_0x10000 * FLOAT_0x10000 );
    TS += ( double ) ( low4Bytes );
    TS /= ( double ) ( STD_SYSTEM_CLOCK_FREQ );

    return TS;
}

double K3b::MpegInfo::ReadTSMpeg2( llong offset )
{
    byte highbit;
    unsigned long low4Bytes;
    unsigned long sys_clock_ref;
    double TS;

    highbit = ( GetByte( offset ) & 0x20 ) >> 5;

    low4Bytes = ( ( GetByte( offset ) & 0x18 ) >> 3 ) << 30;
    low4Bytes |= ( GetByte( offset ) & 0x03 ) << 28;
    low4Bytes |= GetByte( offset + 1 ) << 20;
    low4Bytes |= ( GetByte( offset + 2 ) & 0xF8 ) << 12;
    low4Bytes |= ( GetByte( offset + 2 ) & 0x03 ) << 13;
    low4Bytes |= GetByte( offset + 3 ) << 5;
    low4Bytes |= ( GetByte( offset + 4 ) ) >> 3;

    sys_clock_ref = ( GetByte( offset + 4 ) & 0x3 ) << 7;
    sys_clock_ref |= ( GetByte( offset + 5 ) >> 1 );

    TS = ( double ) ( highbit * FLOAT_0x10000 * FLOAT_0x10000 );
    TS += ( double ) ( low4Bytes );
    if ( sys_clock_ref == 0 )
        TS /= ( double ) ( STD_SYSTEM_CLOCK_FREQ );
    else {
        TS /= ( double ) ( 27000000 / sys_clock_ref );
    }

    return TS;
}
