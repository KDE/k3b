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

#include "k3bwavedecoder.h"

#include <qfile.h>
#include <qcstring.h>

#include <kdebug.h>
#include <klocale.h>
#include <kinstance.h>



static unsigned short le_a_to_u_short( unsigned char* a ) {
  return ((unsigned short) 
	  ((a[0]       & 0xFF) | 
	   (a[1] << 8  & 0xFF00)) );
}

static unsigned long le_a_to_u_long( unsigned char* a ) {
  return ((unsigned long) 
	  ((a[0]       & 0xFF) | 
	   (a[1] << 8  & 0xFF00) | 
	   (a[2] << 16 & 0xFF0000) | 
	   (a[3] << 24 & 0xFF000000)) );
}


/**
 * Returns the length of the wave file in frames (1/75 second) if
 * it is a 16bit stereo wave file
 * Otherwise 0 is returned.
 * leave file seek pointer past WAV header.
 */
static unsigned long identifyWaveFile( QFile* f, int* samplerate = 0 )
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
    kdDebug() << "(K3bWaveDecoder) unable to read from " << f->name() << endl;
    return 0;
  }
  if( qstrncmp( (char*)chunk.ckid, WAV_RIFF_MAGIC, 4 ) ) {
    kdDebug() << "(K3bWaveDecoder) " << f->name() << ": not a RIFF file." << endl;
    return 0;
  }

  // read wave chunk
  if( f->readBlock( (char*)&riff, sizeof(riff) ) != sizeof(riff) ) {
    kdDebug() << "(K3bWaveDecoder) unable to read from " << f->name() << endl;
    return 0;
  }
  if( qstrncmp( (char*)riff.wave, WAV_WAVE_MAGIC, 4 ) ) {
    kdDebug() << "(K3bWaveDecoder) " << f->name() << ": not a WAVE file." << endl;
    return 0;
  }


  // read fmt chunk
  if( f->readBlock( (char*)&chunk, sizeof(chunk) ) != sizeof(chunk) ) {
    kdDebug() << "(K3bWaveDecoder) unable to read from " << f->name() << endl;
    return 0;
  }
  if( qstrncmp( (char*)chunk.ckid, WAV_FMT_MAGIC, 4 ) ) {
    kdDebug() << "(K3bWaveDecoder) " << f->name() << ": could not find format chunk." << endl;
    return 0;
  }
  if( f->readBlock( (char*)&fmt, sizeof(fmt) ) != sizeof(fmt) ) {
    kdDebug() << "(K3bWaveDecoder) unable to read from " << f->name() << endl;
    return 0;
  }
  if( le_a_to_u_short(fmt.fmt_tag) != 1 ||
      le_a_to_u_short(fmt.channels) != 2 ||
      /*      le_a_to_u_long(fmt.sample_rate) != 44100 ||*/
      le_a_to_u_short(fmt.bits_per_sample) != 16) {
    kdDebug() << "(K3bWaveDecoder) " << f->name() << ": wrong format:" << endl
	      << "                format:      " << le_a_to_u_short(fmt.fmt_tag) << endl
	      << "                channels:    " << le_a_to_u_short(fmt.channels) << endl
	      << "                samplerate:  " << le_a_to_u_long(fmt.sample_rate) << endl
	      << "                bits/sample: " << le_a_to_u_short(fmt.bits_per_sample) << endl;
    return 0;
  }

  int sampleRate = le_a_to_u_long(fmt.sample_rate);
  if( samplerate )
    *samplerate = sampleRate;

  // skip all other (unknown) format chunk fields
  if( !f->at( f->at() + le_a_to_u_long(chunk.cksize) - sizeof(fmt) ) ) {
    kdDebug() << "(K3bWaveDecoder) " << f->name() << ": could not seek in file." << endl;
    return 0;
  }


  // find data chunk
  bool foundData = false;
  while( !foundData ) {
    if( f->readBlock( (char*)&chunk, sizeof(chunk) ) != sizeof(chunk) ) {
      kdDebug() << "(K3bWaveDecoder) unable to read from " << f->name() << endl;
      return 0;
    }

    // skip chunk data of unknown chunk
    if( qstrncmp( (char*)chunk.ckid, WAV_DATA_MAGIC, 4 ) ) {
      kdDebug() << "(K3bWaveDecoder) skipping chunk: " << (char*)chunk.ckid << endl;
      if( !f->at( f->at() + le_a_to_u_long(chunk.cksize) ) ) {
        kdDebug() << "(K3bWaveDecoder) " << f->name() << ": could not seek in file." << endl;
        return 0;
      }
    }
    else
      foundData = true;
  }

  // found data chunk
  unsigned long size = le_a_to_u_long(chunk.cksize);
  if( f->at() + size > (unsigned long)f->size() ) {
    kdDebug() << "(K3bWaveDecoder) " << f->name() << ": file length " << f->size() 
    << " does not match length from WAVE header " << f->at() << " + " << size
    << " - using actual length." << endl;
    size = (f->size() - f->at());
  }

  if( sampleRate != 44100 )
    size = size * 44100 / sampleRate;

  // we pad to a multible of 2352 bytes
  if( (size%2352) > 0 )
    size = (size/2352) + 1;
  else
    size = size/2352;

  return size;
}


K3bWaveDecoder::K3bWaveDecoder( QObject* parent, const char* name )
  : K3bAudioDecoder( parent, name )
{
  m_file = new QFile();
}


K3bWaveDecoder::~K3bWaveDecoder()
{
  delete m_file;
}


int K3bWaveDecoder::decodeInternal( char* _data, int maxLen )
{
  int read = m_file->readBlock( _data, maxLen );
  if( read > 0 ) {
    if( read % 2 > 0 ) {
      kdDebug() << "(K3bWaveDecoder) data length is not a multible of 2! Cannot write data." << endl;
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
    return 0;
  }
  else
    return -1;
}


bool K3bWaveDecoder::analyseFileInternal( K3b::Msf* frames, int* samplerate, int* channels )
{
  // handling wave files is very easy...
  if( initDecoderInternal() ) {
    *frames = m_size;
    *samplerate = m_sampleRate;
    *channels = 2; // FIXME: support mono files
    return true;
  }
  else
    return false;
}


bool K3bWaveDecoder::initDecoderInternal()
{
  cleanup();

  m_file->setName( filename() );
  if( !m_file->open( IO_ReadOnly ) ) {
    kdDebug() << "(K3bWaveDecoder) could not open file." << endl;
    return false;
  }

  // skip the header
  m_size = identifyWaveFile( m_file, &m_sampleRate );
  if( m_size <= 0 ) {
    kdDebug() << "(K3bWaveDecoder) no supported wave file." << endl;
    cleanup();
    return false;
  }

  m_headerLength = m_file->at();

  return true;
}


bool K3bWaveDecoder::seekInternal( const K3b::Msf& pos )
{
  return( m_file->at( m_headerLength + (pos.totalFrames()*2352) ) );
}


void K3bWaveDecoder::cleanup()
{
  if( m_file->isOpen() )
    m_file->close();
}


QString K3bWaveDecoder::fileType() const
{
  return i18n("16 bit %1 Hz stereo WAVE").arg(m_sampleRate);
}



K3bWaveDecoderFactory::K3bWaveDecoderFactory( QObject* parent, const char* name )
  : K3bAudioDecoderFactory( parent, name )
{
  s_instance = new KInstance( "k3bwavedecoder" );
}


K3bWaveDecoderFactory::~K3bWaveDecoderFactory()
{
}


K3bPlugin* K3bWaveDecoderFactory::createPluginObject( QObject* parent, 
						      const char* name,
						      const QStringList& )
{
  return new K3bWaveDecoder( parent, name );
}


bool K3bWaveDecoderFactory::canDecode( const KURL& url ) 
{
  QFile f( url.path() );
  if( !f.open(	IO_ReadOnly ) ) {
    kdDebug() << "(K3bWaveDecoder) could not open file " << url.path() << endl;
    return false;
  }

  return (identifyWaveFile( &f ) > 0);
}



#include "k3bwavedecoder.moc"
