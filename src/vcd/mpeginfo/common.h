/***************************************************************************
                          common.h  -  description
                             -------------------
    begin                : Sam Dez 14 2002
    copyright            : (C) 2002 by christian kvasny
    email                : chris@ckvsoft.at
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef CoMmOn
#define CoMmOn


// SYSTEM INCLUDE FILES

#include <stdio.h>

/*
 * for fopen, fprintf, printf, fread...
 */
#include <string.h>

/*
 * fseeko(), ftello() conforms to SUS 2 for LFS suppport
 * Also needed for strcpy, strlen..
 */
#include <sys/types.h>



// if you don't have the signal system call (Windows for ex.)
// then uncomment the next line
//#define NOSIGNAL_H 1

#define mpeg_SILENT  0
#define mpeg_NORMAL  1
#define mpeg_VERBOSE 2

// packlength is variable since mpeg2
//#define PACKlength 12

#define PaddingPkt   0xBE
#define VideoPkt     0xE0
#define AudioPkt  	 0xC0
#define SystemPkt    0xBB


#define mpeg_AUDIO     1
#define mpeg_VIDEO     2
#define mpeg_SYSTEM    3
#define mpeg_UNKNOWN   4
#define mpeg_EMPTY     5
#define mpeg_TRANSPORT 6

// #define BUFFERSIZE   512
#define BUFFERSIZE   16384
//16k buffer
//#define COPYBUFFERSIZE 512
//#define COPYBUFFERSIZE 65536
//64k buffer
#define COPYBUFFERSIZE 2097152
//2meg buffer

//#define ENABLE_OPTIMIZATION 1
// this is used to calculate time
// used by 1)printf 2)mpeg::GetByte 3) mpegOut::Copy
#ifdef ENABLE_OPTIMIZATION

#define MAIN 0
#define PRINTF 1
#define GETBYTE 2
#define COPY 3
#define SELF 4
#define N_FUNCTIONS 5

double Clock_Now();
void   AddTime(double timestart, int function);
extern double functions_cummulated_time[N_FUNCTIONS];
void init_cummulated_time();
void PrintTime();

#define START_CLOCK double StartOfClock = Clock_Now();
#define STOP_CLOCK(x) AddTime(StartOfClock,x);
extern double MainClockStart;
#else
#define START_CLOCK ;
#define STOP_CLOCK(x) ;
#endif



#define MAX_ID3_GENRE 148
extern const char *genre[MAX_ID3_GENRE];

typedef unsigned char byte;
typedef unsigned char marker;
typedef const char* c_char;

typedef struct {
	char name[30];
	char artist[30];
	char album [30];
	char year [4];
	char comment [30];
	unsigned char genre;
} id3;

typedef struct {
	int mpeg_ver;
	int layer;
	int protect;
	int bitrate;
	float byte_rate;
	int sampling_rate;
	int mode;
	int padding;
	int modext;
	int emphasis_index;
	bool copyright;
	bool original;
	double duration;
	int frame_length;
	id3* tag;
	off_t first_frame_offset;
} mpgtx_audio;

typedef struct {
	unsigned long hsize,vsize;
	double frame_rate;
	unsigned long bitrate;
	double duration;
	byte aspect_ratio;
	byte* video_header;
	int   video_header_size;

	off_t first_gop_offset;
} mpgtx_video;

typedef struct {
	byte* video_system_header;
	off_t  video_system_header_length;

	byte* audio_system_header;
	off_t  audio_system_header_length;

	byte* first_video_packet;
	off_t   first_video_packet_length;
	//initial timestamp
	double initial_TS;
	unsigned long muxrate;
} mpgtx_system;


typedef struct {
	bool progressive;
	byte  chroma_format;
	bool low_delay;
} sequence_ext;

typedef struct {
	byte video_format;
	byte colour_prim;
	byte transfer_char;
	byte matrix_coef;
	unsigned long h_display_size;
	unsigned long v_display_size;
}display_ext;

typedef struct {
	char* ud;
	int ud_length;
}user_data;


/*  The following Macros ensure compatibility for glibc < 2.2+ and kernel < 2.4
    It also gives opportunity to support large files (>4 Go) such as DVDs
	_OFF_d and _OFF_x are used in format strings of printf. They refer to the off_t type
	and substitute to %lld %llx resp, whith large file support and %ld %lx otherwise.

	FSEEK substitutes to the new fseeko function if possible or becomes standard fseek otherwise
	FTELL does the same with ftello and ftell

*/


#ifdef _LARGEFILE_SOURCE
#ifdef _MACOSX
	#define   _OFF_d   "%qd"
	#define   _OFF_x   "%qx"
#else
	#define   _OFF_d   "%lld"
        #define   _OFF_x   "%llx"
#endif
	#define   FSEEK    fseeko
	#define   FTELL    ftello
#else
	#define   _OFF_d   "%ld"
	#define   _OFF_x   "%lx"
	#define   FSEEK    fseek
	#define   FTELL    ftell
#endif


#endif //CoMmOn

