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

#include "k3baudiodecoder.h"

#include <kdebug.h>
#include <kfilemetainfo.h>

#include <qmap.h>

#include <math.h>

#ifdef HAVE_LIBSAMPLERATE
#include <samplerate.h>
#else
#include "libsamplerate/samplerate.h"
#endif

#if !(HAVE_LRINT && HAVE_LRINTF)
#define lrint(dbl)              ((int) (dbl))
#define lrintf(flt)             ((int) (flt))
#endif

class K3bAudioDecoder::Private
{
public:
  Private()
    : metaInfo(0),
      resampleState(0),
      resampleData(0),
      inBuffer(0),
      inBufferPos(0),
      inBufferLen(0),
      inBufferSize(0),
      outBuffer(0),
      outBufferSize(0),
      monoBuffer(0),
      monoBufferSize(0) {
  }

  unsigned long alreadyDecoded;
  K3b::Msf decodingStartPos;
  K3b::Msf decodingLength;

  KFileMetaInfo* metaInfo;

  // set to true once decodeInternal() returned 0
  bool decoderFinished;

  // resampling
  SRC_STATE* resampleState;
  SRC_DATA* resampleData;

  float* inBuffer;
  float* inBufferPos;
  int inBufferLen;
  int inBufferSize;

  float* outBuffer;
  int outBufferSize;

  int samplerate;
  int channels;

  // mono -> stereo conversion
  char* monoBuffer;
  int monoBufferSize;

  QMap<QString, QString> technicalInfoMap;
  QMap<MetaDataField, QString> metaInfoMap;
};



K3bAudioDecoder::K3bAudioDecoder( QObject* parent, const char* name )
  : K3bPlugin( parent, name )
{
  d = new Private();
}


K3bAudioDecoder::~K3bAudioDecoder()
{
  delete d->metaInfo;
  if( d->inBuffer ) delete [] d->inBuffer;
  if( d->outBuffer ) delete [] d->outBuffer;
  if( d->monoBuffer ) delete [] d->monoBuffer;
  delete d->resampleData;
  if( d->resampleState )
    src_delete( d->resampleState );
  delete d;
}


void K3bAudioDecoder::setFilename( const QString& filename )
{
  m_fileName = filename;
  delete d->metaInfo;
  d->metaInfo = 0;
}


bool K3bAudioDecoder::analyseFile()
{
  d->technicalInfoMap.clear();
  d->metaInfoMap.clear();
  delete d->metaInfo;
  d->metaInfo = 0;

  cleanup();

  bool ret = analyseFileInternal( m_length, d->samplerate, d->channels );
  if( ret ) {
    return ( ( d->channels == 1 || d->channels == 2 ) && m_length > 0 );
  }
  else
    return false;
}


bool K3bAudioDecoder::initDecoder( const K3b::Msf& startOffset, const K3b::Msf& len )
{
  cleanup();

  if( d->resampleState )
    src_reset( d->resampleState );

  d->alreadyDecoded = 0;

  if( startOffset > length() )
    d->decodingStartPos = 0;
  else
    d->decodingStartPos = startOffset;

  if( len + d->decodingStartPos > length() )
    d->decodingLength = length() - d->decodingStartPos;
  else
    d->decodingLength = len;

  d->decoderFinished = false;

  if( initDecoderInternal() ) {
    if( startOffset > 0 )
      return seek( startOffset );
    else
      return true;
  }
  else
    return false;
}


bool K3bAudioDecoder::initDecoder()
{
  return initDecoder( 0, length() );
}


int K3bAudioDecoder::decode( char* _data, int maxLen )
{
  unsigned long lengthToDecode = d->decodingLength.audioBytes();

  if( d->alreadyDecoded >= lengthToDecode )
    return 0;

  int read = 0;

  if( !d->decoderFinished ) {
    if( d->samplerate != 44100 ) {

      // check if we have data left from some previous conversion
      if( d->inBufferLen > 0 ) {
	read = resample( _data, maxLen );
      }
      else {
	// FIXME: change this so the buffer is never deleted here. Use QMIN(d->inBufferSize, maxLen) instead
	if( d->inBufferSize < maxLen/2 ) {
	  if( d->inBuffer )
	    delete [] d->inBuffer;
	  d->inBuffer = new float[maxLen];
	  d->inBufferSize = maxLen;
	}

	if( (read = decodeInternal( _data, maxLen )) == 0 )
	  d->decoderFinished = true;

	d->inBufferLen = read/2;
	d->inBufferPos = d->inBuffer;
	from16bitBeSignedToFloat( _data, d->inBuffer, d->inBufferLen );
      
	read = resample( _data, maxLen );
      }
    }
    else if( d->channels == 1 ) {
      // FIXME: change this so the buffer is never deleted here. Use QMIN(d->monoBufferSize, maxLen) instead
      if( d->monoBufferSize < maxLen/2 ) {
	if( d->monoBuffer )
	  delete [] d->monoBuffer;
	d->monoBuffer = new char[maxLen/2];
      }

      // we simply duplicate every frame
      if( (read = decodeInternal( d->monoBuffer, maxLen/2 )) == 0 )
	d->decoderFinished = true;

      for( int i = 0; i < read; i+=2 ) {
	_data[2*i] = _data[2*i+2] = d->monoBuffer[i];
	_data[2*i+1] = _data[2*i+3] = d->monoBuffer[i+1];
      }

      read *= 2;
    }
    else {
      if( (read = decodeInternal( _data, maxLen )) == 0 )
	d->decoderFinished = true;
    }
  }



  if( read < 0 ) {
    return -1;
  }
  else if( read == 0 ) {
    // check if we need to pad
    int bytesToPad = lengthToDecode - d->alreadyDecoded;
    if( bytesToPad > 0 ) {
      kdDebug() << "(K3bAudioDecoder) track length: " << lengthToDecode
		<< "; decoded module data: " << d->alreadyDecoded
		<< "; we need to pad " << bytesToPad << " bytes." << endl;

      if( maxLen < bytesToPad )
	bytesToPad = maxLen;

      ::memset( _data, 0, bytesToPad );

      d->alreadyDecoded += bytesToPad;

      return bytesToPad;
    }
    else {
      kdDebug() << "(K3bAudioDecoder) decoded " << d->alreadyDecoded << " bytes." << endl;
      return 0;
    }
  }
  else {

    // check if we decoded too much
    if( d->alreadyDecoded + read > lengthToDecode ) {
      kdDebug() << "(K3bAudioDecoder) we decoded too much. Cutting output by " 
		<< (read + d->alreadyDecoded - lengthToDecode) << endl;
      read = lengthToDecode - d->alreadyDecoded;
    }

    d->alreadyDecoded += read;
    return read;
  }
}


// resample data in d->inBufferPos and save the result to data
// 
// 
int K3bAudioDecoder::resample( char* data, int maxLen )
{
  if( !d->resampleState ) {
    d->resampleState = src_new( SRC_SINC_MEDIUM_QUALITY, d->channels, 0 );
    if( !d->resampleState ) {
      kdDebug() << "(K3bAudioDecoder) unable to initialize resampler." << endl;
      return -1;
    }
    d->resampleData = new SRC_DATA;
  }

  if( d->outBufferSize == 0 ) {
    d->outBufferSize = maxLen/2;
    d->outBuffer = new float[maxLen/2];
  }

  d->resampleData->data_in = d->inBufferPos;
  d->resampleData->data_out = d->outBuffer;
  d->resampleData->input_frames = d->inBufferLen/d->channels;
  d->resampleData->output_frames = maxLen/2/2;  // in case of mono files we need the space anyway
  d->resampleData->src_ratio = 44100.0/(double)d->samplerate;
  if( d->inBufferLen == 0 )
    d->resampleData->end_of_input = 1;  // this should force libsamplerate to output the last frames
  else
    d->resampleData->end_of_input = 0;

  int len = 0;
  if( (len = src_process( d->resampleState, d->resampleData ) ) ) {
    kdDebug() << "(K3bAudioDecoder) error while resampling: " << src_strerror(len) << endl;
    return -1;
  }

  if( d->channels == 2 )
    fromFloatTo16BitBeSigned( d->outBuffer, data, d->resampleData->output_frames_gen*d->channels );
  else {
    for( int i = 0; i < d->resampleData->output_frames_gen; ++i ) {
      fromFloatTo16BitBeSigned( &d->outBuffer[i], &data[4*i], 1 );
      fromFloatTo16BitBeSigned( &d->outBuffer[i], &data[4*i+2], 1 );
    }
  }
   
  d->inBufferPos += d->resampleData->input_frames_used*d->channels;
  d->inBufferLen -= d->resampleData->input_frames_used*d->channels;
  if( d->inBufferLen <= 0 ) {
    d->inBufferPos = d->inBuffer;
    d->inBufferLen = 0;
  }

  // 16 bit frames, so we need to multiply by 2
  // and we always have two channels
  return d->resampleData->output_frames_gen*2*2;
}


void K3bAudioDecoder::from16bitBeSignedToFloat( char* src, float* dest, int samples )
{
  while( samples ) {
    samples--;
    dest[samples] = static_cast<float>( Q_INT16(((src[2*samples]<<8)&0xff00)|(src[2*samples+1]&0x00ff)) / 32768.0 );
  }
}


void K3bAudioDecoder::fromFloatTo16BitBeSigned( float* src, char* dest, int samples )
{
  while( samples ) {
    samples--;

    float scaled = src[samples] * 32768.0;
    Q_INT16 val = 0;

    // clipping
    if( scaled >= ( 1.0 * 0x7FFF ) )
      val = 32767;
    else if( scaled <= ( -8.0 * 0x1000 ) )
      val = -32768;
    else
      val = lrintf(scaled);

    dest[2*samples]   = val>>8;
    dest[2*samples+1] = val;
  }
}


void K3bAudioDecoder::from8BitTo16BitBeSigned( char* src, char* dest, int samples )
{
  while( samples ) {
    samples--;

    float scaled = static_cast<float>(Q_UINT8(src[samples])-128) / 128.0 * 32768.0;
    Q_INT16 val = 0;
      
    // clipping
    if( scaled >= ( 1.0 * 0x7FFF ) )
      val = 32767;
    else if( scaled <= ( -8.0 * 0x1000 ) )
      val = -32768;
    else
      val = lrintf(scaled);

    dest[2*samples]   = val>>8;
    dest[2*samples+1] = val;
  }
}


bool K3bAudioDecoder::seek( const K3b::Msf& pos )
{
  d->alreadyDecoded = 0;
  return seekInternal( pos );
}


void K3bAudioDecoder::cleanup()
{
}


QString K3bAudioDecoder::metaInfo( MetaDataField f )
{
  if( d->metaInfoMap.contains( f ) )
    return d->metaInfoMap[f];

  // fall back to KFileMetaInfo
  if( !d->metaInfo )
    d->metaInfo = new KFileMetaInfo( filename() );

  if( d->metaInfo->isValid() ) {
    QString tag;
    switch( f ) {
    case META_TITLE:
      tag = "Title";
      break;
    case META_ARTIST:
      tag = "Artist";
      break;
    case META_SONGWRITER:
      tag = "Songwriter";
      break;
    case META_COMPOSER:
      tag = "Composer";
      break;
    case META_COMMENT:
      tag = "Comment";
      break;
    }

    KFileMetaInfoItem item = d->metaInfo->item( tag );
    if( item.isValid() )
      return item.string();
  }

  return QString::null;
}


void K3bAudioDecoder::addMetaInfo( MetaDataField f, const QString& value )
{
  if( !value.isEmpty() )
    d->metaInfoMap[f] = value;
  else
    kdDebug() << "(K3bAudioDecoder) empty meta data field." << endl;
}


QStringList K3bAudioDecoder::supportedTechnicalInfos() const
{
  QStringList l;
  for( QMap<QString, QString>::const_iterator it = d->technicalInfoMap.begin();
       it != d->technicalInfoMap.end(); ++it )
    l.append( it.key() );
  return l;
}


QString K3bAudioDecoder::technicalInfo( const QString& key ) const
{
  return d->technicalInfoMap[key];
}


void K3bAudioDecoder::addTechnicalInfo( const QString& key, const QString& value )
{
  d->technicalInfoMap[key] = value;
}


#include "k3baudiodecoder.moc"
