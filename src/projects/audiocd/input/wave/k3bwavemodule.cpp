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
#include <k3baudiotrack.h>

#include <qfile.h>
#include <qcstring.h>
#include <qdatastream.h>
#include <qtimer.h>

#include <kdebug.h>



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
}


K3bWaveModule::~K3bWaveModule()
{
  delete m_file;
}


int K3bWaveModule::decodeInternal( char* _data, int maxLen )
{
  int read = m_file->readBlock( _data, maxLen );
  if( read > 0 ) {
    if( read % 2 > 0 ) {
      kdDebug() << "(K3bWaveModule) data length is not a multible of 2! Cannot write data." << endl;
      return -1;
    }

    // swap bytes
    char buf;
    for( int i = 0; i < read-1; i+=2 ) {
      buf = _data[i];
      _data[i] = _data[i+1];
      _data[i+1] = buf;
    }

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
  if( !m_file->open( IO_ReadOnly ) ) {
    kdDebug() << "(K3bWaveModule) could not open file." << endl;
    return false;
  }

  // skip the header
  if( identifyWaveFile( m_file ) <= 0 ) {
    kdDebug() << "(K3bWaveModule) no supported wave file." << endl;
    return false;
  }

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


/**
 * Returns the length of the wave file in frames (1/75 second) if
 * it is a 16bit stereo 44100 kHz wave file
 * Otherwise 0 is returned.
 * leave file seek pointer past WAV header.
 */
unsigned long K3bWaveModule::identifyWaveFile( QFile* f )
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
 
  static const char* WAV_RIFF_MAGIC = "RIFF";		// Magic for file format
  static const char* WAV_WAVE_MAGIC = "WAVE";		// Magic for Waveform Audio
  static const char* WAV_FMT_MAGIC  = "fmt ";		// Start of Waveform format
  static const char* WAV_DATA_MAGIC = "data";		// Start of data chunk

  chunk_t chunk;
  riff_chunk riff;
  fmt_chunk fmt;


  // read riff chunk
  if( f->readBlock( (char*)&chunk, sizeof(chunk) ) != sizeof(chunk) ) {
    kdDebug() << "(K3bWaveModule) unable to read from " << f->name() << endl;
    return 0;
  }
  if( qstrncmp( (char*)chunk.ckid, WAV_RIFF_MAGIC, 4 ) ) {
    kdDebug() << "(K3bWaveModule) " << f->name() << ": not a RIFF file." << endl;
    return 0;
  }

  // read wave chunk
  if( f->readBlock( (char*)&riff, sizeof(riff) ) != sizeof(riff) ) {
    kdDebug() << "(K3bWaveModule) unable to read from " << f->name() << endl;
    return 0;
  }
  if( qstrncmp( (char*)riff.wave, WAV_WAVE_MAGIC, 4 ) ) {
    kdDebug() << "(K3bWaveModule) " << f->name() << ": not a WAVE file." << endl;
    return 0;
  }


  // read fmt chunk
  if( f->readBlock( (char*)&chunk, sizeof(chunk) ) != sizeof(chunk) ) {
    kdDebug() << "(K3bWaveModule) unable to read from " << f->name() << endl;
    return 0;
  }
  if( qstrncmp( (char*)chunk.ckid, WAV_FMT_MAGIC, 4 ) ) {
    kdDebug() << "(K3bWaveModule) " << f->name() << ": could not find format chunk." << endl;
    return 0;
  }
  if( f->readBlock( (char*)&fmt, sizeof(fmt) ) != sizeof(fmt) ) {
    kdDebug() << "(K3bWaveModule) unable to read from " << f->name() << endl;
    return 0;
  }
  if( le_a_to_u_short(fmt.fmt_tag) != 1 ||
      le_a_to_u_short(fmt.channels) != 2 ||
      le_a_to_u_long(fmt.sample_rate) != 44100 ||
      le_a_to_u_short(fmt.bits_per_sample) != 16) {
    kdDebug() << "(K3bWaveModule) " << f->name() << ": wrong format:" << endl
	      << "                format:      " << le_a_to_u_short(fmt.fmt_tag) << endl
	      << "                channels:    " << le_a_to_u_short(fmt.channels) << endl
	      << "                samplerate:  " << le_a_to_u_long(fmt.sample_rate) << endl
	      << "                bits/sample: " << le_a_to_u_short(fmt.bits_per_sample) << endl;
    return 0;
  }

  // skip all other (unknown) format chunk fields
  if( !f->at( f->at() + le_a_to_u_long(chunk.cksize) - sizeof(fmt) ) ) {
    kdDebug() << "(K3bWaveModule) " << f->name() << ": could not seek in file." << endl;
    return 0;
  }


  // find data chunk
  bool foundData = false;
  while( !foundData ) {
    if( f->readBlock( (char*)&chunk, sizeof(chunk) ) != sizeof(chunk) ) {
      kdDebug() << "(K3bWaveModule) unable to read from " << f->name() << endl;
      return 0;
    }

    // skip chunk data of unknown chunk
    if( qstrncmp( (char*)chunk.ckid, WAV_DATA_MAGIC, 4 ) ) {
      kdDebug() << "(K3bWaveModule) skipping chunk: " << (char*)chunk.ckid << endl;
      if( !f->at( f->at() + le_a_to_u_long(chunk.cksize) ) ) {
        kdDebug() << "(K3bWaveModule) " << f->name() << ": could not seek in file." << endl;
        return 0;
      }
    }
    else
      foundData = true;
  }

  // found data chunk
  int headerLen = f->at();
  unsigned long size = le_a_to_u_long(chunk.cksize);
  if( headerLen + size > (unsigned long)f->size() ) {
    kdDebug() << "(K3bWaveModule) " << f->name() << ": file length " << f->size() 
    << " does not match length from WAVE header " << headerLen << " + " << size
    << " - using actual length." << endl;
    size = (f->size() - headerLen);
  }

  // we pad to a multible of 2352 bytes
  if( (size%2352) > 0 )
    size = (size/2352) + 1;
  else
    size = size/2352;

  return size;
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
