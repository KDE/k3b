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

#ifndef K3BMPEGINFO
#define K3BMPEGINFO

#include <stdio.h>

// #define BUFFERSIZE   16384
#define BUFFERSIZE   65536

#define MPEG_START_CODE_PATTERN  ((ulong) 0x00000100)
#define MPEG_START_CODE_MASK     ((ulong) 0xffffff00)

#define MPEG_PICTURE_CODE        ((ulong) 0x00000100)
/* [...slice codes... 0x1a7] */

#define MPEG_USER_CODE           ((uchar) 0xb2)
#define MPEG_SEQUENCE_CODE       ((uchar) 0xb3)
#define MPEG_EXT_CODE            ((uchar) 0xb5)
#define MPEG_SEQ_END_CODE        ((uchar) 0xb7)
#define MPEG_GOP_CODE            ((uchar) 0xb8)
#define MPEG_PROGRAM_END_CODE    ((uchar) 0xb9)
#define MPEG_PACK_HEADER_CODE    ((uchar) 0xba)
#define MPEG_SYSTEM_HEADER_CODE  ((uchar) 0xbb)
#define MPEG_PRIVATE_1_CODE      ((uchar) 0xbd)
#define MPEG_PAD_CODE            ((uchar) 0xbe)

#define MPEG_AUDIO_C0_CODE       ((uchar) 0xc0) /* default */
#define MPEG_AUDIO_C1_CODE       ((uchar) 0xc1) /* 2nd audio stream id (dual channel) */
#define MPEG_AUDIO_C2_CODE       ((uchar) 0xc2) /* 3rd audio stream id (surround sound) */

#define MPEG_VIDEO_E0_CODE       ((uchar) 0xe0) /* motion */
#define MPEG_VIDEO_E1_CODE       ((uchar) 0xe1) /* lowres still */
#define MPEG_VIDEO_E2_CODE       ((uchar) 0xe2) /* hires still */

#define FLOAT_0x10000 (double)((unsigned long)1 << 16)
#define STD_SYSTEM_CLOCK_FREQ (unsigned long)90000

typedef unsigned char byte;
typedef long long llong;

#include <kdebug.h>

namespace K3b {
    class video_info
    {
    public:
        bool seen;
        unsigned long hsize;
        unsigned long vsize;
        double aratio;
        double frate;
        unsigned long bitrate;
        unsigned long vbvsize;
        bool progressive;
        unsigned char video_format;
        unsigned char chroma_format;
        bool constrained_flag;
    };


    class audio_info
    {
    public:
        bool seen;
        unsigned int version;
        unsigned int layer;
        unsigned int protect;
        unsigned long bitrate;
        float byterate;
        unsigned long sampfreq;
        int mode;
        bool copyright;
        bool original;
    };


    class Mpeginfo
    {

    public:
        Mpeginfo()
            : version( 0 ),
              muxrate( 0 ),
              playing_time( 0 ),
              has_video ( false ),
              has_audio ( false )
        {
            for ( int i = 0; i < 3; i++ ) {
                video[ i ].seen = false;
                audio[ i ].seen = false;
            }
        };

        ~Mpeginfo()
        {}
        ;

        unsigned int version;
        unsigned long muxrate;
        double playing_time;
        bool has_video;
        bool has_audio;
        video_info video[ 3 ];
        audio_info audio[ 3 ];
    };

    class MpegInfo
    {
    public:
        MpegInfo( const char* filename );
        ~MpegInfo();
        enum mpeg_version { MPEG_VERS_INVALID = 0, MPEG_VERS_MPEG1 = 1, MPEG_VERS_MPEG2 = 2 };
        enum mode { MPEG_STEREO = 1, MPEG_JOINT_STEREO, MPEG_DUAL_CHANNEL, MPEG_SINGLE_CHANNEL };

        int version()
        {
            return mpeg_info->version;
        };
        const QString error_string()
        {
            return m_error_string;
        };
        Mpeginfo* mpeg_info;


    private:
        //  General ToolBox
        byte GetByte( llong offset );
        byte bdGetByte( llong offset );
        llong GetNBytes( llong, int );
        unsigned short int GetSize( llong offset );
        llong FindNextMarker( llong );
        llong FindNextMarker( llong, byte* );
        llong FindNextMarker( llong, byte );
        llong bdFindNextMarker( llong, byte );
        llong bdFindNextMarker( llong, byte* );
        llong FindNextVideo( llong );
        llong FindNextAudio( llong );

        int GetVideoIdx ( byte );
        int GetAudioIdx ( byte );
        bool EnsureMPEG( llong, byte );
        void ParseVideo ( llong, byte );
        void ParseAudio ( llong, byte );
        bool MpegParsePacket ();
        llong MpegParsePacket ( llong );
        llong SkipPacketHeader( llong );

        double ReadTS( llong offset );
        double ReadTSMpeg2( llong offset );

        FILE* m_mpegfile;

        const char* m_filename;
        llong m_filesize;

        bool m_done;

        llong m_buffstart;
        llong m_buffend;
        byte* m_buffer;
        double m_initial_TS;
        QString m_error_string;

    };
}

#endif //MpegInfo
