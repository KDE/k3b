/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include <config.h>

#include "k3bwavedecoder.h"

#include <k3bpluginfactory.h>

#include <qfile.h>
#include <qcstring.h>

#include <kdebug.h>
#include <klocale.h>



K_EXPORT_COMPONENT_FACTORY( libk3bwavedecoder, K3bPluginFactory<K3bWaveDecoderFactory>( "libk3bwavedecoder" ) )


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
 * Returns the length of the wave file in bytes
 * Otherwise 0 is returned.
 * leave file seek pointer past WAV header.
 */
static unsigned long identifyWaveFile( QFile* f, int* samplerate = 0, int* channels = 0, int* samplesize = 0 )
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
      le_a_to_u_short(fmt.channels) > 2 ||
      ( le_a_to_u_short(fmt.bits_per_sample) != 16 &&
	le_a_to_u_short(fmt.bits_per_sample) != 8 ) ) {
    kdDebug() << "(K3bWaveDecoder) " << f->name() << ": wrong format:" << endl
	      << "                format:      " << le_a_to_u_short(fmt.fmt_tag) << endl
	      << "                channels:    " << le_a_to_u_short(fmt.channels) << endl
	      << "                samplerate:  " << le_a_to_u_long(fmt.sample_rate) << endl
	      << "                bits/sample: " << le_a_to_u_short(fmt.bits_per_sample) << endl;
    return 0;
  }

  int sampleRate = le_a_to_u_long(fmt.sample_rate);
  int ch = le_a_to_u_short(fmt.channels);
  int sampleSize = le_a_to_u_short(fmt.bits_per_sample);;
  if( samplerate )
    *samplerate = sampleRate;
  if( channels )
    *channels = ch;
  if( samplesize )
    *samplesize = sampleSize;

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

  return size;
}


class K3bWaveDecoder::Private {
public:
  Private()
    : buffer(0),
      bufferSize(0) {
  }

  QFile* file;

  long headerLength;
  int sampleRate;
  int channels;
  int sampleSize;
  unsigned long size;
  unsigned long alreadyRead;

  char* buffer;
  int bufferSize;
};


K3bWaveDecoder::K3bWaveDecoder( QObject* parent, const char* name )
  : K3bAudioDecoder( parent, name )
{
  d = new Private();
  d->file = new QFile();
}


K3bWaveDecoder::~K3bWaveDecoder()
{
  delete d->file;
  delete d;
}


int K3bWaveDecoder::decodeInternal( char* _data, int maxLen )
{
  int read = 0;

  maxLen = QMIN( maxLen, (int)(d->size - d->alreadyRead) );

  if( d->sampleSize == 16 ) {
    read = d->file->readBlock( _data, maxLen );
    if( read > 0 ) {
      d->alreadyRead += read;

      if( read % 2 > 0 ) {
	kdDebug() << "(K3bWaveDecoder) data length is not a multible of 2! Cutting data." << endl;
	read -= 1;
      }

      // swap bytes
      char buf;
      for( int i = 0; i < read; i+=2 ) {
	buf = _data[i];
	_data[i] = _data[i+1];
	_data[i+1] = buf;
      }
    }
  }
  else {
    if( !d->buffer ) {
      d->buffer = new char[maxLen/2];
      d->bufferSize = maxLen/2;
    }

    read = d->file->readBlock( d->buffer, QMIN(maxLen/2, d->bufferSize) );
    d->alreadyRead += read;

    // stretch samples to 16 bit
    from8BitTo16BitBeSigned( d->buffer, _data, read );

    read *= 2;
  }

  return read;
}


bool K3bWaveDecoder::analyseFileInternal( K3b::Msf& frames, int& samplerate, int& channels )
{
  // handling wave files is very easy...
  if( initDecoderInternal() ) {

    //
    // d->size is the number of bytes in the wave file
    //
    unsigned long size = d->size;
    if( d->sampleRate != 44100 )
      size = (int)((double)size * 44100.0 / (double)d->sampleRate);

    if( d->sampleSize == 8 )
      size *= 2;
    if( d->channels == 1 )
      size *= 2;

    //
    // we pad to a multible of 2352 bytes 
    // (the actual padding of zero data will be done by the K3bAudioDecoder class)
    //
    if( (size%2352) > 0 )
      size = (size/2352) + 1;
    else
      size = size/2352;

    frames = size;
    samplerate = d->sampleRate;
    channels = d->channels;
    return true;
  }
  else
    return false;
}


bool K3bWaveDecoder::initDecoderInternal()
{
  cleanup();

  d->file->setName( filename() );
  if( !d->file->open( IO_ReadOnly ) ) {
    kdDebug() << "(K3bWaveDecoder) could not open file." << endl;
    return false;
  }

  // skip the header
  d->size = identifyWaveFile( d->file, &d->sampleRate, &d->channels, &d->sampleSize );
  if( d->size <= 0 ) {
    kdDebug() << "(K3bWaveDecoder) no supported wave file." << endl;
    cleanup();
    return false;
  }

  d->headerLength = d->file->at();
  d->alreadyRead = 0;

  return true;
}


bool K3bWaveDecoder::seekInternal( const K3b::Msf& pos )
{
  return( d->file->at( d->headerLength + (pos.totalFrames()*2352) ) );
}


void K3bWaveDecoder::cleanup()
{
  if( d->file->isOpen() )
    d->file->close();
}


QString K3bWaveDecoder::fileType() const
{
  return i18n("WAVE");
}


QStringList K3bWaveDecoder::supportedTechnicalInfos() const
{
  return QStringList::split( ";", 
			     i18n("Channels") + ";" +
			     i18n("Sampling Rate") + ";" +
			     i18n("Sample Size") );
}


QString K3bWaveDecoder::technicalInfo( const QString& name ) const
{
  if( name == i18n("Channels") )
    return QString::number(d->channels);
  else if( name == i18n("Sampling Rate") )
    return i18n("%1 Hz").arg(d->sampleRate);
  else if( name == i18n("Sample Size") )
    return i18n("%1 bits").arg(d->sampleSize);
  else
    return QString::null;
}


K3bWaveDecoderFactory::K3bWaveDecoderFactory( QObject* parent, const char* name )
  : K3bAudioDecoderFactory( parent, name )
{
}


K3bWaveDecoderFactory::~K3bWaveDecoderFactory()
{
}


K3bAudioDecoder* K3bWaveDecoderFactory::createDecoder( QObject* parent, 
						       const char* name ) const
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
