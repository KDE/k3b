/***************************************************************************
                          k3bmp3track.cpp  -  description
                             -------------------
    begin                : Thu Jun 14 2001
    copyright            : (C) 2001 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/



#include "k3bmp3track.h"
#include "../k3bglobals.h"

// ID3lib-includes
#include <id3/tag.h>
#include <id3/misc_support.h>


#include <kapp.h>
#include <kconfig.h>

#include <qstring.h>
#include <qfileinfo.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stream.h>


K3bMp3Track::K3bMp3Track( QList<K3bAudioTrack>* parent, const QString& filename )
  : K3bAudioTrack( parent, filename), m_bufferFile()
{
  m_filetype = K3b::MP3;

  readTrackInfo( filename );
}


K3bMp3Track::~K3bMp3Track()
{
  // delete the buffer file
  if( !m_bufferFile.isEmpty() ) {
    qDebug( "Deleting buffered file" + m_bufferFile );
    QFile::remove( m_bufferFile );
  }
}


void K3bMp3Track::readTrackInfo( const QString& fileName )
{
  ID3_Tag _tag( fileName.latin1() );
  qDebug("*******************************");
  qDebug( "ID3_Tag created. Size: %i", _tag.Size() );
  ID3_Frame* _frame = _tag.Find( ID3FID_TITLE );
  qDebug( "ID3_Frame created" );
  if( _frame )
    setTitle( QString(ID3_GetString(_frame, ID3FN_TEXT )) );
  else
    qDebug("No title info found.");
		
  _frame = _tag.Find( ID3FID_LEADARTIST );
  if( _frame )
    setArtist( QString(ID3_GetString(_frame, ID3FN_TEXT )) );
  else
    qDebug("No artist info found.");

  
  int _id3TagSize = _tag.Size();

  // find frame-header
  unsigned char data[1024];
  unsigned char* datapointer;

//   m_file.open( IO_ReadOnly );
//   m_file.at(0);
//   int readbytes = m_file.readBlock( buffer, 1024 );

//   m_file.close();

  FILE *fd;
  fd = fopen(fileName.latin1(),"r");
  fseek(fd, 0, SEEK_SET);
  int readbytes = fread(&data, 1, 1024, fd);
  fclose(fd);


  bool found = false;

  unsigned int _header = data[0]<<24 | data[1]<<16 | data[2]<<8 | data[3];
  datapointer = data+4;
  readbytes -= 4;

  while( !found && (readbytes > 0) ) 
    {
      if( mp3HeaderCheck(_header) ) {
	qDebug( "   header found: %x", _header );
	found = true;
	break;
      }
  
      _header = _header<<4 | *datapointer;
      datapointer++;
      readbytes--;
    }
  if( found ) 
    {
      // calculate length
      if( mp3SampleRate(_header) ) {
	int _frameSize = 144 * mp3Bitrate(_header) * 1000 / ( mp3SampleRate(_header) + mp3Padding(_header) );
	qDebug( "**framesize calculated: %i", _frameSize);
	if( _frameSize ) {
	  int _frameNumber = (QFileInfo(m_file).size() - _id3TagSize ) / _frameSize;
	  qDebug( " #frames: %i", _frameNumber );
	  setLength(  _frameNumber * 26 / 10 );
	}
      }
      else
	qDebug("Samplerate is 0");
    }
  else
    {
      qDebug( "(K3bMp3Track) Warning: No mp3-frame-header found!!!" );
    }

  qDebug("*******************************");
}


int K3bMp3Track::mp3VersionNumber(unsigned int header) 
{
  qDebug(" version number" );

  int d = (header & 0x00180000) >> 19;

  switch (d) 
    {
    case 3:
      return 0;
    case 2:
      return 1;
    case 0:	
      return 2;
    }

  return -1;
}



int K3bMp3Track::mp3LayerNumber(unsigned int header) 
{
  qDebug(" layer number" );

  int d = (header & 0x00060000) >> 17;
  return 4-d;
}

bool K3bMp3Track::mp3Protection(unsigned int header) 
{
  qDebug(" protection" );

  return ((header & 0x00010000)==1);
}

int K3bMp3Track::mp3Bitrate(unsigned int header) 
{
  qDebug(" bitrate" );

  const unsigned int bitrates[3][3][15] =
    {
      {
	{0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448},
     	{0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384},
	{0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320}
      },
      {
     	{0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256},
	{0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160},
	{0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}
      },
      {
     	{0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256},
	{0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160},
	{0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}
      }
    };
  int version = mp3VersionNumber(header);
  int layer = mp3LayerNumber(header)-1;
  int bitrate = (header & 0x0000f000) >> 12;

 if( bitrate == 0 )
qDebug(" Bitrate is 0");

  printf("Header:%x\n",header);
  printf("version:%x\n",version);
  printf("layer:%d\n",layer);
  printf("Bitindex:%d\n",bitrate);
  printf("Bitrate:%d\n",bitrates[version][layer][bitrate]);
  return bitrates[version][layer][bitrate];
}

int K3bMp3Track::mp3SampleRate(unsigned int header)
{
  const unsigned int s_freq[3][4] =
    {
      {44100, 48000, 32000, 0},
      {22050, 24000, 16000, 0},
      {11025, 8000, 8000, 0}
    };

  int version = mp3VersionNumber(header);
  int srate = (header & 0x00000c00) >> 10;

  qDebug(" samplerate: %i", s_freq[version][srate] );

  return s_freq[version][srate];
}


bool K3bMp3Track::mp3HeaderCheck(unsigned int head)
{
  if( (head & 0xffe00000) != 0xffe00000)
    return false;

  if(!((head>>17)&3))
    return false;

  if( ((head>>12)&0xf) == 0xf)
    return false;

  if( ((head>>10)&0x3) == 0x3 )
    return false;

  if ((head & 0xffff0000) == 0xfffe0000)
    return false;

  return true;
}


int K3bMp3Track::mp3Padding(unsigned int header)
{
  qDebug(" padding" );

  if( header & 0x00000200 == 0x00000200 ) {
    // calculate padding
    return 1;
  }
  else return 0;
}
