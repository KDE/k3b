/***************************************************************************
                          chunkTab.cpp  -  description
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

#include "chunkTab.h"
#include <string.h>
#include <stdio.h>


void chunkTab::PrintInfos() {
	int i;
	for ( i = 0; i < current_mpeg; i++)
		MpegTab[i]->PrintInfos();
}


chunk** chunkTab::GetChunks(int* nchunks) {
	*nchunks = current_chunk;
	return TheTab;
}


chunkTab::chunkTab(int _max)
:max(_max), current_mpeg_ptr(0)
{
	max = (max > 0)?max:1;
	TheTab = new chunk*[max];
	current_chunk = current_mpeg=0;
	MpegTab = new mpeg*[max];
}


bool chunkTab::AddFile(char* filename) {
	int i;
	if (!filename) {
		return false;
	}

	for ( i=0; i< current_mpeg;i++){
		if(!strcmp(filename, MpegTab[i]->Name())) {
			current_mpeg_ptr = MpegTab[i];
			return true;
		}
	}


	//if we're here, filename is a new mpeg file
	if (current_mpeg == max) MoreRoom(20);
	mpeg* newmpeg = new mpeg(filename);
	if (!newmpeg->has_audio() && ! newmpeg->has_video()){
		delete newmpeg;
		return false;
	}
	MpegTab[current_mpeg++] = newmpeg;
	current_mpeg_ptr = newmpeg;
	//	current_file=new char[strlen(filename)+1];
	//	strcpy(current_file,filename);
	return true;
}



// this adds some room to the internal tabs
void chunkTab::MoreRoom(int n){
	int i;
	max += n;
	chunk** tempchunk = new chunk*[max];
	mpeg** tempmpeg = new mpeg*[max];

	for ( i = 0; i < current_chunk; i++)
		tempchunk[i] = TheTab[i];
	for ( i = 0; i < current_mpeg; i++)
		tempmpeg[i] = MpegTab[i];

	delete[] TheTab;
	delete[] MpegTab;
	TheTab = tempchunk;
	MpegTab = tempmpeg;
}


bool chunkTab::ParseRange(char* range) {
	char* first_range = range+1;
	char* second_range = range;
	int first_range_size = 0;
	int second_range_size = 0;
	int slash_sep = 0;
	int sep = 0;
	unsigned int i;

	for ( i = 1; i < strlen(range); i++) {
		if (range[i] == '-') {
			sep++;
			second_range = &range[i+1];
			first_range_size = i-1;
		}
		if (range[i] == '/') {
			slash_sep++;
			second_range = &range[i+1];
			first_range_size = i-2;
		}
	}
	second_range_size = strlen(range)-3-first_range_size;

	if ((sep + slash_sep) == 0) {
		fprintf(stderr,"Error : No separator in range %s\n", range);
		return false;
	}

	if (sep > 1) {
		fprintf(stderr,"Invalid range argument %s : too many '-' in format\n", range);
		return false;
	}
	if (slash_sep >1) {
		fprintf(stderr,"Invalid range argument %s : too many '/' in format\n", range);
		return false;
	}
	if (slash_sep + sep >1) {
		fprintf(stderr,"Invalid range argument %s : can't mix '-' and '/'\n", range);
		return false;
	}

	off_t part,nparts;
	chunk* tempchunk = new chunk;
	tempchunk->file = 0;
	tempchunk->from = -1;
	tempchunk->to = -1;
	tempchunk->to_included = false;
	tempchunk->from_included = false;
	tempchunk->until_file_size = false;
	tempchunk->unit_is_second = false;

	if (current_mpeg_ptr != 0)
		tempchunk->mpegfile = current_mpeg_ptr;
	else {
		fprintf(stderr, "Range argument must follow an mpeg file %s \n", range);
		delete tempchunk;
		return false;
	}

	if (current_chunk == max) MoreRoom(20);
	char* offset = range + 1; // after [ or ]

	if (range[0] == ']') tempchunk->from_included = false;
	else tempchunk->from_included = true;

	switch (range[strlen(range) - 1]) {
		case ']':
			tempchunk->to_included = true;
			break;
		case '[':
			tempchunk->to_included = false;
			break;
		default:
			fprintf(stderr, "malformed range argument %s"
				" (range must end with ] or [ )\n", range);
			delete tempchunk;
			return false;
	}

	if (first_range_size == 0 && second_range_size == 0) {
		fprintf(stderr, "Invalid range %s\n",  range);
		delete tempchunk;
		return false;
	}


	if (sscanf(offset,  _OFF_d "/" _OFF_d , &part, &nparts) == 2) {
		if ((part <= 0) || (nparts < 0) || (part > nparts)) {
			fprintf(stderr, "invalid part %s\n", range);
			delete tempchunk;
			return false;
		}
		//		printf("part %d out of %d\n",part,nparts);

		tempchunk->from =
			off_t((((tempchunk->mpegfile->Size())*1.0)/nparts)*(part-1));
		tempchunk->to =
			off_t((((tempchunk->mpegfile->Size())*1.0)/nparts)*(part));
		if (part == nparts) tempchunk->until_file_size = true;
		TheTab[current_chunk++] = tempchunk;
		return true;
	}

	if (slash_sep) {
		fprintf(stderr, "Invalid part %s\n", range);
		delete tempchunk;
		return false;
	}

	if (first_range_size == 0) {
		tempchunk->from = 0;
		tempchunk->sfrom = 0;
		tempchunk->from_included = false;
		if(!ParseValue(second_range, second_range_size,
			&(tempchunk->to), &(tempchunk->sto),
			&(tempchunk->unit_is_second)))
		{
			fprintf(stderr,"Invalid range %s\n",range);
			delete tempchunk;
			return false;
		}
		if (tempchunk->unit_is_second)
		{
			tempchunk->to =
				off_t((tempchunk->sto/tempchunk->mpegfile->Duration())*
						tempchunk->mpegfile->Size());
		}
		TheTab[current_chunk++] = tempchunk;
		return true;
	}

	if (second_range_size == 0) {
		tempchunk->to = tempchunk->mpegfile->Size();
		tempchunk->sto = tempchunk->mpegfile->Duration();
		tempchunk->to_included = true;
		if (!ParseValue(first_range, first_range_size,
			&(tempchunk->from), &(tempchunk->sfrom),
			&(tempchunk->unit_is_second)))
		{
			fprintf(stderr,"Invalid range %s\n",range);
			delete tempchunk;
			return false;
		}
		if (tempchunk->unit_is_second) {
			tempchunk->from =
				off_t((tempchunk->sfrom/tempchunk->mpegfile->Duration()) *
					tempchunk->mpegfile->Size());
		}
		TheTab[current_chunk++] = tempchunk;
		return true;
	}

	bool insecs;
	if(!ParseValue(first_range, first_range_size,
		&(tempchunk->from), &(tempchunk->sfrom),
		&(tempchunk->unit_is_second)))
	{
		fprintf(stderr,"Invalid range %s\n",range);
		delete tempchunk;
		return false;
	}

	if(!ParseValue(second_range, second_range_size,
		&(tempchunk->to), &(tempchunk->sto),&insecs))
	{
		fprintf(stderr,"Invalid range %s\n",range);
		delete tempchunk;
		return false;
	}

	if (insecs != tempchunk->unit_is_second) {
		fprintf(stderr, "Error, mixed seconds with bytes in range %s\n", range);
		delete tempchunk;
		return false;
	}

	if (!insecs) {
		if (tempchunk->from>tempchunk->to) {
			fprintf(stderr,"Invalid range %s : start greater than stop\n", range);
			delete tempchunk;
			return false;
		}
	} else {
		if (tempchunk->sfrom>tempchunk->sto) {
			fprintf(stderr,"Invalid range %s : start greater than stop\n", range);
			delete tempchunk;
			return false;
		}
	}

	//convert time to offset if needed
	if (tempchunk->unit_is_second) {
		if(tempchunk->sfrom != 0) {
			tempchunk->from=
				off_t((tempchunk->sfrom/tempchunk->mpegfile->Duration())*
						tempchunk->mpegfile->Size());
		}
		if(tempchunk->sto != tempchunk->mpegfile->Duration()) {
			tempchunk->to =
				off_t((tempchunk->sto/tempchunk->mpegfile->Duration())*
						tempchunk->mpegfile->Size());
		}
	}

	// is it okay?
	if ((tempchunk->from < 0) ||
			(tempchunk->to < tempchunk->from) ||
			(tempchunk->to > tempchunk->mpegfile->Size()))
	{

		fprintf(stderr, "range %s results in invalid [" _OFF_d "-" _OFF_d "] range\n",
				range, tempchunk->from,tempchunk->to);


		delete tempchunk;
		return false;
	}
	TheTab[current_chunk++] = tempchunk;
	return true;
}



bool chunkTab::ParseValue(
		char* value,
		int value_length,
		off_t* translation,
		float* stranslation,
		bool* time)
{
	int i;
	bool Time=false;
	bool byte=false;
	int nbytes=0;
	int ncolon=0;

	for ( i = 0; i < value_length; i++) {
		switch (value[i]) {
			case 'M':
			case 'k':
				nbytes++;
				byte=true;
				break;
			case ':':
				ncolon++;
				Time=true;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case '.':
				break;
			default :
				return false;
				break;
		}
	}

	if (Time && byte) {
		return false;
	}

	if (ncolon>2){
		return false;
	}

	if (nbytes>1){
		return false;
	}

	if (Time) *time = true;
	else {
		*time = false;
		byte = true;
	}

	if (byte) {

		if (sscanf(value,  _OFF_d , translation) != 1) {
			return false;
		}
		if (nbytes) {
			switch(value[value_length - 1]) {
				case 'M':
					*translation = *translation * 1024 * 1024;
					return true;
					break;
				case 'k':
					*translation = *translation * 1024;
					return true;
					break;
				default:
					return false;
			}
		}
	}
	// okay, let's parse the time

	float m1, m2, m3, start = 0;
	int nconv;
	nconv = sscanf(value,"%f:%f:%f", &m1, &m2, &m3);

	if ((nconv == 0) || (nconv-1 != ncolon)) {
		return false;
	}

	switch (nconv) {
		case 1: start = m1; break;
		case 2: start = m2 + m1 * 60; break;
		case 3: start = m3 + m2 * 60 + m1 * 3600; break;
	}

	*stranslation = start;
	return true;
}



bool chunkTab::ParseBoundaries(char* boundaries) {
	char* start = boundaries + 1;
	int size;
	if (boundaries[strlen(boundaries) - 1] != '}') {
		fprintf(stderr, "Invalid range %s\n", boundaries);
		return false;
	}

	chunk* tc = new chunk;
	if (current_mpeg_ptr != 0)
		tc->mpegfile = current_mpeg_ptr;
	else {
		fprintf(stderr, "Range argument must follow an mpeg file %s\n", boundaries);
		delete tc;
		return false;
	}

	tc->from = 0;
	tc->sfrom = 0;
	tc->from_included = false;
	tc->to_included = true;

	for (unsigned int i = 1; i <= strlen(boundaries) -1; i++) {
		if ((boundaries[i] == '-') || (boundaries[i] == '}')) {
			size = (&boundaries[i] - start);
			if (size <= 0) {
				fprintf(stderr,"Invalid range %s\n",boundaries);
				return false;
			}

			if (!ParseValue(start, size,
					&(tc->to), &(tc->sto), &(tc->unit_is_second)))
			{
				fprintf(stderr,"Invalid range %s\n",boundaries);
				return false;
			}
			if (current_chunk >= max) MoreRoom(max+20);
			if (tc->unit_is_second) {
				if(tc->sfrom != 0) {
					tc->from =
						off_t((tc->sfrom/tc->mpegfile->Duration())*
							tc->mpegfile->Size());
				}
				if(tc->sto != tc->mpegfile->Duration()) {
					tc->to =
						off_t((tc->sto/tc->mpegfile->Duration())*
							tc->mpegfile->Size());
				}
			}

			// is it okay?
			if ((tc->from < 0) ||
				(tc->to < tc->from) ||
				(tc->to > tc->mpegfile->Size()))
			{
				if((tc->to>tc->mpegfile->Size()))
					fprintf(stderr,
						"invalid range %s :boundary below end of file " _OFF_d "\n",
						boundaries, tc->to);
				else
					fprintf(stderr,
						"range %s results in invalid range [" _OFF_d "-" _OFF_d "]\n",
						boundaries, tc->from, tc->to);

				delete tc;
				return false;
			}

			TheTab[current_chunk] = tc;
			tc = new chunk;
			tc->mpegfile = current_mpeg_ptr;
			tc->from = TheTab[current_chunk]->to;
			tc->sfrom = TheTab[current_chunk]->sto;
			current_chunk++;
			tc->from_included = false;
			tc->to_included = true;
			start = &boundaries[i + 1]; //after the '-'
		}
	}
	if (current_chunk >= max) MoreRoom(max + 20);
	tc->from_included = false;
	tc->to_included = true;
	tc->to = tc->mpegfile->Size();
	tc->sto = tc->mpegfile->Duration();
	// is it okay?
	if ((tc->from < 0) ||
		(tc->to < tc->from) ||
		(tc->to > tc->mpegfile->Size()))
	{
		if((tc->to > tc->mpegfile->Size()))
			fprintf(stderr, "invalid range %s :boundary below end of file " _OFF_d "\n",
				boundaries, tc->to);
		else
			fprintf(stderr,"range %s results in invalid range [" _OFF_d "-" _OFF_d "]\n",
				boundaries, tc->from, tc->to);
		delete tc;
		return false;
	}

	TheTab[current_chunk++] = tc;
	return true;
}



void chunkTab::PrintTab() {
	int i;
	printf("%d chunks in %d files\n\n", current_chunk,current_mpeg);

	for (i = 0; i < current_chunk; i++) {
		printf("chunk %.2d : %p\n    ", i, (void*)(TheTab[i]->mpegfile));
		if (TheTab[i]->from_included) printf("[");
		else printf("]");

		printf( _OFF_d "|" _OFF_d , TheTab[i]->from, TheTab[i]->to);

		if (TheTab[i]->to_included) printf("]");
		else printf("[");
		if (TheTab[i]->until_file_size) printf(" til EOF");
		printf("\n\n");
	}

	for (i = 0; i < current_mpeg; i++) {
		printf("mpeg %.2d : %s [%p](" _OFF_d " bytes)\n",
			i, MpegTab[i]->Name(), (void*)(MpegTab[i]), MpegTab[i]->Size());
	}
}




chunkTab::~chunkTab(){
	int i;
	for (i = 0; i < current_chunk; i++) delete TheTab[i];
	for (i = 0; i < current_mpeg; i++) delete MpegTab[i];
	delete[] MpegTab;
	delete[] TheTab;
}



bool chunkTab::Nchunks(int n){
	int i;

	if (n <=1 ) {
		fprintf(stderr,"Can not cut in %d parts\n",n);
		return false;
	}

	if (!current_mpeg_ptr) {
		fprintf(stderr,"No mpeg file for option -%d\n",n);
		return false;
	}

	if ((current_chunk + n) >= max) {
		MoreRoom(n - (max-current_chunk)+20);
	}
	chunk* tempchunk;

	for (i = 1; i <= n; i++) {
		tempchunk = new chunk;
		tempchunk->file = 0;
		tempchunk->from = -1;
		tempchunk->to = -1;
		tempchunk->to_included = true;
		tempchunk->from_included = false;
		tempchunk->until_file_size = false;
		tempchunk->unit_is_second = false;
		tempchunk->mpegfile = current_mpeg_ptr;
		tempchunk->from = off_t((((tempchunk->mpegfile->Size())*1.0)/n)*(i-1));
		tempchunk->to = off_t((((tempchunk->mpegfile->Size())*1.0)/n)*(i));
		if (i == n) tempchunk->until_file_size = true;
		TheTab[current_chunk++] = tempchunk;
	}
	return true;
}

