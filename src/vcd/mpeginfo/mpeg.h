/*
*
* $Id$
* Copyright (C) 2003 Christian Kvasny <chris@k3b.org>
*
* This file is part of the K3b project.
* Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
* See the file "COPYING" for the exact licensing terms.
*/

#ifndef MpEg
#define MpEg

#include "common.h"

typedef struct
{
    // this stores the junk header of mpeg files, in case someone needs it later
    off_t size;
    byte *buf;

}
header_buf;

// return a pointer to the junk
header_buf *readHeader( FILE *myMpegfile, off_t offset, int rw );

typedef int PID;

class mpeg_descriptors
{
    public:
        mpeg_descriptors()
        {
            video_coding_version = -1;
            audio_coding_version = -1;
            CA = -1;
            CA_PID = -1;
            lang_audio_type = -1;
            languages = 0;
            copyright = false;
            n_unhandled_desc = 0;
        }
        ~mpeg_descriptors()
        {
            if ( languages )
                delete[] languages;
        }
        void PrintInfos( const char* prefix );
        int video_coding_version;
        int audio_coding_version;
        int CA;
        PID CA_PID;
        int lang_audio_type;
        char* languages;
        bool copyright;
        byte n_unhandled_desc;
};


typedef struct ES_t
{
    PID pid;
    byte type;
    ES_t *next;
    mpeg_descriptors descs;
    FILE* demuxFile;
    bool demuxFileOk;
}
EStream;


typedef struct t_prog
{
    EStream *TStreams;
    int nstreams;
    int prog_num;
    mpeg_descriptors descs;
}
program;



class mpeg;

//#define MAX_PROGS 100

//helper class that carries transport stream infos
class transport
{
        friend class mpeg;
        friend class demuxer;
    public :
        transport();
        ~transport();

    protected:
        void delete_programs();
        void delete_ES( EStream* stream );
        void PrintInfos();
        program* programs;
        int n_progs;
        int n_audio_streams;
        int n_video_streams;
        int n_other_streams;
        PID network_PID;

        PID* PMT_PIDs;
        int n_PMT_PIDs;
        int read_pmts;
};

class mpeg
{
        friend class mpegSystemOut;
        friend class demuxer;
    protected:
        // disallow basic constructors
        mpeg()
        {}
        ;

        // Functions

        //  General ToolBox

        byte GetByte( off_t offset );
        byte bdGetByte( off_t offset );
        bool EnsureMPEG( off_t offset, marker mark );
        double ReadTS ( off_t offset );
        double ReadTSMpeg2( off_t offset );
        long ReadPACKMuxRate( off_t offset );

        // find any marker
        off_t FindNextMarker( off_t from );
        // find any marker, change mark accordingly
        off_t FindNextMarker( off_t from, marker* mark );
        off_t bdFindNextMarker( off_t from, marker* mark );
        // find 0x00 00 01 mark
        void print_all_ts( byte kind );
    public:
        mpeg( const char* filename, int verbosity = mpeg_SILENT );
        ~mpeg();

        //  General ToolBox

        int MpegVersion();
        void PrintInfos();
        bool has_audio()
        {
            return HasAudio;
        }
        bool has_video()
        {
            return HasVideo;
        }
        off_t Size()
        {
            return FileSize;
        }
        const char* Name()
        {
            return FileName;
        }
        int Version()
        {
            return mpeg_version;
        }
        bool Match( mpeg* peer );
        float Duration();
        byte Byte( off_t offset );


        off_t FindNextMarker( off_t from, marker mark );
        off_t bdFindNextMarker( off_t from, marker mark );

        unsigned short int GetSize( off_t offset );

        void SecsToHMS( char* HMS, float duration );

        //  Audio ToolBox
        bool ParseAudio( off_t myoffset );
        bool ParseID3();
        bool PrintID3();

        off_t FindMatchingAudio( off_t myoffset );
        off_t bdFindMatchingAudio( off_t myoffset );
        bool MatchAudio( off_t myoffset );
        //  Video ToolBox
        bool ParseVideo( off_t myoffset );
        bool ParseExtension( off_t myoffset );
        bool ParseSequenceExt( off_t myoffset );
        bool ParseSequenceDisplayExt( off_t myoffset );
        bool ParseUserData( off_t myoffset );
        //  System ToolBox
        bool ParseSystem();
        bool ParseSystemPacket( off_t startofpacket, off_t startofpack );
        bool ParseSystemHeader( off_t myoffset );
        off_t SkipPacketHeader( off_t myoffset );

        void ParseFramesInGOP( off_t offset );
        bool ParseRIFF();
        bool ParseTransportStream( off_t offset );
        void ParseCAT( off_t offset );
        void ParsePAT( off_t offset );
        void ParsePMT( off_t offset );
        off_t ParseDescriptor( off_t offset, mpeg_descriptors* target );
        void DescCA( off_t offset, mpeg_descriptors* target );
        void DescLang( off_t offset, mpeg_descriptors* target );

        long Read12bitLength( off_t offset );
        long Read2Bytes( off_t offset );
        long ReadPID( off_t offset );

        PID NextTrPacket( off_t* offset, off_t* payload_start, off_t* payload_end );

        // file handle (I hate iostream and I can't use read/write on windows)
        FILE* MpegFile;

        // how much we speak
        int Verboseness;

        // mpeg Internals to be filled by constructor
        bool HasAudio, HasVideo;
        bool composite, editable;

        off_t FileSize;

        char* FileName;
        int MpegType;

        // audio internals
        mpgtx_audio* Audio;
        byte n_audio;
        // video internals
        mpgtx_video* Video;
        byte n_video;
        // system internals
        mpgtx_system* System;

        // transport stream internals
        transport* Transport;

        // extentions
        sequence_ext* SExt;
        display_ext* DExt;
        user_data* UData;

        // start with id3v2 tag
        bool start_with_id3;
        // for mpeg 1/ mpeg2
        byte mpeg_version;

        // for GetByte

        off_t buffstart;
        off_t buffend;
        byte* buffer;

};


typedef struct
{
    char* file;
    off_t from;
    float sfrom;
    off_t to;
    float sto;
    bool from_included;
    bool to_included;
    bool until_file_size;
    bool unit_is_second;
    mpeg* mpegfile;
}
chunk;

#endif //MpEg

