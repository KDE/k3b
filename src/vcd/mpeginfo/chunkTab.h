/***************************************************************************
                          chunkTab.h  -  description
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

#ifndef __chunkTab_hh_
#define __chunkTab_hh_

#include "mpeg.h"


class chunkTab {
public:
	chunkTab(int _max = 20);
	~chunkTab();
	bool AddFile(char* filename);
	bool ParseRange(char* range);
	bool ParseBoundaries(char* boundaries);
  int MpegVersion();
  void PrintTab();
	void PrintInfos();
	bool Nchunks(int n);
	chunk** GetChunks(int* n);
protected:
	void MoreRoom(int n);
	bool ParseValue(
		char* value,
		int value_length,
		off_t* translation,
		float* stranlslation,
		bool* time);
	//ordered tab of chunks
	chunk** TheTab;
	int max;
	int current_chunk;
	int current_mpeg;
	mpeg* current_mpeg_ptr;
	mpeg** MpegTab;
	//	char* current_file;
};


#endif
