/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bwavemodule.h"
#include <audio/k3baudiotrack.h>
#include <tools/k3bglobals.h>

#include <qfile.h>
#include <qcstring.h>
#include <qdatastream.h>
#include <qtimer.h>

#include <kdebug.h>

#include <string.h>


unsigned short K3bWaveModule::le_a_to_u_short( unsigned char* a ) {
  return ((unsigned short) 
	  ((a[0]       & 0xFF) | 
	   (a[1] << 8  & 0xFF00)) );
}

unsigned long K3bWaveModule::le_a_to_u_long( unsigned char* a ) {
  return ((unsigned long) 
	  ((a[0]       & 0xFF) | 
	   (a[1] << 8  & 0xFF00) | 
	   (a[2] << 16 & 0xFF0000) | 
	   (a[3] << 24 & 0xFF000000)) );
}



K3bWaveModule::K3bWaveModule( QObject* parent, const char* name )
  : K3bAudioModule( parent, name )
{
  m_file = new QFile();
  m_data = new QByteArray( 10*4096 );
}


K3bWaveModule::~K3bWaveModule()
{
  delete m_data;
  delete m_file;
}


int K3bWaveModule::decodeInternal( const char** _data )
{
  int read = m_file->readBlock( m_data->data(), m_data->size() );
  if( read > 0 ) {
    if( read % 2 > 0 ) {
      kdDebug() << "(K3bWaveModule) data length is not a multible of 2! Cannot write data." << endl;
      return -1;
    }

    // swap bytes
    char buf;
    for( int i = 0; i < read-1; i+=2 ) {
      buf = m_data->at(i);
      m_data->at(i) = m_data->at(i+1);
      m_data->at(i+1) = buf;
    }

    *_data = m_data->data();
    return read;
  }
  else if( read == 0 ) {
    m_file->close();
    return 0;
  }
  else
    return -1;
}


bool K3bWaveModule::initDecodingInternal( const QString& filename )
{
  m_file->close();
  m_file->setName( filename );
  if( !m_file->open( IO_ReadOnly ) )
    return false;
  m_data->resize( 10*4096 );
  return true;
}


void K3bWaveModule::cleanup()
{
  m_file->close();
}


int K3bWaveModule::analyseTrack( const QString& filename, unsigned long& size )
{
  QFile f( filename );
  if( !f.open( IO_ReadOnly ) ) {
    kdDebug() << "(K3bWaveModule) could not open file " << filename << endl;
    return K3bAudioTitleMetaInfo::CORRUPT;
  }

  size = identifyWaveFile( &f );
  if( size <= 0 )
    return K3bAudioTitleMetaInfo::CORRUPT;

  return K3bAudioTitleMetaInfo::OK;
}


/*
 * Read WAV header, leave file seek pointer past WAV header.
 */
long K3bWaveModule::wavSize( QFile* f )
{
  typedef struct {
    unsigned char	ckid[4];
    unsigned char	cksize[4];
  } chunk_t;

  typedef struct {
    unsigned char	wave[4];
  } riff_chunk;

  typedef struct {
    unsigned char	fmt_tag[2];
    unsigned char	channels[2];
    unsigned char	sample_rate[4];
    unsigned char	av_byte_rate[4];
    unsigned char	block_size[2];
    unsigned char	bits_per_sample[2];
  } fmt_chunk;
 
 static const char* WAV_RIFF_MAGIC = "RIFF";		/* Magic for file format     */
 static const char* WAV_WAVE_MAGIC = "WAVE";		/* Magic for Waveform Audio  */
 static const char* WAV_FMT_MAGIC  = "fmt ";		/* Start of Waveform format  */
 static const char* WAV_DATA_MAGIC = "data";		/* Start of data chunk	     */

  if( !f->isOpen() )
    if( !f->open( IO_ReadOnly ) )
      return -1;

  chunk_t chunk;
  riff_chunk riff;
  fmt_chunk fmt;
  bool gotFormat = false;
  bool success = false;
  unsigned long size = 0;

  for( int i = 0; i < 3; ++i ) { // we need to test 3 chunks
    if( f->readBlock( (char*)&chunk, sizeof(chunk) ) != sizeof(chunk) )
      break;

    size = le_a_to_u_long(chunk.cksize);

    if( strncmp((char*)chunk.ckid, WAV_RIFF_MAGIC, 4 ) == 0 ) {
      /*
       * We found (first) RIFF header. Check if a WAVE
       * magic follows. Set up size to be able to skip
       * past this header.
       */
      if( f->readBlock( (char*)&riff, sizeof(riff) ) != sizeof(riff) )
	break;

      if( strncmp((char*)riff.wave, WAV_WAVE_MAGIC, 4) != 0 )
	break;
      size = sizeof(riff);
    } 
    else if( strncmp((char*)chunk.ckid, WAV_FMT_MAGIC, 4) == 0 ) {
      /*
       * We found WAVE "fmt " header. Check size (if it is
       * valid for a WAVE file) and coding whether it is
       * useable for a CD. 
       */
      if( size < sizeof(fmt) ) 
	break;
      if( f->readBlock( (char*)&fmt, sizeof(fmt) ) != sizeof(fmt) )
	break;

      if( le_a_to_u_short(fmt.channels) != 2 ||
	  le_a_to_u_long(fmt.sample_rate) != 44100 ||
	  le_a_to_u_short(fmt.bits_per_sample) != 16) {
	break;
      }
      gotFormat = true;
    } 
    else if (strncmp((char *)chunk.ckid, WAV_DATA_MAGIC, 4) == 0) {
      /*
       * We found WAVE "data" header. This contains the
       * size value of the audio part.
       */
      if( !gotFormat )
	break;
      if( (f->at() + size + sizeof(chunk)) > f->size() )
	size = f->size() - (f->at() + sizeof(chunk));
      success = true;
    }

    f->at( f->at() + size + sizeof(chunk) ); // Skip over current chunk
  }

  if( success )
    return size;
  else
    return -1;
}


/**
 * Returns the length of the wave file in frames (1/75 second) if
 * it is a 16bit stereo 44100 kHz wave file
 * Otherwise 0 is returned.
 */
unsigned long K3bWaveModule::identifyWaveFile( QFile* f )
{
  QDataStream inputStream( f );

  char magic[4];

  inputStream.readRawBytes( magic, 4 );
  if( inputStream.atEnd() || qstrncmp(magic, "RIFF", 4) ) {
    kdDebug() << f->name() << ": not a RIFF file." << endl;
    return 0;
  }

  f->at( 8 );
  inputStream.readRawBytes( magic, 4 );
  if( inputStream.atEnd() || qstrncmp(magic, "WAVE", 4) ) {
    kdDebug() << f->name() << ": not a wave file." << endl;
    return 0;
  }

  Q_INT32 chunkLen;

  while( qstrncmp(magic, "fmt ", 4) ) {

    inputStream.readRawBytes( magic, 4 );
    if( inputStream.atEnd() ) {
      kdDebug() << f->name() << ": could not find format chunk." << endl;
      return 0;
    }

    inputStream >> chunkLen;
    chunkLen = K3b::swapByteOrder( chunkLen );
    chunkLen += chunkLen & 1; // round to multiple of 2

    // skip chunk data of unknown chunk
    if( qstrncmp(magic, "fmt ", 4) )
      if( !f->at( f->at() + chunkLen ) ) {
        kdDebug() << f->name() << ": could not seek in file." << endl;
        return 0;
      }
  }

  // found format chunk
  if( chunkLen < 16 )
    return 0;

  Q_INT16 waveFormat;
  inputStream >> waveFormat;
  if (inputStream.atEnd() || K3b::swapByteOrder(waveFormat) != 1) {
    kdDebug() << f->name() << ": not in PCM format: " << waveFormat << endl;
    return 0;
  }

  Q_INT16 waveChannels;
  inputStream >> waveChannels;
  if (inputStream.atEnd() || K3b::swapByteOrder(waveChannels) != 2) {
    kdDebug() << f->name() << ": found " << waveChannels << " channel(s), require 2 channels." << endl;
    return 0;
  }

  Q_INT32 waveRate;
  inputStream >> waveRate; 
  if (inputStream.atEnd() || K3b::swapByteOrder(waveRate) != 44100) {
     kdDebug() << f->name() << ": found sampling rate " << waveRate << "d, require 44100." << endl;
     return 0;
  }

  Q_INT16 buffer16;
  Q_INT32 buffer32;
  inputStream >> buffer32; // skip average bytes/second
  inputStream >> buffer16; // skip block align

  Q_INT16 waveBits;
  inputStream >> waveBits;
  if (inputStream.atEnd() || K3b::swapByteOrder(waveBits) != 16) {
    kdDebug() << f->name() << ": found " << waveBits << " bits per sample, require 16." << endl;
    return 0;
  }

  chunkLen -= 16;
  // skip all other (unknown) format chunk fields
  if( !f->at( f->at() + chunkLen ) ) {
    kdDebug() << f->name() << ": could not seek in file." << endl;
    return 0;
  }


  // search data chunk
  while( qstrncmp(magic,"data", 4) ) {

    inputStream.readRawBytes( magic, 4 );
    if( inputStream.atEnd()  ) {
      kdDebug() << f->name() << ": could not find data chunk." << endl;
      return 0;
    }

    inputStream >> chunkLen;
    kdDebug() << "----before bs: " << chunkLen << "i" << endl;
    chunkLen = K3b::swapByteOrder( chunkLen );
    kdDebug() << "----after  bs: " << chunkLen << "i" << endl;
    chunkLen += chunkLen & 1; // round to multiple of 2
    kdDebug() << "----after rnd: " << chunkLen << "i" << endl;

    // skip chunk data of unknown chunk
    if( qstrncmp(magic, "data", 4) )
      if( !f->at( f->at() + chunkLen ) ) {
        kdDebug() << f->name() << ": could not seek in file." << endl;
        return 0;
      }
  }

  // found data chunk
  int headerLen = f->at();
  if( headerLen + chunkLen > (int)f->size() ) {
    kdDebug() << f->name() << ": file length " << f->size()
      << " does not match length from WAVE header " << headerLen << " + " << chunkLen
      << " - using actual length." << endl;

    // we pad to a multible of 2352 bytes
    int size = (f->size() - headerLen)/2352;
    if( (f->size() - headerLen)%2352 > 0 )
      size++;
    return size;
  }
  else {
    int size = chunkLen/2352;
    if( chunkLen%2352 > 0 )
      size++;
    return size;
  }
}


bool K3bWaveModule::canDecode( const KURL& url ) 
{
  QFile f( url.path() );
  if( !f.open(	IO_ReadOnly ) ) {
    kdDebug() << "(K3bWaveModule) could not open file " << url.path() << endl;
    return false;
  }

  return (identifyWaveFile( &f ) > 0);
}

#include "k3bwavemodule.moc"
