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


#include "k3baudiodecoder.h"

#include <kdebug.h>
#include <kfilemetainfo.h>

#include <math.h>

#include "libsamplerate/samplerate.h"

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
      outBufferSize(0) {
  }

  unsigned long alreadyDecoded;
  K3b::Msf decodingStartPos;

  KFileMetaInfo* metaInfo;


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
};



K3bAudioDecoder::K3bAudioDecoder( QObject* parent, const char* name )
  : K3bPlugin( parent, name )
{
  d = new Private();
}


K3bAudioDecoder::~K3bAudioDecoder()
{
  if( d->inBuffer ) delete [] d->inBuffer;
  if( d->outBuffer ) delete [] d->outBuffer;
  delete d->metaInfo;
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
  cleanup();
  return analyseFileInternal( &m_length, &d->samplerate, &d->channels );
}


bool K3bAudioDecoder::initDecoder()
{
  cleanup();

  if( d->resampleState )
    src_reset( d->resampleState );

  d->alreadyDecoded = 0;
  d->decodingStartPos = 0;

  return initDecoderInternal();
}


int K3bAudioDecoder::decode( char* _data, int maxLen )
{
  unsigned long lengthToDecode = (length() - d->decodingStartPos).audioBytes();

  if( d->alreadyDecoded >= lengthToDecode )
    return 0;

  // check if we have data left from some previous conversion
  if( d->inBufferLen > 0 ) {
    int len = resample( _data, maxLen );
    d->alreadyDecoded += len;
    return len;
  }

  int len = decodeInternal( _data, maxLen );
  if( len < 0 )
    return -1;

  // TODO: handle mono files
  if( d->channels != 2 )
    return -1;

  else if( len == 0 ) {
    // check if we need to pad
    int bytesToPad = lengthToDecode - d->alreadyDecoded;
    if( bytesToPad > 0 ) {
      kdDebug() << "(K3bAudioDecoder) track length: " << lengthToDecode
		<< "; decoded module data: " << d->alreadyDecoded
		<< "; we need to pad " << bytesToPad << " bytes." << endl;

      ::memset( _data, 0, maxLen );
      if( maxLen < bytesToPad )
	bytesToPad = maxLen;

      d->alreadyDecoded += bytesToPad;
      return bytesToPad;
    }
    else {
      kdDebug() << "(K3bAudioDecoder) decoded " << d->alreadyDecoded << " bytes." << endl;
      return 0;
    }
  }
  else {

    // check if we need to resample
    if( d->samplerate != 44100 ) {
      if( d->inBufferSize < maxLen/2 ) {
	if( d->inBuffer )
	  delete [] d->inBuffer;
	d->inBuffer = new float[maxLen];
	d->inBufferSize = maxLen;
      }
      d->inBufferLen = len/2;
      d->inBufferPos = d->inBuffer;
      from16bitBeSignedToFloat( _data, d->inBuffer, d->inBufferLen );

      len = resample( _data, maxLen );
      if( len < 0 )
	return -1;
    }

    // check if we decoded too much
    if( d->alreadyDecoded + len > lengthToDecode ) {
      kdDebug() << "(K3bAudioDecoder) we decoded too much. Cutting output." << endl;
      len = length().audioBytes() - d->alreadyDecoded;
    }

    d->alreadyDecoded += len;
    return len;
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
  d->resampleData->output_frames = maxLen/2/d->channels;
  d->resampleData->src_ratio = 44100.0/(double)d->samplerate;
  d->resampleData->end_of_input = 0;  // FIXME: at the end of the input this needs to be set to 1

  int len = 0;
  if( (len = src_process( d->resampleState, d->resampleData ) ) ) {
    kdDebug() << "(K3bAudioDecoder) error while resampling: " << src_strerror(len) << endl;
    return -1;
  }

  fromFloatTo16BitBeSigned( d->outBuffer, data, d->resampleData->output_frames_gen*d->channels );
   
  d->inBufferPos += d->resampleData->input_frames_used*d->channels;
  d->inBufferLen -= d->resampleData->input_frames_used*d->channels;
  if( d->inBufferLen <= 0 ) {
    d->inBufferPos = d->inBuffer;
    d->inBufferLen = 0;
  }

  // 16 bit frames, so we need to multiply by 2
  return d->resampleData->output_frames_gen*2*d->channels;
}


void K3bAudioDecoder::from16bitBeSignedToFloat( char* src, float* dest, int samples )
{
  for( int i = 0; i < samples; ++i ) {
    Q_INT16 val = ((src[2*i]<<8)&0xff00)|(src[2*i+1]&0x00ff);
    if( val <= 0 )
      dest[i] = (float)val / 32768.0;
    else
      dest[i] = (float)val / 32767.0;
  }
}


void K3bAudioDecoder::fromFloatTo16BitBeSigned( float* src, char* dest, int samples )
{
  for( int i = 0; i < samples; ++i ) {
    Q_INT16 val = 0;

    // clipping
    if( src[i] > 1.0 )
      val = 32767;
    else if( src[i] < -1.0 )
      val = -32768;

    else if( src[i] <= 0 )
      val = (Q_INT16)(src[i] * 32768.0);
    else
      val = (Q_INT16)(src[i] * 32767.0);

    dest[2*i]   = val>>8;
    dest[2*i+1] = val;
  }
}


bool K3bAudioDecoder::seek( const K3b::Msf& pos )
{
  d->decodingStartPos = pos;
  d->alreadyDecoded = 0;
  return seekInternal( pos );
}


void K3bAudioDecoder::cleanup()
{
}


QString K3bAudioDecoder::metaInfo( const QString& tag )
{
  if( !d->metaInfo )
    d->metaInfo = new KFileMetaInfo( filename() );

  if( d->metaInfo->isValid() ) {
    KFileMetaInfoItem item = d->metaInfo->item( tag );
    if( item.isValid() )
      return item.string();
  }

  return QString::null;
}


#include "k3baudiodecoder.moc"
