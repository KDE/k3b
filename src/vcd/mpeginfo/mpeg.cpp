/***************************************************************************
                          mpeg.cpp  -  description
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


#include "mpeg.h"
#include <stdlib.h>
#include <stdio.h>

extern bool desperate_mode;
extern bool preserve_header;
int mpeg2found = 0;

// #define _DEBUG_
int mpeg_bitrate_index [2][3][16] =
{{{0,32,64,96,128,160,192,224,256,288,320,352,384,416,448,0},
	{0,32,48,56,64,80,96,112,128,160,192,224,256,320,384,0},
	{0,32,40,48,56,64,80,96,112,128,160,192,224,256,320,0}},
	{{0,32,48,56, 64,80 ,96 ,112,128,144,160,176,192,224,256,0},
		{0, 8,16,24,32,40,48, 56,64 ,80 ,96 ,112,128,144,160,0},
		{0, 8,16,24,32,40,48, 56,64 ,80 ,96 ,112,128,144,160,0}}};

int mpeg_sampling_index [3][4] =
{
	{44100,48000,32000,0}, //mpeg 1
	{22050,24000,16000,0}, //mpeg 2
	{11025,12000,8000,0}   //mpeg 2.5

};

double mpeg_frame_rate_index [9] =
{
	0., 24000./1001., 24., 25.,
	30000./1001., 30., 50.,
	60000./1001., 60.
};

// ripped from id3ed V1.10.2 (Thx to Matt Mueller)
// HEY you didn't want me to type *that* !
const char *genre[MAX_ID3_GENRE]={
	"Blues",		"Classic Rock",	"Country",			"Dance",			"Disco",			"Funk",
	"Grunge", 		"Hip-Hop", 		"Jazz", 			"Metal", 			"New Age", 			"Oldies",
	"Other", 		"Pop", 			"R&B", 				"Rap", 				"Reggae", 			"Rock",
	"Techno", 		"Industrial", 	"Alternative",		"Ska", 				"Death Metal",		"Pranks",
	"Soundtrack",	"Euro-Techno",	"Ambient",			"Trip-Hop", 		"Vocal", 			"Jazz+Funk",
	"Fusion", 		"Trance", 		"Classical",		"Instrumental",		"Acid", 			"House",
	"Game", 		"Sound Clip", 	"Gospel", 			"Noise", 			"AlternRock",		"Bass",
	"Soul", 		"Punk", 		"Space", 			"Meditative",		"Instrumental Pop",	"Instrumental Rock",
	"Ethnic", 		"Gothic", 		"Darkwave", 		"Techno-Industrial","Electronic", 		"Pop-Folk",
	"Eurodance",	"Dream", 		"Southern Rock",	"Comedy",			"Cult", 			"Gangsta",
	"Top 40", 		"Christian Rap","Pop/Funk", 		"Jungle", 			"Native American",	"Cabaret",
	"New Wave", 	"Psychadelic", 	"Rave", 			"Showtunes", 		"Trailer", 			"Lo-Fi",
	"Tribal", 		"Acid Punk", 	"Acid Jazz", 		"Polka", 			"Retro", 			"Musical",
	"Rock & Roll",	"Hard Rock", 	"Folk", 			"Folk/Rock", 		"National Folk",	"Swing",
	"Fast-Fusion",	"Bebob", 		"Latin", 			"Revival", 			"Celtic", 			"Bluegrass",
	"Avantgarde",	"Gothic Rock", 	"Progressive Rock",	"Psychedelic Rock", "Symphonic Rock",	"Slow Rock",
	"Big Band",		"Chorus", 		"Easy Listening",	"Acoustic", 		"Humour", 			"Speech",
	"Chanson",		"Opera", 		"Chamber Music",	"Sonata", 			"Symphony", 		"Booty Bass",
	"Primus",		"Porn Groove", 	"Satire", 			"Slow Jam", 		"Club", 			"Tango",
	"Samba",		"Folklore", 	"Ballad", 			"Power Ballad",		"Rhythmic Soul", 	"Freestyle",
	"Duet",			"Punk Rock", 	"Drum Solo", 		"A capella", 		"Euro-House", 		"Dance Hall",
	"Goa", 			"Drum & Bass", 	"Club House", 		"Hardcore", 		"Terror",			"Indie",
	"BritPop", 		"NegerPunk", 	"Polsk Punk", 		"Beat",				"Christian Gangsta","Heavy Metal",
	"Black Metal",	"Crossover", 	"Contemporary C",	"Christian Rock",	"Merengue", 		"Salsa",
	"Thrash Metal",	"Anime", 		"JPop",				"SynthPop"
};

float mpeg::Duration(){
	if (Audio) return float(Audio->duration);
	if (Video) return float(Video->duration);
	return 0;
}


bool mpeg::Match(mpeg* peer){
	if (MpegType != peer->MpegType) {
		fprintf(stderr, "mpeg files are not the same type!\n");
		return false;
	}

	if (MpegType == mpeg_AUDIO || MpegType == mpeg_SYSTEM) {
		if (Audio->bitrate != peer->Audio->bitrate) {
			fprintf (stderr, "Incompatible audio bitrates %s (%d bps) %s (%d bps)\n",
					FileName, Audio->bitrate,
					peer->FileName, peer->Audio->bitrate);
			return false;
		}
		if (Audio->mpeg_ver != peer->Audio->mpeg_ver) {
			fprintf (stderr, "Incompatible mpeg audio versions "
					"%s is mpeg %d %s is mpeg %d\n",
					FileName,Audio->mpeg_ver,
					peer->FileName, peer->Audio->mpeg_ver);
			return false;
		}
		if (Audio->layer != peer->Audio->layer) {
			fprintf (stderr, "Incompatible audio layers %s is layer %d  %s is layer %d",
					FileName, Audio->layer,
					peer->FileName, peer->Audio->layer);
			return false;
		}
		if (Audio->sampling_rate != peer->Audio->sampling_rate){
			fprintf (stderr,"Incompatible sampling rates %s (%d Hz) %s (%d Hz)\n",
					FileName, Audio->sampling_rate,
					peer->FileName, peer->Audio->sampling_rate);
			return false;
		}


		return true;
	}

	if (MpegType == mpeg_VIDEO || MpegType == mpeg_SYSTEM){
		if (  (Video->hsize != peer->Video->hsize) ||
				(Video->vsize != peer->Video->vsize)) {
			fprintf(stderr,"Incompatible size %s [%ldx%ld] %s [%ldx%ld]\n",
					FileName, Video->hsize,Video->vsize,
					peer->FileName, peer->Video->hsize, peer->Video->vsize);
			return false;
		}
		if (Video->bitrate != peer->Video->bitrate) {
			fprintf(stderr,"incompatible video bitrate %s (%ld bps) %s (%ld bps)\n",
					FileName, Video->bitrate,
					peer->FileName, peer->Video->bitrate);
			return false;
		}
		if (Video->frame_rate != peer->Video->frame_rate) {
			fprintf(stderr, "incompatible video frame rate %s (%f fps) %s (%f fps)\n",
					FileName, Video->frame_rate,
					peer->FileName, peer->Video->frame_rate);
			return false;
		}

		return true;
	}
	// no known file
	return false;
}




mpeg::mpeg(const char* filename,int verbosity)
	: MpegFile(0), Verboseness(verbosity),
	HasAudio(false), HasVideo(false),
	composite(false), editable(false),
	MpegType(mpeg_UNKNOWN),
	Audio(0), n_audio(0), Video(0), n_video(0), System(0), Transport(0),
	start_with_id3(false),
	mpeg_version(1),
buffstart(0),buffend(0),buffer(0)
{
	UData=0;
	SExt=0;
	DExt=0;

	// keep track of filename (usefull for debugging)
	// TODO : only on mpeg_VERBOSE ?

	FileName = new char[strlen(filename) + 1];
	strcpy(FileName, filename);

	// open infile read-only, binary type (for compatibility with other than POSIX/GNU)
	MpegFile = fopen(filename, "rb");
#ifdef _DEBUG_
	Verboseness = mpeg_VERBOSE;
#endif

	// case unable to open
	if (MpegFile == 0) {
		if (Verboseness!=mpeg_SILENT) {
			printf("Unable to open %s\n",filename);
			if(Verboseness == mpeg_VERBOSE)
				perror(filename);
		}
		//nothing more to do...
		return;
	}

	// Allocate buffer
	buffer = new byte[BUFFERSIZE];

	// Seek to end of file

	if (FSEEK(MpegFile, 0, SEEK_END)) {
		if (Verboseness != mpeg_SILENT) {
			printf("Unable to seek in file %s\n", filename);
			if (Verboseness == mpeg_VERBOSE)
				perror(filename);
		}
		//give up..
		return;
	}

	// File size

	off_t off_eof = FTELL(MpegFile);

	if (off_eof == -1) {
		if (Verboseness != mpeg_SILENT) {
			printf("Seeking to end of input file %s failed.\n", filename);
			if (Verboseness == mpeg_VERBOSE)
				perror(filename);
		}
		//give up..
		return;
	} else FileSize = off_eof;

	//	if (Verboseness==mpeg_VERBOSE){
#ifdef _DEBUG_
	printf("%s has size " _OFF_d "\n", FileName, FileSize);
#endif

	//	}

	// nothing to do on an empty file
	if (!FileSize) {
		if (Verboseness != mpeg_SILENT) {
			printf("File %s is empty.\n", filename);
			if (Verboseness == mpeg_VERBOSE)
				perror(filename);
		}
		//give up..
		return;
	}



	// first determine the file type between audio/video/system
	if (ParseAudio(0)) {
		// this is an Audio only mpeg file
		MpegType = mpeg_AUDIO;
		composite = false;
		editable = true;
	}
	else if (ParseVideo(0)) {
		// this is a Video only mpeg file
		MpegType = mpeg_VIDEO;
		composite = false;
		editable = true;
	}
	else if (ParseSystem()) {
		// this is an Audio and Video mpeg file
		MpegType = mpeg_SYSTEM;

	}
	else if (ParseID3()) {
		//this is an audio file with an ID3 v2
		MpegType = mpeg_AUDIO;
		editable = true;
		composite = false;
	}
	else if (ParseRIFF()) {
		// this is either a Wave file or a divx or something else
		editable = false;
		composite = false;
	}
	else if (ParseTransportStream(0)) {
		// this is a MPEG 2 transport stream
		MpegType = mpeg_TRANSPORT;
		editable = false;
		composite = true;
	}
	else
	{
		// can not handle this mpeg file
		if (Verboseness == mpeg_VERBOSE)
			fprintf(stderr,
					"%s is not a valid MPEG file (can't handle it)\n",
					FileName);
	}
}


// mpeg class destructor

mpeg::~mpeg() {

	if (buffer) delete[] buffer;
	if (MpegFile) fclose(MpegFile);
	if (Audio) {
		if (Audio->tag) delete Audio->tag;
		delete Audio;
	}
	if (Video) {
		if (Video->video_header)
			delete[] Video->video_header;
		delete Video;
	}
	if (System) {
		if(System->audio_system_header)
			delete[] System->audio_system_header;
		if(System->video_system_header)
			delete[] System->video_system_header;
		delete System;
	}
	if (SExt) delete SExt;
	if (DExt) delete DExt;
	if (UData) {
		if (UData->ud) delete[] UData->ud;
		delete UData;
	}
	if (Transport) delete Transport;
}




// map a buffer onto file for efficiency
#ifndef _WIN32
#ifndef _MACOSX
inline
#endif
#endif

byte mpeg::GetByte(off_t offset) {
	START_CLOCK;
	size_t nread;
	if ((offset >= buffend) || (offset < buffstart))
	{

		if (FSEEK(MpegFile, offset, SEEK_SET))
		{
			if (Verboseness == mpeg_VERBOSE)
				fprintf(stderr, "could not get seek to offset (" _OFF_d ") in file %s (size:" _OFF_d ")\n",
						offset, FileName, FileSize);

			fprintf(stderr,"mpgtx: AT EOF - please stop me!\n");
			return 0x11;
		}
		nread = fread(buffer, 1, BUFFERSIZE, MpegFile);
		buffstart = offset;
		buffend = offset + nread;
		if ((offset >= buffend) || (offset < buffstart))
		{
			// weird
			if (Verboseness == mpeg_VERBOSE)

				fprintf(stderr, "could not get offset " _OFF_d " in file %s [" _OFF_x "]\n",
						offset, FileName, FileSize);
			fprintf(stderr,"mpgtx: AT EOF - please stop me!\n");
			return 0x11;
		}
	}
	STOP_CLOCK(GETBYTE);
	return buffer[offset - buffstart];
}


byte mpeg::Byte(off_t offset){
	return GetByte(offset); // because GetByte is inline
}


// same as above but improved for backward search
#ifndef _WIN32
#ifndef _MACOSX
inline
#endif
#endif

byte mpeg::bdGetByte(off_t offset){
	size_t nread;
	if ((offset >= buffend) || (offset < buffstart)){
		off_t start = offset-BUFFERSIZE+1 ;
		start = start >= 0?start:0;

		FSEEK(MpegFile, start, SEEK_SET);

		nread = fread(buffer, 1, BUFFERSIZE, MpegFile);
		buffstart = start;
		buffend = start + nread;
		if ((offset >= buffend) || (offset < buffstart))
		{
			// weird
			if(Verboseness == mpeg_VERBOSE)
				fprintf(stderr,"could not get offset " _OFF_x " in file %s [" _OFF_x "]\n",
						offset, FileName, FileSize);

			return 0x11;
		}
	}
	return buffer[offset - buffstart];
}




// make sure the sequence 0x00 00 01 mark
#ifndef _WIN32
#ifndef _MACOSX
inline
#endif
#endif

bool mpeg::EnsureMPEG(off_t offset, marker mark){
	if ((	GetByte(offset) == 0x00		) &&
			(	GetByte(offset+1) == 0x00	) &&
			(	GetByte(offset+2) == 0x01	) &&
			(	GetByte(offset+3) == mark	))
		return true;
	else return false;
}





// get a two byte size
#ifndef _WIN32
#ifndef _MACOSX
inline
#endif
#endif


unsigned short int mpeg::GetSize(off_t offset ) {
	return GetByte(offset)*256 + GetByte(offset+1);

}


bool mpeg::PrintID3() {
	if	(	(GetByte(0)!='I') ||
			(GetByte(1)!='D') ||
			(GetByte(2)!='3'))
		return false;

	printf("  ID3 v2.%d.%d tag (more info on http://www.id3.org/)\n",
			GetByte(3), GetByte(4));
	printf("     ----------------\n");

	// get tag size
	size_t framesize;
	off_t tagsize;
	size_t i;
	tagsize =	GetByte(6)<<21;
	tagsize |=	GetByte(7)<<14;
	tagsize |=	GetByte(8)<<7;
	tagsize |=	GetByte(9);

#ifdef _DEBUG_
	printf("Tag size : " _OFF_d "\n", tagsize);
#endif

	bool handled;
	off_t offset = 10;

	while (offset < tagsize+10) {
		if ((GetByte(offset)<'0') || (GetByte(offset)>'Z')) break;
		handled = false;
		framesize = (GetByte(offset+4)<<24) | (GetByte(offset+5)<<16) |
			(GetByte(offset+6)<<8) | (GetByte(offset+7));
		printf("     %c%c%c%c : ", GetByte(offset), GetByte(offset+1),
				GetByte(offset+2), GetByte(offset+3));
		if (GetByte(offset) == 'T') {
			//text tag
			if (!GetByte(offset+10)) //if text encoding = standard encoding
				for (i = 1; i < framesize; i++) {
					printf("%c", GetByte(offset + i + 10));
					handled = true;
				}
			if (!handled) {
				printf("(empty)");
				handled=true;
			}
		}
		if (	(GetByte(offset)  =='C') &&
				(GetByte(offset+1)=='O') &&
				(GetByte(offset+2)=='M') &&
				(GetByte(offset+3)=='M')) {
			printf("(lang: %c%c%c) : ",
					GetByte(offset+11), GetByte(offset+12), GetByte(offset+13));
			for (i = 5; i < framesize; i++) {
				printf("%c", GetByte(offset + i + 10));
				handled = true;
			}
			if (!handled) {
				printf("(empty)");
				handled=true;
			}
		}
		if (!handled) printf("(skipped)");
		printf("\n");
		offset += 10 + framesize;
	}
	printf("     ----------------\n");
	return true;
}




bool mpeg::ParseID3(){
	if (	(GetByte(0)!='I') ||
			(GetByte(1)!='D') ||
			(GetByte(2)!='3'))
		return false;

	start_with_id3 = true;
	// get tag size
	size_t tagsize=0;
	tagsize = GetByte(6)<<21;
	tagsize |= GetByte(7)<<14;
	tagsize |= GetByte(8)<<7;
	tagsize |= GetByte(9);
	return ParseAudio(tagsize + 10);

}



// Parses Audio header returns false on not audio
bool mpeg::ParseAudio(off_t myoffset) {
	HasAudio = false;
	bool mpeg2_5 = false;
	int i;
	// ensure that two first bytes are FFFx
	// TODO: give it a chance and search for a valid start after the beginning
	// even if most audio players will refuse to play such a file
	if ((GetByte(myoffset+0)!=0xFF)||((GetByte(myoffset+1) & 0xF0)!=0xF0)) {
		// doesn't start with 12 bits set
		if ((GetByte(myoffset+0)!=0xFF)||((GetByte(myoffset+1) & 0xE0)!=0xE0)) {
			// doesn't start with 11 bits set
			return false;
		} else {
			// start with 11 bits set
			mpeg2_5 = true;
		}
	}

	// seems to be audio, let's allocate the struct
	Audio=new mpgtx_audio;
	Audio->tag=0;
	Audio->first_frame_offset=myoffset;

	// Find mpeg version 1.0 or 2.0
	if(GetByte(myoffset + 1) & 0x08) {
		if (!mpeg2_5) Audio->mpeg_ver = 1;
		else return false; // invalid 01 encountered
	} else {
		if (!mpeg2_5) Audio->mpeg_ver = 2;
		else Audio->mpeg_ver = 3; //for mpeg 2.5
	}

	// Find Layer
	Audio->layer = (GetByte(myoffset + 1) & 0x06)>>1;
	switch(Audio->layer) {
		case 0: Audio->layer=-1; return false;
		case 1: Audio->layer=3; break;
		case 2: Audio->layer=2; break;
		case 3: Audio->layer=1; break;
	}

	// Protection Bit
	Audio->protect = GetByte(myoffset + 1) & 0x01;
	if (Audio->protect) Audio->protect = 0;
	else Audio->protect = 1;

	// Bitrate index and sampling index to pass through the array
	int bitrate_index = GetByte(myoffset+2)>>4;
	int sampling_index = (GetByte(myoffset+2)&0x0f)>>2;
	if (sampling_index >= 3) return false;
	if (bitrate_index == 15) return false;
	Audio->bitrate = mpeg_bitrate_index[Audio->mpeg_ver-1][Audio->layer-1][bitrate_index];
	Audio->byte_rate = (float)((Audio->bitrate*1000)/8.0);
	Audio->sampling_rate = mpeg_sampling_index[Audio->mpeg_ver-1][sampling_index];

	// Padding bit
	if (GetByte(myoffset+2)&0x02) Audio->padding = 1;
	else Audio->padding = 0;

	// Audio mode
	Audio->mode=GetByte(myoffset+3)>>6;

	if (Audio->mode==1) Audio->modext=(GetByte(myoffset+3)>>4)&0x03;
	else Audio->modext=-1;

	// Copyright bit
	if(GetByte(myoffset+3)&0x08) Audio->copyright=true;
	else Audio->copyright=false;

	// Original/Copy bit
	if(GetByte(myoffset+3)&0x04) Audio->original=true;
	else Audio->original=false;


	// emphasis index
	Audio->emphasis_index=GetByte(myoffset+3)&0x03;

	// Frame Length
	if (Audio->mpeg_ver == 1) {
		if (Audio->layer == 1) Audio->frame_length =
			int((48000.0*Audio->bitrate)/Audio->sampling_rate) + 4*Audio->padding;
		else Audio->frame_length =
			int((144000.0*Audio->bitrate)/Audio->sampling_rate) + Audio->padding;
	}
	else if (Audio->mpeg_ver == 2) {
		if (Audio->layer == 1) Audio->frame_length =
			int((24000.0*Audio->bitrate)/Audio->sampling_rate) +4*Audio->padding;
		else Audio->frame_length =
			int((72000.0*Audio->bitrate)/Audio->sampling_rate) +Audio->padding;
	}
	else {
		if (Verboseness == mpeg_VERBOSE)
			fprintf(stderr,
					"%s: Audio mpeg file layer invalid : "
					"%d (should be 1 or	2)\n",
					FileName,Audio->mpeg_ver);
		return false;
	}

	// size of CRC
	if (Audio->protect) Audio->frame_length += 2;
	//Audio->frame_length += 4; // size of standard header
	// let's get the id3 tag if present.
	if (	GetByte(FileSize-128) == 'T' &&
			GetByte(FileSize-127) == 'A' &&
			GetByte(FileSize-126) == 'G') {

		FSEEK(MpegFile, FileSize-125, SEEK_SET);

		Audio->tag = new id3;
		fread(Audio->tag, 1, sizeof(id3), MpegFile);
		FileSize -= 128;
		//empty unused fields
		bool keep_it=false;
		for ( i=0; i<30;i++) {
			if (Audio->tag->name[i] != ' ') {
				if (Audio->tag->name[i] != '\0')
					keep_it = true;
				break;
			}
		}
		if (!keep_it) Audio->tag->name[0] = 0;

		keep_it = false;
		for ( i=0; i<30;i++){
			if (Audio->tag->artist[i] != ' ') {
				if (Audio->tag->artist[i] != '\0')
					keep_it = true;
				break;
			}
		}
		if (!keep_it) Audio->tag->artist[0] = 0;

		keep_it = false;
		for ( i=0; i<30;i++) {
			if (Audio->tag->album[i] != ' ') {
				if (Audio->tag->album[i] != '\0')
					keep_it = true;
				break;
			}
		}
		if (!keep_it) Audio->tag->album[0] = 0;

		keep_it = false;
		for ( i=0; i<4;i++){
			if (Audio->tag->year[i] != ' ') {
				if (Audio->tag->year[i] != '\0')
					keep_it = true;
				break;
			}
		}
		if (!keep_it) Audio->tag->year[0] = 0;

		keep_it = false;
		for ( i=0; i<30;i++) {
			if (Audio->tag->comment[i] != ' '){
				if (Audio->tag->comment[i] != '\0')
					keep_it = true;
				break;
			}
		}
		if (!keep_it) Audio->tag->comment[0] = 0;

	}
	Audio->duration = ((FileSize*1.0)/Audio->bitrate)*0.008;
	HasAudio = true;

//	fprintf(stderr, "found audio.\n");
	return true;
}









//Parses Video header returns false on not video
bool mpeg::ParseVideo(off_t myoffset){

	// the MPEG video file should start with
	// the video start sequence, if not exit.
	// TODO: give it a chance and seek a video start elsewhere
	if(!EnsureMPEG(myoffset, 0xB3)) return false;
	off_t header_start=myoffset;
	// seems to be video okay, let's allocate the struct
	Video = new mpgtx_video;
	Video->video_header = 0;

	// Get vertical and horizontal picture sizes
	myoffset += 4; // after video sequence code
	Video->hsize = GetSize(myoffset) >> 4;
	Video->vsize = GetSize(myoffset+1) & 0x0FFF;

	// Get picture rate
	myoffset += 3; // after picture sizes
	int frame_rate_index = GetByte(myoffset) & 0x0F;
	if (frame_rate_index > 8) {
		// invalid frame rate index
		if (Verboseness == mpeg_VERBOSE)
			fprintf(stderr, "Invalid frame rate index in file %s : %d\n",
					FileName, frame_rate_index);
		Video->frame_rate = 0.0;
	}
	else Video->frame_rate = mpeg_frame_rate_index[frame_rate_index];

	Video->aspect_ratio = (GetByte(myoffset) & 0xF0) >>4;
	if (!Video->aspect_ratio) {
		if (Verboseness == mpeg_VERBOSE)
			fprintf(stderr, "Invalid aspect ratio in file %s : %d\n",
					FileName, Video->aspect_ratio);
	}

	//Get Bitrate for duration estimation
	myoffset += 1; // after picture rate

	// 18 following bytes are the bitrate /400

	//read first 16 bytes
	Video->bitrate = GetSize(myoffset);
	// scale
	Video->bitrate <<= 2;
	byte lasttwo = GetByte(myoffset + 2);
	lasttwo >>= 6;
	Video->bitrate |= lasttwo;

	//Given bitrate and FileSize, compute an estimation of the duration.
	Video->duration = (FileSize*8.0) / (Video->bitrate*400);

	//////////// Begin changes
	marker mymark;
	while (true) {
		myoffset = FindNextMarker(myoffset, &mymark);
		if (mymark == 0xB8) break;
		switch (GetByte(myoffset + 3)) {
			case 0xB5 :
				ParseExtension(myoffset);
				break;
			case 0xB2 :
#ifdef _DEBUG_
				printf("user data start code found\n");
#endif
				ParseUserData(myoffset);
				break;
		}
		myoffset++;
	}


	///////////// end changes

	//Okay let's save the video sequence header somewhere

	// find first GOP after video sequence header
	off_t header_end = FindNextMarker(myoffset, 0xB8);
	Video->first_gop_offset = header_end;
	// did we find header end or are we lost in cyberspace?
	if (header_end < 0){
		//could not find first GOP
		if(Verboseness == mpeg_VERBOSE)
			fprintf(stderr,
					"%s: could not find first GOP after Video Sequence start [" _OFF_x " (decimal)]\n",
					FileName, header_start);
		return false;
	}

	// okay we have header boundaries, let's fill the appropriate
	// field.
	Video->video_header_size = header_end - header_start;
	Video->video_header = new byte[Video->video_header_size];
	// fprintf(stderr,"philipp found header: 0x%qx 0x%qx\n", header_end, header_start);

	FSEEK(MpegFile, header_start, SEEK_SET);

	if(fread(Video->video_header, Video->video_header_size, 1, MpegFile) != 1) {
		// couldn't read video header
		if (Verboseness == mpeg_VERBOSE)
			fprintf(stderr,
					"%s: Found video header but couldn't read it [" _OFF_x "-" _OFF_x "]\n",
					FileName, header_start, header_end);
		return false;
	}

	HasVideo = true;
	return true;
}










//Parses System header returns false on not system
bool mpeg::ParseSystem() {
	off_t offset = 0;
	off_t PACKlength = 0;
	unsigned short packetsize;
	byte packettype;
	int found = 0;


	// the MPEG system file should start with
	// the video start sequence, if not exit.
	// TODO: give it a chance and seek a PACK elsewhere

	// try to skip any zeros at mpeg start (some begin with a BUNCH of zeros
	while(GetByte(offset)== 0x00) offset ++;
	//here we're on the first non null byte let's get back to leave two zeros (packet start code)
	offset -=2;

	if (offset != 0) {
		// we actually skipped some zeroes
		fprintf(stderr,"Skipped " _OFF_d " zeroes at start of file\n",offset);
	}

	if (!EnsureMPEG(offset, 0xBA)){
		//this file does not begin with a pack!
		fprintf(stderr,"mmm, this file does not start with a pack, offset: " _OFF_d " \n",offset);
		fprintf(stderr,"use the desperate_mode switch as the first option -X to search for a header in the whole file!\n");
		fprintf(stderr,"if you want to force the operation. May yield to an endless loop if no valid header is found!\n");
		if (GetByte(offset+2) != 0x01) {
			fprintf(stderr,"Does not even begin with a 00 00 01 xx sequence!\n");
			// does not even begin with a 00 00 01 xx sequence !
			if (desperate_mode)
			{
// philipp tries here to search further ....
			offset +=3;
			while (found  == 0) {
				fprintf (stderr,"Searching for a mpeg signature:\n");
				fprintf(stderr,"Searching for 0x00 at offset:           ");
				while(GetByte(offset) != 0x00) {
					fprintf (stderr,"\b\b\b\b\b\b\b\b\b\b%10qd", offset);
					offset++;
				}
				fprintf(stderr,"\nSearching for != 0x00 at offset:           ");
				while(GetByte(offset) == 0x00) {
					offset++;
					fprintf (stderr,"\b\b\b\b\b\b\b\b\b\b%10qd", offset);
				}
				// now from the beginning ..
				offset -=2;
				if (!EnsureMPEG(offset, 0xBA)){
					fprintf(stderr,"\nno success......\n");
					offset += 3;
				}
				else {
					fprintf(stderr,"\nsuccess! at " _OFF_d " \n",offset);
					found = 1;
					fprintf(stderr,"I dont know which kind of crazy header you have put into this file!\n");
					if (!preserve_header) {
					fprintf(stderr,"But I will assume this is ok. If you split or join this file, it will be discarded\n");
					}
					else
					{
					fprintf(stderr,"But I will assume this is ok. And if you split this file, it will be inserted\n");
					}
					fprintf(stderr,"Before any segment. This movie maybe wil not play with quicktime, use Videolan instead\n");
					fprintf(stderr,"If you dont want this, please correct the file, so its start with a valid Pack!\n");

				}
			}
			}
			if (found == 0)
				return false;
		}
	}

	// allocate necessary space
	System = new mpgtx_system;
	System->audio_system_header = 0;
	System->video_system_header = 0;
	System->first_video_packet = 0;

	// tries to find the two system packets and store them
	bool keep_going = true;
	marker mark = 0;
	// fprintf(stderr, "Philipp: Searching for system packets (audio, video).\n");

	while (keep_going)
	{
		// fprintf(stderr,"philipp: offset4 ?: 0x%qx\n",offset);

		offset = FindNextMarker(offset, &mark);
		// fprintf(stderr,"philipp: offset5 ?: 0x%qx\n",offset);

		// if error exit
		if (offset == -1) {
			fprintf(stderr, "offset error (-1)\n");
			break;
		}
		// if this is a Video or Audio packet we're done with system
		if ((mark == VideoPkt) || (mark == AudioPkt)) break;

		// if this is a padding packet
		if (mark == PaddingPkt) {
			offset += GetSize(offset + 4);
			continue;
		}

		// if this is a PACK
		if (mark==0xBA) {
			System->muxrate = ReadPACKMuxRate(offset + 4);
			offset += 12;	//standard pack length
			continue;
		}

		//Hey no more guess...
		if (mark != SystemPkt) {
			if (Verboseness == mpeg_VERBOSE)

				fprintf(stderr,
						"%s: Unhandled packet encountered (%x @" _OFF_x ") while seeking system headers\n",
						FileName, mark, offset);

			offset += 4;
			continue;
		}
		// it has to be a system packet

		//     March 27 changed because of variable length PACK in mpeg 2

		// are we lost?
		// fprintf(stderr,"philipp: offset3 ?: 0x%qx\n",offset);

		off_t startofpack = bdFindNextMarker(offset, 0xBA);
		// fprintf(stderr,"philipp: offset2 ?: 0x%qx\n",offset);

		if (startofpack !=-1){
			//found a PACK before system packet, compute its size (mpeg1 != mpeg2)
			if ((GetByte(startofpack + 4) & 0xF0) == 0x20) {
				// standard mpeg1 pack
				PACKlength=12;
			}
			else {
				if((GetByte(startofpack + 4) & 0xC0) == 0x40){
					// new mpeg2 pack : 14 bytes + stuffing
					PACKlength = 14 + (GetByte(startofpack + 13) & 0x07);
				}
				else{
					// wazup?
					fprintf(stderr, "Weird PACK encountered\n");
					PACKlength = 12;
				}
			}
		}


		if ((startofpack == -1) || (startofpack + PACKlength != offset)) {
			// we're probably lost
				fprintf(stderr,
						"%s: System Packet not preceded by a PACK [" _OFF_x "] start of pack : " _OFF_x " PACKlength : " _OFF_x " I'll probably crash but I love risk\n",
						FileName, offset,startofpack,PACKlength);
				startofpack=offset;

// keep going anyway			return false;
		}



		//   end of modif.

		ParseSystemPacket(offset,startofpack);

		packetsize = GetSize(offset + 4);  // to size field

		packettype = GetByte(offset + 12); // to ccode field

		if (GetByte(offset + 15) == AudioPkt || GetByte(offset + 15) == VideoPkt) {
			// system packet with both audio and vid infos
			packettype = VideoPkt;	// since video is mandatory
		}

		if (packettype == AudioPkt){
			if (System->audio_system_header != 0){
				if (Verboseness == mpeg_VERBOSE)
					fprintf(stderr,
							"%s: Warning two or more audio sys header encountered [" _OFF_x "]\n",
							FileName, offset);

				delete[] System->audio_system_header;
			}
			System->audio_system_header_length = PACKlength + 4 + 2 + packetsize;
			System->audio_system_header = new byte [System->audio_system_header_length];

			FSEEK(MpegFile, offset-PACKlength, SEEK_SET);


			fread(
					System->audio_system_header,
					System->audio_system_header_length,
					1, MpegFile);
		}
		else if (packettype == VideoPkt) {
			if (System->video_system_header != 0) {
				if (Verboseness == mpeg_VERBOSE)
					fprintf(stderr,
							"%s: Warning two or more video sys header encountered [" _OFF_x "]\n",
							FileName, offset);


				delete[] System->video_system_header;
			}

			// does this video packet begin
			System->video_system_header_length =
				PACKlength + 4 + 2 + packetsize;
			System->video_system_header =
				new byte [System->video_system_header_length];

			// keep track of the initial timestamp
			if (PACKlength == 12) {
				System->initial_TS = ReadTS(offset - PACKlength);
				mpeg2found = 0;
			}
			else {
				// off_t firstmpeg2gop = FindNextMarker(1, 0xB8);
#ifdef _DEBUG_
				fprintf(stderr,"Now read mpeg2 initial_TS at [" _OFF_x "] \n",offset - PACKlength);
				if (getchar() == 'y')
				 	print_all_ts(0xba);
				while(getchar() != '\n')
					;
#endif
				mpeg2found = 1;
//				System->initial_TS = ReadTSMpeg2(firstmpeg2gop + 4);
				System->initial_TS = ReadTSMpeg2(offset - PACKlength);

			}
			FSEEK(MpegFile, (offset - PACKlength), SEEK_SET);

			fread(
					System->video_system_header,
					System->video_system_header_length,
					1, MpegFile);
		}
		else {
			if (Verboseness==mpeg_VERBOSE)
				fprintf(stderr, "%s: Unknown system packet %x [" _OFF_x "]\n",
						FileName, packettype, offset);
		}


		offset += 4;
	}

	//okay this is a miracle but we have what we wanted here.

	// hey wait! are we really okay?
	if (!System->video_system_header) {
		//GOSH! we don't have video  here!
		if (Verboseness == mpeg_VERBOSE)
			fprintf(stderr,
					"%s: didn't find any video system header in this mpeg system file\n", FileName);
		//		return false;
		fprintf(stderr,
				"warning: couldn't find any valid system header. I'm continuing anyway\n");

	}


	// let's go on and find audio and video infos

	// Video!
	offset = FindNextMarker(0, 0xB3);
	// fprintf(stderr,"philipp: offset1 ?: 0x%qx\n",offset);
	if (offset == -1) {
		//couldn't find any valid video
		if (Verboseness == mpeg_VERBOSE)
			fprintf(stderr,
					"%s: didn't find any video sequence header in this mpeg system file\n", FileName);
		return false;
	}

	// mkay, we have the video sequence header
	if (!ParseVideo(offset)) {
		return false;
	}

	off_t xB3_offset = offset;
	// now get the pack and the packet header just before the video sequence
	offset = bdFindNextMarker(offset, 0xBA);


	if (offset == -1) {
		//couldn't find any PACK before...
		if (Verboseness == mpeg_VERBOSE)
			fprintf(stderr,
					"%s: didn't find any PACK before video sequence\n", FileName);
		return false;
	}

	// May 2, PACK don't necessarily precede video sequence!
	off_t start_of_vid_packet = bdFindNextMarker(xB3_offset, 0xE0);
	// start of packet
	off_t offset2 = bdFindNextMarker(start_of_vid_packet-1, &mark);
	if (offset != offset2) {
		// the packet that precedes video sequence is not a PACK
		offset = start_of_vid_packet;
	} else {
		// end May 2

		// March 27 compute pack length
		if ((GetByte(offset + 4) & 0xF0) == 0x20) {
			//standard mpeg1 pack
			PACKlength = 12;
		} else {
			if ((GetByte(offset + 4) & 0xC0) == 0x40) {
				//new mpeg2 pack : 14 bytes + stuffing
				PACKlength = 14 + (GetByte(offset + 13) & 0x07);
			} else {
				//wazup?
				fprintf(stderr,"Weird PACK encountered\n");
				PACKlength = 12;
			}
		}
		// end March 27

		// May 2
	}
	//end May 2



	// okay let's save this...
	System->first_video_packet = new byte[xB3_offset-offset];
	System->first_video_packet_length = xB3_offset-offset;

	readHeader(MpegFile,offset,1); // save this if needed for later
	FSEEK(MpegFile,offset,SEEK_SET);

	fread(System->first_video_packet, 1, xB3_offset-offset, MpegFile);


	// Now look for an audio packet if necessary
	HasAudio = false;
	offset = FindNextMarker(0, AudioPkt);
	if ((offset == -1) || (offset > FileSize - 15)) {
		if (Verboseness == mpeg_VERBOSE)
			fprintf(stderr,
					"%s: didn't find any audio packet in this mpeg system file\n",FileName);
	} else {

		// we assume that we are on the first audio packet...
		offset = SkipPacketHeader(offset);
		if (!ParseAudio(offset)) {
			//mmm audio packet doesn't begin with FFF
			while ((offset<FileSize - 10) && !HasAudio) {
				if ((GetByte(offset) == 0xFF) && (GetByte(offset + 1) & 0xF0) == 0xF0)
					if (ParseAudio(offset))
						HasAudio = true;
				offset++;
			} //if
		} //else
	} //while



	//seek the file duration by fetching the last PACK
	//and reading its timestamp

	off_t last_pack = bdFindNextMarker(FileSize - 13, 0xBA);
	// -12 because a PACK is at least 12 bytes

	double duration;
	if ((GetByte(last_pack + 4) & 0xF0) == 0x20){
		//standard mpeg1 pack
		duration = ReadTS(last_pack + 4);
	} else {
		last_pack = bdFindNextMarker(FileSize - 8, 0xB8);
		if(((GetByte(last_pack + 4) & 0xC0) == 0x40) || (mpeg2found == 1)) {
			//new mpeg2 pack : 14 bytes + stuffing
#ifdef _DEBUG_
			fprintf(stderr,"now getting mpeg2 running time");
#endif
			last_pack = bdFindNextMarker(FileSize - 8, 0xBA);
			duration = ReadTSMpeg2(last_pack + 4);
#ifdef _DEBUG_
			fprintf(stderr," : %lf\n", duration);
#endif

		} else {
			//wazup?
			duration = ReadTS(last_pack + 4);
		}
	}

	duration -= System->initial_TS;

	if (HasVideo) Video->duration = duration;
	if (HasAudio) Audio->duration = duration;
	//okay okay this is redundant

	return true;
}













// print mpeg infos on stdout
void mpeg::PrintInfos() {
	char HMS[30];
	if (MpegType == mpeg_AUDIO){
		SecsToHMS(HMS, Audio->duration);
		printf("%s\n", FileName);
		if (Audio->mpeg_ver != 3)
			printf("  Audio : Mpeg %d layer %d\n", Audio->mpeg_ver,Audio->layer);
		else
			printf("  Audio : Mpeg 2.5 (rare) layer %d\n", Audio->layer);

		printf("  Estimated Duration: %s\n", HMS);
		if (Audio->bitrate) {
			printf("  %d kbps  %d Hz\n", Audio->bitrate,Audio->sampling_rate);
			printf("  Frame size: %d bytes\n", Audio->frame_length);
		}
		else {
			printf("  free bitrate %d Hz (this file is currently unsplitable)\n",
					Audio->sampling_rate);
		}
		switch (Audio->mode) {
			case 0: printf("  Stereo,"); break;
			case 1: printf("  Joint Stereo: ");
					  if (Audio->layer==1 || Audio->layer==2) {
						  switch (Audio->modext) {
							  case 0: printf("(Intensity stereo on bands 4-31/32)\n"); break;
							  case 1: printf("(Intensity stereo on bands 8-31/32)\n"); break;
							  case 2: printf("(Intensity stereo on bands 12-31/32)\n"); break;
							  case 3: printf("(Intensity stereo on bands 16-31/32)\n"); break;
						  }
					  } else {
						  //mp3
						  switch (Audio->modext){
							  case 0: printf("(Intensity stereo off, M/S stereo off)\n"); break;
							  case 1: printf("(Intensity stereo on, M/S stereo off)\n"); break;
							  case 2: printf("(Intensity stereo off, M/S stereo on)\n"); break;
							  case 3: printf("(Intensity stereo on, M/S stereo on)\n"); break;
						  }
					  }
					  break;
			case 2: printf("  Dual Channel,"); break;
			case 3: printf("  Mono,"); break;
		}

		switch (Audio->emphasis_index){
			case 0: printf("  No emphasis,"); break;
			case 1: printf("  Emphasis: 50/15 microsecs,"); break;
			case 2: printf("  Emphasis Unknown,"); break;
			case 3: printf("  Emphasis CCITT J 17,"); break;
		}

		if (Audio->copyright) printf("  (c),");
		if (Audio->original) printf("  original\n");
		else printf("  copy\n");

		if (Audio->tag) {
			//print id3 tag----------------
			if (Audio->tag->comment[28] == 0)
				printf ("  ID3 v1.1 tag\n     ----------------\n");
			else
				printf("  ID3 v1.0 tag\n     ----------------\n");
			if (Audio->tag->name[0])
				printf("     title   : %.30s\n", Audio->tag->name);
			if (Audio->tag->artist[0])
				printf("     artist  : %.30s\n", Audio->tag->artist);
			if (Audio->tag->album[0]) {
				printf("     album   : %.30s\n", Audio->tag->album);
				if(Audio->tag->comment[28]==0)
					printf("     track   : %d\n", Audio->tag->comment[29]);
			}
			if (Audio->tag->year[0])
				printf("     year    : %.4s\n", Audio->tag->year);
			if (Audio->tag->comment[0])
				printf("     comment : %.30s\n", Audio->tag->comment);
			if (Audio->tag->genre < MAX_ID3_GENRE)
				printf("     genre   : %s\n", genre[Audio->tag->genre]);
			printf("     ----------------\n");
		}

		if (start_with_id3) PrintID3();
		printf("\n");
		return;
	}

	if (MpegType == mpeg_VIDEO) {
		SecsToHMS(HMS, Video->duration);
		printf("%s\n", FileName);
		printf("  Mpeg %d Video File\n  Estimated Duration: %s\n", mpeg_version, HMS);

		switch(Video->aspect_ratio) {
			case 0: printf("  Invalid aspect ratio (forbidden)\n"); break;
			case 1: printf("  Aspect ratio 1/1 (VGA)\n"); break;
			case 2: printf("  Aspect ratio 4/3 (TV)\n"); break;
			case 3: printf("  Aspect ratio 16/9 (large TV)\n"); break;
			case 4: printf("  Aspect ratio 2.21/1 (Cinema)\n"); break;
#ifdef _DEBUG_
			default: printf("  Invalid Aspect ratio (reserved)\n");
#endif

		}
		if (SExt) {
			if (SExt->progressive) printf("  Not interlaced, chroma format: ");
			else printf("  Interlaced, chroma format: ");
			switch (SExt->chroma_format) {
				case 1 : printf ("4:2:0\n"); break;
				case 2 : printf ("4:2:2\n"); break;
				case 3 : printf ("4:4:4\n"); break;
			}
		}
		if (DExt){
			switch (DExt->video_format) {
				case 0 : printf ("  Video Format: Component\n"); break;
				case 1 : printf ("  Video Format: PAL\n"); break;
				case 2 : printf ("  Video Format: NTSC\n"); break;
				case 3 : printf ("  Video Format: SECAM\n"); break;
				case 4 : printf ("  Video Format: MAC\n"); break;
				case 5 : printf ("  Video Format: Unspecified\n"); break;
			}
			if (	(DExt->h_display_size != Video->hsize) ||
					(DExt->v_display_size != Video->vsize))
				printf("  Display Size [%ld x %ld]\n",
						DExt->h_display_size,
						DExt->v_display_size);
		}
		printf("  Size [%ld x %ld]     %0.2f fps    %.2f Mbps\n",
				Video->hsize,Video->vsize,
				Video->frame_rate,
				Video->bitrate/2500.0);
		if (UData)
			printf("\nUser Data:\n------------\n%s------------\n\n",UData->ud);
		return;
	}

	if (MpegType == mpeg_SYSTEM ) {
		if (composite) {
			printf("  Mpeg System File [%d Video/ %d Audio]\n", n_video,n_audio);
		} else {
			if (HasVideo) {
				SecsToHMS(HMS, Video->duration);
				printf("%s\n", FileName);
				if (mpeg_version == 1) {
					printf("  Mpeg 1 System File [Video");
				}
				else {
					if (mpeg_version!=2);
					printf("  Mpeg 2 Program Stream File [Video");
				}
				if (HasAudio) printf("/Audio]\n");
				else printf("]\n");

				printf("  Muxrate : %.2f Mbps\n", (System->muxrate * 8.0) / 1000000);

				printf("  Estimated Duration: %s\n", HMS);
				switch (Video->aspect_ratio) {
					case 0: printf("  Invalid aspect ratio (forbidden)\n"); break;
					case 1: printf("  Aspect ratio 1/1 (VGA)\n"); break;
					case 2: printf("  Aspect ratio 4/3 (TV)\n"); break;
					case 3: printf("  Aspect ratio 16/9 (large TV)\n"); break;
					case 4: printf("  Aspect ratio 2.21/1 (Cinema)\n"); break;
#ifdef _DEBUG_
					default: printf("  Invalid Aspect ratio (reserved)\n");
#endif

				}

				if (SExt){
					if (SExt->progressive) printf("  Not interlaced, chroma format: ");
					else printf("  Interlaced, chroma format: ");
					switch (SExt->chroma_format){
						case 1 : printf ("4:2:0\n");break;
						case 2 : printf ("4:2:2\n");break;
						case 3 : printf ("4:4:4\n");break;
					}
				}
				if (DExt){
					switch(DExt->video_format){
						case 0 : printf ("  Video Format: Component\n");break;
						case 1 : printf ("  Video Format: PAL\n");break;
						case 2 : printf ("  Video Format: NTSC\n");break;
						case 3 : printf ("  Video Format: SECAM\n");break;
						case 4 : printf ("  Video Format: MAC\n");break;
						case 5 : printf ("  Video Format: Unspecified\n");break;
					}
					if ((DExt->h_display_size != Video->hsize)||
							(DExt->v_display_size != Video->vsize))
						printf("  Display Size [%ld x %ld]\n",
								DExt->h_display_size,DExt->v_display_size);
				}

				printf("  Size [%ld x %ld]     %0.2f fps    %.2f Mbps\n",
						Video->hsize,Video->vsize,
						Video->frame_rate,
						(Video->bitrate/2500.0));
			}
			if(UData )	printf("\nUser Data:\n------------\n%s------------\n\n",UData->ud);
			if (HasAudio){
				if (Audio->mpeg_ver !=3) printf("  Audio : Mpeg %d layer %d\n",Audio->mpeg_ver,Audio->layer);
				else printf("  Audio : Mpeg 2.5 (rare) layer %d\n",Audio->layer);
				if (Audio->bitrate) printf("  %d kbps  %d Hz\n",Audio->bitrate,Audio->sampling_rate);
				else printf("  free bitrate %d Hz\n",Audio->sampling_rate);

				switch (Audio->mode){
					case 0: printf("  Stereo,"); break;
					case 1: printf("  Joint Stereo: ");
							  if (Audio->layer==1 || Audio->layer==2){
								  switch (Audio->modext){
									  case 0: printf("(Intensity stereo on bands 4-31/32)\n"); break;
									  case 1: printf("(Intensity stereo on bands 8-31/32)\n"); break;
									  case 2: printf("(Intensity stereo on bands 12-31/32)\n"); break;
									  case 3: printf("(Intensity stereo on bands 16-31/32)\n"); break;
								  }
							  }
							  else {
								  //mp3
								  switch (Audio->modext){
									  case 0: printf("(Intensity stereo off, M/S stereo off)\n"); break;
									  case 1: printf("(Intensity stereo on, M/S stereo off)\n"); break;
									  case 2: printf("(Intensity stereo off, M/S stereo on)\n"); break;
									  case 3: printf("(Intensity stereo on, M/S stereo on)\n"); break;
								  }

							  }
							  break;
					case 2: printf("  Dual Channel,"); break;
					case 3: printf("  Mono,"); break;
				}

				switch (Audio->emphasis_index){
					case 0: printf("  No emphasis\n"); break;
					case 1: printf("  Emphasis: 50/15 microsecs\n"); break;
					case 2: printf("  Emphasis Unknown\n"); break;
					case 3: printf("  Emphasis CCITT J 17\n"); break;
				}
			}
		}
		return;
	}

	if (MpegType== mpeg_TRANSPORT){
		printf("%s\n",FileName);
		if (!Transport) {
			fprintf(stdout,"  Invalid MPEG 2 Transport Stream\n");
			return;
		}
		Transport->PrintInfos();
		return;
	}

	printf("%s can not be handled by this program\n",FileName);
	return;
} //PrintInfos









// convert a time in second to HH:mm:ss notation
void mpeg::SecsToHMS(char* HMS, float duration){
	byte hours = (byte)(duration / 3600);
	byte mins = (byte)((duration / 60) - (hours * 60));
	float secs = duration - 60 * mins - 3600 * hours;
	if (hours != 0) {
		sprintf (HMS,"%02d:%02d:%05.2fs",hours,mins,secs);
		return;
	}
	if (mins != 0) {
		sprintf(HMS,"%02d:%05.2fs",mins,secs);
		return;
	}
	sprintf(HMS,"%05.2fs",secs);
}









// find next 0x 00 00 01 xx sequence, returns offset or -1 on err
#ifndef _WIN32
#ifndef _MACOSX
inline
#endif
#endif

off_t mpeg::FindNextMarker(off_t from){
	off_t offset;
	for (offset = from; offset < (FileSize - 4); offset++)
	{
		if (
				(GetByte(offset + 0) == 0x00) &&
				(GetByte(offset + 1) == 0x00) &&
				(GetByte(offset + 2) == 0x01))
		{
			return offset;
		}
	}
	return -1;
}






// find next 0x 00 00 01 xx sequence, returns offset or -1 on err and
// change mark to xx
#ifndef _WIN32
#ifndef _MACOSX
inline
#endif
#endif

off_t mpeg::FindNextMarker(off_t from, marker* mark){
	off_t offset = FindNextMarker(from);
	if (offset >= 0) {
		*mark = GetByte(offset + 3);
		return offset;
	}
	else {
		return -1;
	}
}








// find next 0X00 00 01 mark
#ifndef _WIN32
#ifndef _MACOSX
inline
#endif
#endif


off_t mpeg::FindNextMarker(off_t from, marker mark){
	off_t offset = from;
	while(offset >= 0){
		offset = FindNextMarker(offset);
		if (offset == -1) {
			return -1;
		}
		if (EnsureMPEG(offset, mark)) {
			return offset;
		}
		else offset++;
	}

	//shouldn't be here
	return -1;
}







// same as above but optimized for backward search
#ifndef _WIN32
#ifndef _MACOSX
inline
#endif
#endif

off_t mpeg::bdFindNextMarker(off_t from, marker mark) {
	off_t offset;
	for( offset = from; offset >= 0; offset--)
	{
		if(
			(bdGetByte(offset) == 0x00) &&
			(bdGetByte(offset+1) == 0x00) &&
			(bdGetByte(offset+2) == 0x01) &&
			(bdGetByte(offset+3) == mark))
		{
			return offset;
		}
	}
	return -1;
}

#ifndef _WIN32
#ifndef _MACOSX
inline
#endif
#endif

off_t mpeg::bdFindNextMarker(off_t from, marker* mark) {
	off_t offset;
	for (offset = from; offset >= 0; offset--)
	{
		if (	(bdGetByte(offset)  ==0x00) &&
				(bdGetByte(offset+1)==0x00) &&
				(bdGetByte(offset+2)==0x01))
		{
			*mark=bdGetByte(offset+3);
			return offset;
		}
	}
	return -1;

}

// read Time Stamp at given offset
// NOTE: The guys at MPEG are really mad.
//       look at what we have to do to get a timestamp :/

#ifndef _WIN32
#ifndef _MACOSX
inline
#endif
#endif

double mpeg::ReadTS(off_t offset) {
	byte highbit;
	unsigned long low4Bytes;
	double TS;

	highbit = (GetByte(offset) >> 3) & 0x01;

	low4Bytes = ((GetByte(offset) >> 1) & 0x03) << 30;
	low4Bytes |= GetByte(offset + 1) << 22;
	low4Bytes |= (GetByte(offset + 2) >> 1) << 15;
	low4Bytes |= GetByte(offset + 3) << 7;
	low4Bytes |= GetByte(offset + 4) >> 1;

#define FLOAT_0x10000 (double)((unsigned long)1 << 16)
#define STD_SYSTEM_CLOCK_FREQ (unsigned long)90000

	TS =  (double)(highbit * FLOAT_0x10000 * FLOAT_0x10000);
	TS += (double)(low4Bytes);
	TS /= (double)(STD_SYSTEM_CLOCK_FREQ);

	return TS;
}

#ifndef _WIN32
#ifndef _MACOSX
inline
#endif
#endif

double mpeg::ReadTSMpeg2(off_t offset){
	byte highbit;
	unsigned long low4Bytes;
	unsigned long sys_clock_ref;
	double TS;
#ifdef _DEBUG_
	// fprintf(stderr,"Offset: [" _OFF_x "] The bytes...: %2x %2x %2x %2x %2x %2x",offset,GetByte(offset + 0),GetByte(offset + 1),GetByte(offset + 2),GetByte(offset + 3),GetByte(offset + 4),GetByte(offset + 5) );
#endif

	highbit = (GetByte(offset)&0x20)>>5;

	low4Bytes= 		((GetByte(offset) & 0x18) >> 3)	<< 30;
	low4Bytes |= 	 (GetByte(offset) & 0x03) << 28;
	low4Bytes |=	  GetByte(offset + 1	) << 20;
	low4Bytes |= 	 (GetByte(offset + 2	) & 0xF8) << 12;
	low4Bytes |=	 (GetByte(offset + 2	) & 0x03) << 13;
	low4Bytes |=	  GetByte(offset + 3	) << 5;
	low4Bytes |=	 (GetByte(offset + 4	)) >> 3;

	sys_clock_ref=(GetByte(offset + 4) & 0x3) << 7;
	sys_clock_ref|=(GetByte(offset + 5) >> 1);

	TS =  (double)(highbit * FLOAT_0x10000 * FLOAT_0x10000);
	TS += (double)(low4Bytes);
	if (sys_clock_ref ==0)	TS /= (double)(STD_SYSTEM_CLOCK_FREQ);
	else {
		// this is what I understood... CHECK
		// if not zero, then we have a 27 MHz accuracy with a max of 27 MHz
		// so clockfreq might well be 27MHz / sys_clock_ref
		//  nonsense: TS /= (double)(27000000 / sys_clock_ref);
		TS /= (double)(STD_SYSTEM_CLOCK_FREQ);
	}



	//debug
#ifdef _DEBUG_
	// printf("read TS2 : %lf, base : 0x%d,%032lx ref : %016lx\n",TS,highbit,low4Bytes,sys_clock_ref);
#endif
	return TS;
}

off_t mpeg::SkipPacketHeader(off_t myoffset){
	byte mybyte;
	if (mpeg_version == 1) {
		// skip startcode and packet size
		myoffset += 6;
		//remove stuffing bytes
		mybyte = GetByte(myoffset);
		while (mybyte & 0x80) mybyte = GetByte(++myoffset);

		if ((mybyte & 0xC0) == 0x40) // next two bits are 01
			myoffset += 2;

		mybyte=GetByte(myoffset);
		if			((mybyte & 0xF0) == 0x20) myoffset += 5;
		else if	((mybyte & 0xF0) == 0x30) myoffset += 10;
		else myoffset++;

		return myoffset;
	}
	else if (mpeg_version==2) {
		// this is a PES, easyer
		// offset + 9 is the header length (-9)
		return (myoffset + 9 +GetByte(myoffset + 8));
	}
	else return (myoffset + 10);
}


bool mpeg::ParseSystemPacket(off_t startofpacket, off_t startofpack){
	int size = Read2Bytes(startofpacket + 4);
	int i;
	byte code;
	size -= 6;
	//TODO check if there's already a system packet
	if ((size%3) != 0) return false;
#ifdef _DEBUG_
	else printf("%d streams found\n", size/3);
#endif

	for (i=0; i < size/3; i++) {
		code = GetByte(startofpacket + 12 + i * 3);
#ifdef _DEBUG_
		printf("%02x ",code);
#endif

		if ((code&0xF0)==0xC0)
			n_audio++;
		else if ((code&0xF0)==0xE0 || (code & 0xF0)==0xD0)
			n_video++;
	}

	if (n_audio) {
		HasAudio = true;
		if (n_audio > 1) {
			composite = true;
			editable = false;
		}
	}
	if (n_video) {
		HasVideo = true;
		if (n_video > 1){
			composite = true;
			editable = false;
		}
	}
#ifdef _DEBUG_
	printf("\n audio %d video %d \n",n_audio,n_video);
#endif

	return true;
}


void mpeg::ParseFramesInGOP(off_t offset){
	int pict_ref;
	int pict_type;
	char type;
	marker mark;
	off_t off = offset + 1;

	off = FindNextMarker(off , &mark);

	while (off > 0) {

		switch (mark) {
			case 0xB8 :

				printf("GOP ends at [" _OFF_x "]\n",off);

				return;
			case 0x00 :
				//picture
				pict_ref=(GetByte(off+4)<<2);
				pict_ref |= ((GetByte(off+5)&0xC0)>>6);
				pict_type = ((GetByte(off+5)&0x38)>>3);
				switch (pict_type){
					case 1: type ='I';break;
					case 2: type ='P';break;
					case 3: type ='B';break;
					default:type ='U';break;
				}

				printf("\n\n%c (%.2d)     [" _OFF_x "]\n", type, pict_ref, off);


				break;
			case 0xba: printf("            PACK    [" _OFF_x "]\n", off); break;
			case VideoPkt: printf("            Video   [" _OFF_x "]\n", off); break;
			case AudioPkt: printf("            Audio   [" _OFF_x "]\n", off); break;
			case PaddingPkt: printf("            Padding [" _OFF_x "]\n", off); break;

		}
		off++;
		off=FindNextMarker(off,&mark);
	}
}



bool mpeg::ParseExtension(off_t myoffset) {
	myoffset+=4;
	switch (GetByte(myoffset) >> 4) {
		case 1:  return ParseSequenceExt(myoffset);
		case 2:  return ParseSequenceDisplayExt(myoffset);
	}
	return false;
}


bool mpeg::ParseUserData(off_t myoffset) {
	off_t end = FindNextMarker(myoffset + 1);
	off_t i;
	bool allprintable=true;
	byte car;
	off_t mysize = end - myoffset - 4;

	if (mysize <= 0) return false;

	for (i = myoffset+4 ; i < end; i++) {
		car=GetByte(i);
		if ((car < 0x20) && (car != 0x0A) && (car != 0x0D)) {
			allprintable = false;
			break;
		}
	}

	if (allprintable) {
		if  (!UData) {
			UData = new user_data;
			UData->ud = new char[1];
			UData->ud[0] = 0;
			UData->ud_length = 1;
		}
		char* temp = new char[mysize+UData->ud_length+1]; //1 is for \n
		//copy all until \0
		for ( i=0; i < UData->ud_length-1; i++){
			temp[i] = UData->ud[i];
		}
		//okay now copy the new string
		for ( i=0; i < mysize; i++){
			temp[i + UData->ud_length -1] = GetByte(myoffset + 4 + i);
		}

		temp[UData->ud_length- 1 + mysize] = '\n';
		temp[UData->ud_length + mysize] = '\0';
		UData->ud_length += mysize + 1;
		delete[] UData->ud;
		UData->ud = temp;
	}
	return true;
}

bool mpeg::ParseSequenceExt(off_t myoffset) {
	unsigned long hsize;
	unsigned long vsize;
	unsigned long bitrate;
	// We are an mpeg 2 file
	mpeg_version=2;
	if (!SExt) SExt = new sequence_ext;

	//13TH
	if (GetByte(myoffset + 1) & 0x08) SExt->progressive = true;
	SExt->chroma_format = (GetByte(myoffset + 1) & 0x06) >> 1;
	hsize = (GetByte(myoffset + 1) & 0x01) << 1;
	hsize |= (GetByte(myoffset + 2) & 80) >> 7;
	hsize <<= 12;
	if (!Video) return false;
	Video->hsize |= hsize;
	vsize = (GetByte(myoffset+2)&0x60) << 7;
	Video->vsize |= vsize;
	bitrate = (GetByte(myoffset + 2) & 0x1F) << 7;
	bitrate |= (GetByte(myoffset+3)&0xFE) >> 1;
	bitrate <<= 18;
	Video->bitrate |= bitrate;
	if (GetByte(myoffset + 5) & 0x80) SExt->low_delay = true;
	else SExt->low_delay = false;

	byte frate_n = (GetByte(myoffset + 5) & 0x60) >> 5;
	byte frate_d = (GetByte(myoffset + 5) & 0x1F);
	frate_n++;
	frate_d++;
	Video->frame_rate = (Video->frame_rate * frate_n) / frate_d;
	return true;
}

bool mpeg::ParseSequenceDisplayExt(off_t myoffset){
	if (!DExt) DExt = new display_ext;
	DExt->video_format  = (GetByte(myoffset) & 0x0E) >> 1;
	if (GetByte(myoffset) & 0x01) {
		// three bytes to read
		DExt->colour_prim = GetByte(myoffset + 1);
		DExt->transfer_char = GetByte(myoffset + 2);
		DExt->matrix_coef = GetByte(myoffset + 3);
		myoffset += 3;
	} else {
		DExt->colour_prim = 0;
		DExt->transfer_char = 0;
		DExt->matrix_coef = 0;
	}
	DExt->h_display_size =	 GetByte(myoffset + 1)				<< 6;
	DExt->h_display_size |=	(GetByte(myoffset + 2) & 0xFC)	>> 2;
	DExt->v_display_size =	(GetByte(myoffset + 2) & 0x01)	<< 13;
	DExt->v_display_size |=	 GetByte(myoffset + 3)				<< 5;
	DExt->v_display_size |=	(GetByte(myoffset + 4) & 0xF8)	>> 3;
	return true;
}

bool mpeg::ParseRIFF(){
	// RIFF
	if (	GetByte(0) != 'R' ||
			GetByte(1) != 'I' ||
			GetByte(2) != 'F' ||
			GetByte(3) != 'F')
		return false;

	// 23 march 2001 != changed to == in the following
	// Thank to Volker Moell

	// WAVE
	if (	GetByte(8) == 'W' ||
			GetByte(9) == 'A' ||
			GetByte(10) == 'V' ||
			GetByte(11) =='E') {
		fprintf(stderr,"%s is a Wave file\n", FileName);
		return false;
	}
	// 'AVI '
	if (	GetByte(8)=='A' ||
			GetByte(9)=='V' ||
			GetByte(10) == 'I' ||
			GetByte(11) ==' ') {
		fprintf(stderr,"%s is an AVI file\n",FileName);
	}

	return false;
}

off_t mpeg::FindMatchingAudio(off_t myoffset){
	for( ; myoffset < FileSize - 5; myoffset++) {
		if (GetByte(myoffset) == 0xFF)
			// maybe an audio sync
			if (MatchAudio(myoffset)) return myoffset;
	}
	if (myoffset >= FileSize) return FileSize;
	return -1;
}

off_t mpeg::bdFindMatchingAudio(off_t myoffset){
	for( ; myoffset >= Audio->first_frame_offset; myoffset--) {
		if (bdGetByte(myoffset) == 0xFF)
			if (MatchAudio(myoffset)) return myoffset;
	}
	return -1;
}

bool mpeg::MatchAudio(off_t myoffset){
	// do we have audio anyway?
	if (!Audio) return false;
	//does this start as an mpeg audio frame?
	if (GetByte(myoffset) != 0xFF || (GetByte(myoffset + 1) & 0xE0) != 0xE0)
		return false;
	// are version compatibles?
	int version = ((GetByte(myoffset + 1)) & 0x18) >> 3;
	switch (Audio->mpeg_ver){
		case 1: if (version != 3) return false; break;
		case 2: if (version != 2) return false; break;
		case 3: if (version != 0) return false; break;
		default: return false; break;
	}

	//are layers compatible?
	int layer = (GetByte(myoffset+1)&0x06)>>1;
	switch (Audio->layer) {
		case 1: if (layer != 3) return false; break;
		case 2: if (layer != 2) return false; break;
		case 3: if (layer != 1) return false; break;
		default: return false; break;
	}

	//are mode compatibles?
	int mode = (GetByte(myoffset + 3) & 0xC0) >> 6;
	if (mode != Audio->mode) return false;

	return true;
}

long mpeg::ReadPACKMuxRate(off_t offset) {
	// at start of a PACK
	long muxrate = 0;
	if ((GetByte(offset) & 0xC0) == 0x40) {
		// mpeg2
		muxrate = GetByte(offset + 6) << 14;
		muxrate |= GetByte(offset + 7) << 6;
		muxrate |= GetByte(offset + 8) >> 2;

	} else {
		//maybe mpeg1
		if ((GetByte(offset)&0xF0)!=0x20)


			fprintf(stderr,
					"weird pack header while parsing muxrate (offset " _OFF_x ")\n", offset);


		muxrate = (GetByte(offset + 5) & 0x7F) << 15;
		muxrate |= (GetByte(offset + 6) << 7);
		muxrate |= (GetByte(offset + 7) >> 1);
	}
	muxrate *= 50;
	return muxrate;
}












void mpeg::ParsePAT(off_t offset){
	//	offset++; //skip the first byte
	int pnum,ppid,i;
	int n_progs;
	if (GetByte(offset + 1) != 0x00) {
#ifdef _DEBUG_
		printf("wrong table_id in PAT\n");
#endif
		return;
	}

	// next 4 bits are 10XX
	int length = Read12bitLength(offset + 2);
#ifdef _DEBUG_
	printf("PAT: %d programs in %d bytes described\n",(length-9)/4,length-9);
#endif

	if ( ( (length - 9)%4) != 0) {
#ifdef _DEBUG_
		printf("malformed PAT \n");
#endif
		return;
	}

	n_progs=(length-9)/4;

	if (Transport->PMT_PIDs) delete [] Transport->PMT_PIDs;
	Transport->PMT_PIDs = new PID [n_progs];
	Transport->n_progs = n_progs;
	Transport->delete_programs();
	Transport->programs = new program[n_progs];

	for ( i=0; i < n_progs; i++){
		Transport->programs[i].prog_num = -1;
		Transport->programs[i].nstreams = -1;
		Transport->programs[i].TStreams = 0;
	}


	for ( i=0; i< n_progs; i++){
		pnum = Read2Bytes(offset + 9 + (i*4));
		ppid = ReadPID(offset + 11 + (i*4));
		if (pnum == 0) {
			Transport->network_PID = ppid;
			Transport->n_progs--;
		} else {
			Transport->PMT_PIDs[Transport->n_PMT_PIDs++] = ppid;
		}
	}

}

long mpeg::Read12bitLength(off_t offset){
	// length must be xxxxAAAA AAAAAAAA
	return (((GetByte(offset) & 0x0F) << 8) | GetByte(offset + 1));
}

long mpeg::Read2Bytes(off_t offset){
	return((GetByte(offset) << 8) | GetByte(offset + 1));
}

long mpeg::ReadPID(off_t offset){
	// PID must be xxxAAAAA AAAAAAAA
	return (((GetByte(offset) & 0x1F) << 8) | GetByte(offset + 1));
}

void mpeg::ParseCAT(off_t offset){
	// conditional access table
	if(GetByte(offset + 1) != 0x01) {
		fprintf(stderr,"malformed CAT\n");
		return;
	}
	//	int length=Read12bitLength(offset+2);

	//	long desc_offset=ParseDescriptor(offset+9);
	//	while (desc_offset < offset+length+4-4){
	//		desc_offset=ParseDescriptor(desc_offset);
	//	}

}

void mpeg::ParsePMT(off_t offset){
	off_t i, desc_offset;
	int prog_index=0;

	if (GetByte(offset + 1) != 0x02) {
#ifdef _DEBUG_
		printf("malformed PMT\n");
#endif
		return;
	}

	// next 4 bits are 10XX
	int length = Read12bitLength(offset + 2);
#ifdef _DEBUG_
	printf("PMT length : %d\n",length);
#endif

	int pnum = Read2Bytes(offset + 4);
#ifdef _DEBUG_
	printf("This PMT describes Program N° %d\n",pnum);
#endif


	// check if the program is already here
	for (i = 0; i < Transport->n_progs; i++) {
		if (Transport->programs[i].prog_num == pnum) {
			// already have info on this!
			fprintf(stderr, "Warning, prog num %d redefined ! \n", pnum);
			if (Transport->programs[i].TStreams != 0)
				Transport->delete_ES(Transport->programs[i].TStreams);
			Transport->programs[i].TStreams = 0;
		}
	}

	// fetch the first empty index
	for (i = 0; i < Transport->n_progs; i++){
		if (Transport->programs[i].prog_num == -1) {
			prog_index = i;
		}
	}

	if (prog_index >= Transport->n_progs) {
		fprintf(stderr,"Error : no room for program %d\n", pnum);
		return;
	}

	program* temp = &(Transport->programs[prog_index]);
	temp->prog_num = pnum;
	temp->nstreams = 0;
	//	temp->next=Transport->programs;
	//	Transport->programs=temp;
	//	Transport->n_progs++;

	//	temp->prog_num=pnum;

#ifdef _DEBUG_
	int pcrpid = ReadPID(offset + 9);
	if (pcrpid != 0x1FFF) printf("PCR of this program is in PID %d\n", pcrpid);
#endif

	int infolen = Read12bitLength(offset + 11);
#ifdef _DEBUG_
	printf("info length : %d\n", infolen);
#endif

	//parsing descriptors
	if (infolen > 0) {
		desc_offset = ParseDescriptor(offset + 12, &(temp->descs));
		while (desc_offset < (offset + 12 + infolen)){
			desc_offset = ParseDescriptor(desc_offset, &(temp->descs));
		}
	}


	desc_offset = (offset + 13 + infolen);
	off_t max_offset = offset + 4 + length - 4; //-4 is for CRC
	size_t desc_size;
	int ES_type;
	int ES_pid;
	int ES_infolen;

	for ( i = desc_offset;i<max_offset;i+=desc_size){
		ES_type = GetByte(i);
		ES_pid = ReadPID(i + 1);
		ES_infolen = Read12bitLength(i + 3);
		desc_size = 5 + ES_infolen;
#ifdef _DEBUG_
		printf("Program [%d]: contains stream : ", pnum);
#endif

		temp->nstreams++;
		EStream* tempES = new EStream;
		tempES->next = temp->TStreams;
		temp->TStreams = tempES;
		tempES->pid = ES_pid;
		tempES->type = ES_type;
		tempES->demuxFile = 0;
		tempES->demuxFileOk = true;
		/*		switch(ES_type){
				case 0 : printf("reserved"); break;
				case 1: printf("MPEG 1 video");break;
				case 2: printf("MPEG 2 video");break;
				case 3: printf("MPEG 1 audio");break;
				case 4: printf("MPEG 2 audio");break;
				case 5: printf("MPEG 2 private section");break;
				case 6: printf("MPEG 2 PES with private data");break;
				case 7: printf("MHEG");break;
				case 8: printf("DSM_CC");break;
				case 9: printf("Private data");break;
				default: if (ES_type <0x80) printf("MPEG 2 reserved");
				else printf("User Private data");
				break;
				}
		 */
		/*		printf(" info@%4lx (%d bytes)  PID [%d]\n",i+5,ES_infolen,ES_pid);*/

		if (ES_infolen > 0) {
			desc_offset = ParseDescriptor(i + 5, &tempES->descs);
			while (desc_offset < (i + 5 + ES_infolen))
				desc_offset = ParseDescriptor(desc_offset, &tempES->descs);
		}
	}
}


off_t mpeg::ParseDescriptor(off_t offset,mpeg_descriptors* target){
	int tag = GetByte(offset);
	int length = GetByte(offset+1);
#ifdef _DEBUG_
	printf("Descriptor %d length %d\n",tag,length);
#endif

	bool handled = false;
	switch (tag) {
		case 0:
		case 1: //printf("reserved descriptor\n");
			break;
		case 2: //printf("video stream descriptor : video version %d\n",GetByte(offset+2));
			target->video_coding_version = GetByte(offset + 2);
			handled = true;
			break;
		case 3: //printf("audio stream descriptor : audio version %d\n",GetByte(offset+2));
			target->audio_coding_version = GetByte(offset + 2);
			handled = true;
			break;
		case 4: //printf("hierarchy descriptor\n");
			break;
		case 5: //printf("registration descriptor\n");
			break;
		case 6: //printf("data stream alignment descriptor\n");
			break;
		case 7: //printf("target background grid descriptor\n");
			break;
		case 8: //printf("video window descriptor\n");
			break;
		case 9: DescCA(offset, target);
				  handled = true;
				  break;
		case 10:DescLang(offset, target);
				  handled = true;
				  break;
		case 11: //printf("System clock descriptor\n");
				  break;
		case 12: //printf("multiplex buffer utilization descriptor\n");
				  break;
		case 13: //printf("copyright descriptor\n");
				  target->copyright = true;
				  break;
		default :
#ifdef _DEBUG_
				  if (tag < 63) printf("MPEG 2 reserved descriptor\n");
				  else printf("User Private descriptor\n");
#endif
				  break;
	}
	if (!handled) {
		/*
			for (int i=0;i<length;i++){
			printf("%02x ",GetByte(offset+i+2));
			}
			printf("\n");
		 */
		target->n_unhandled_desc++;
	}
#ifdef _DEBUG_
	printf("\n");
#endif

	return (offset + length + 2);
}

void mpeg::DescCA(off_t offset, mpeg_descriptors* target){
#ifdef _DEBUG_
	printf("Conditional Access descriptor\n");
#endif

	//	int length=GetByte(offset+1);
	int CAsysID = Read2Bytes(offset + 2);
	PID CA_PID = ReadPID(offset + 4);
	target->CA = CAsysID;
	target->CA_PID = CA_PID;

	/*	printf("Conditional access ID : %d  related PID is %d\n",CAsysID,CA_PID);
		if (length>4){
		printf("private data:\n");
		byte thebyte;
		for (long i=offset+6;i<offset+2+length;i++){
		thebyte=GetByte(i);
		if (thebyte>=0x20) printf("%02x ",thebyte);
		else printf("%02x ",thebyte);
		}
		printf("\n");

		for (long i=offset+6;i<offset+2+length;i++){
		thebyte=GetByte(i);
		if (thebyte>=0x20) printf(" %c ",thebyte);
		else printf("   ");
		}
		printf("\n");
		}
	 */
}


void mpeg::DescLang(off_t offset, mpeg_descriptors* target){
	int i;
#ifdef _DEBUG_
	printf("Language descriptor (untested)\n");
#endif

	int length = GetByte(offset + 1);
	int audio_type = GetByte(offset + 1 + length);
	target->lang_audio_type=audio_type;
	if (length > 1) {
		// there are languages inside
		length--;	//without the trailing audio_type
		if (length%3 != 0) {
			fprintf(stderr,"invalid language in descriptor encountered\n");
			return;
		}
		target->languages = new char[length + 1];
		for (i=0; i < length; i++)
		{
			target->languages[i] = GetByte(offset + 2 + i);
		}
		target->languages[length] = '\0';
		// length /=3;
#ifdef _DEBUG_
		printf("languages(s) : ");
#endif

		/*		for (i=0; i < length; i++){
				printf("%c%c%c ",
				GetByte(offset+2+(i*3)),
				GetByte(offset+3+(i*3)),
				GetByte(offset+4+(i*3)));
				}
				printf("\n");
		 */
	}
}



bool mpeg::ParseTransportStream(off_t offset){
	if (GetByte(offset) != 0x47) return false;
	//	int packetnum=0;
	//	int pmt;
	Transport = new transport;
	int pid, scramble, adaptation;
	int i;
	off_t payload_offset;
	// well this is a mpeg 2? transport stream
	mpeg_version = 2;

#define MPEG2_TR_PKT_LENGTH 188
#define TRANSPORT_PAT 0
#define TRANSPORT_CAT 1

#define PAT_SID 0
#define CAT_SID 1
#define PMT_SID 2

	for (; offset < FileSize-1; offset += MPEG2_TR_PKT_LENGTH) {
		if (GetByte(offset) != 0x47) {
			fprintf(stderr,"Bad Packet start code %#0x should be 0x47\n",GetByte(offset));
			return false;
		}
		pid = ReadPID(offset + 1);
		scramble = GetByte(offset + 3) >> 6;
		adaptation = (GetByte(offset + 3) & 0x30) >> 4;
		if (adaptation == 0) {
			// this is reserved, skip!
			continue;
		}

		//compute the payload offset
		payload_offset = offset + 4;
		if (adaptation & 0x02){
			//adaptation fiel is present.
			// skip it
			// first byte of adaptation field is adaptation field length
			payload_offset += GetByte(offset + 4) + 1;
		}

		// if this is a Program association table parse it
		if (pid == 0x00) {
			ParsePAT(payload_offset);
		}
		// if this is a Program Map Table parse it
		for (i = 0; i < Transport->n_PMT_PIDs; i++) {
			if (Transport->PMT_PIDs[i] == pid) {
				ParsePMT(payload_offset);
				Transport->read_pmts++;
#ifdef _DEBUG_
				printf("PMT at [" _OFF_x "]\n", payload_offset);

#endif

				break;
			}
		}

		if (Transport->n_PMT_PIDs !=0 && Transport->n_PMT_PIDs == Transport->read_pmts) {
			// we read all program map table...
			//			Transport->PrintInfos();
			HasAudio=HasVideo = true;
			return true;

		}
		/*		switch(GetByte(payload_offset+1)){
				case PAT_SID :
				ParsePAT(payload_offset);
				break;
				case CAT_SID :
				ParseCAT(payload_offset);
				break;
				case PMT_SID :
				ParsePMT(payload_offset);
				break;
				default : continue; break;
				}
				printf ("\n\n");
		 */

	}


#ifdef _DEBUG_
	fprintf(stderr,"Sorry MPEG-2 Transport Stream is currently not handled\n");
#endif

	fprintf(stderr,"Warning didn't find the promised Program Map Tables\n");
	return false;
}


PID  mpeg::NextTrPacket(off_t* offset,off_t* payload_start, off_t* payload_end){
	PID pid;
	byte adaptation;
	off_t payload_offset;
	*payload_start = 0;
	*payload_end = 0;
	// *offset = *offset + MPEG2_TR_PKT_LENGTH;
	//	*offset = *offset + 188;
	pid = ReadPID(*offset + 1);
	adaptation = (GetByte(*offset + 3) & 0x30) >> 4;
	if (adaptation == 0){
		// this is reserved, skip!
		*offset += MPEG2_TR_PKT_LENGTH;
		return pid;
	}
	if (pid == 0x1FFF) {
		//null packet
		*payload_start = 0;
		*payload_end = 0;
		*offset += MPEG2_TR_PKT_LENGTH;
		return pid;
	}
	//compute the payload offset
	payload_offset = *offset + 4;
	if (adaptation & 0x02) {
		//adaptation fiel is present.
		// skip it
		// first byte of adaptation field is adaptation field length
		payload_offset += GetByte(*offset + 4) + 1;
	}
	*payload_start = payload_offset;
	*offset += MPEG2_TR_PKT_LENGTH;
	*payload_end = *offset;

	if (*offset >= FileSize)
		*payload_start = *payload_end=-1;

	return pid;
}




transport::transport()
	:programs(0),
	n_progs(0),
	n_audio_streams(0),
	n_video_streams(0),
	n_other_streams(0),
	network_PID(-1),
	PMT_PIDs(0),
	n_PMT_PIDs(0),
read_pmts(0)
{};

transport::~transport() {
	if (n_progs >0) {
		delete [] programs;
	}
}

void transport::delete_programs(){
	if (!programs) return;
	for (int i = 0; i < n_progs; i++){
		if (programs[i].TStreams != 0) delete_ES(programs[i].TStreams);
	}
	delete[] programs;
	programs=0;
}

void transport::delete_ES(EStream* stream) {
	if (stream == 0) return;
	if (stream->next == 0) delete stream;
	else delete_ES(stream->next);
}
void transport::PrintInfos(){
	EStream* current;
	if (n_progs == 1)
		printf("  Mpeg 2 Transport Stream [1 program]\n");
	else
		printf("  Mpeg 2 Transport Stream [%d programs]\n",n_progs);

	for(int i = 0; i < n_progs; i++) {
		printf("    Program N° %d contains %d Elementary Streams:\n"
				,programs[i].prog_num, programs[i].nstreams);
		programs[i].descs.PrintInfos("      ");
		current=programs[i].TStreams;
		int stream_count = 1;
		while (current != 0) {
			printf("      Stream %d: ", stream_count++);
			switch (current->type) {
				case 0: printf("reserved"); break;
				case 1: printf("MPEG 1 video");break;
				case 2: printf("MPEG 2 video");break;
				case 3: printf("MPEG 1 audio");break;
				case 4: printf("MPEG 2 audio");break;
				case 5: printf("MPEG 2 private section");break;
				case 6: printf("MPEG 2 PES with private data");break;
				case 7: printf("MHEG");break;
				case 8: printf("DSM_CC");break;
				case 9: printf("Private data");break;
				default: if (current->type <0x80) printf("MPEG 2 reserved");
								else printf("User Private data");
							break;
			}
			printf(" [pid: %d]\n", current->pid);
			current->descs.PrintInfos("        ");
			current=current->next;
		}
	}
}

void mpeg_descriptors::PrintInfos(char* prefix){
	unsigned int i;
	if (prefix==0) prefix = "";
	char* description = new char[300];
	char temp[100];
	description[0] = '\0';
	if (video_coding_version != -1) {
		sprintf(temp, "%svideo version %d\n", prefix, video_coding_version);
		strcat(description,temp);
	}
	if (audio_coding_version != -1) {
		sprintf(temp,"%saudio version %d\n",prefix, audio_coding_version);
		strcat(description, temp);
	}
	if (CA != -1) {
		sprintf(temp,"%smaybe scrambled (CA ID %d)\n",prefix,CA);
		strcat(description, temp);
	}
	if (languages) {
		sprintf(temp,"%sstream language: ", prefix);
		for (i = 0; i < strlen(languages)/3; i++) {
			sprintf(temp,"%c%c%c ",languages[i*3], languages[i*3+1], languages[i*3+2]);
			strcat(description,temp);
		}
		strcat(description,"\n");
	}
	if (lang_audio_type >= 1 && lang_audio_type <= 3) {
		switch (lang_audio_type) {
			case 1: sprintf(temp,"%sClean effects : not a language\n",prefix);break;
			case 2: sprintf(temp,"%sStream is prepared for hearing impaired\n",prefix);break;
			case 3: sprintf(temp,"%sStream is prepared for commentaries for visually impaired viewers\n",prefix);
		}
		strcat(description,temp);
	}

	if (copyright) {
		sprintf(temp,"%sThis stream has copyright limitations\n",prefix);
		strcat(description,temp);
	}
	if (n_unhandled_desc>0) {
		if (n_unhandled_desc==1)
			sprintf(temp,"%s1 additional descriptor was not handled\n"
					,prefix);
		else
			sprintf(temp,"%s%d additional descriptors were not handled\n"
					,prefix, n_unhandled_desc);
		strcat(description,temp);
	}
	printf("%s", description);
	delete[] description;
}

header_buf *readHeader(FILE *myMpegfile, off_t offset, int rw)
{

static byte *nix;
static off_t size;
static header_buf *p;

	if (rw == 1)
	{
		p = new header_buf;
		size = offset;
		nix = new byte[size];
		if (!nix)
		{
			fprintf(stderr,"unable to alloc buffer for header\n");
			exit(1);
		}
	        FSEEK(myMpegfile,0L,SEEK_SET);
		fread(nix,size,1L,myMpegfile);
		p->size = size;
		p->buf = nix;
		if (preserve_header)
			fprintf(stderr,"Size of Fix: 0x%qx\n",size);
	return(NULL);
	}
	if (rw == 2)
	{
		return(p);
	}

fprintf(stderr,"unable to understand command\n");
exit(1);
}




void mpeg::print_all_ts(byte kind){

off_t p = 0;
byte *nix;
double tsx;
nix = &kind;

while (p != -1)
{
	p = FindNextMarker(p,kind);
	tsx = ReadTSMpeg2(p +4);
	if (p == -1)
		break;
	fprintf (stderr,"offset:  " _OFF_x " TS: %lf\n",p,tsx);
	p+=4;

}


}
