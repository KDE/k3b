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
      outBufferSize(0),
      monoBuffer(0),
      monoBufferSize(0) {
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

  // mono -> stereo conversion
  char* monoBuffer;
  int monoBufferSize;
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
  if( d->monoBuffer ) delete [] d->monoBuffer;
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
  bool ret = analyseFileInternal( &m_length, &d->samplerate, &d->channels );
  if( ret ) {
    return ( ( d->channels == 1 || d->channels == 2 ) && m_length > 0 );
  }
  else
    return false;
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

  int read = 0;

  if( d->samplerate != 44100 ) {

    // check if we have data left from some previous conversion
    if( d->inBufferLen > 0 ) {
      read = resample( _data, maxLen );
    }
    else {    
      if( d->inBufferSize < maxLen/2 ) {
	if( d->inBuffer )
	  delete [] d->inBuffer;
	d->inBuffer = new float[maxLen];
	d->inBufferSize = maxLen;
      }

      read = decodeInternal( _data, maxLen );

      d->inBufferLen = read/2;
      d->inBufferPos = d->inBuffer;
      from16bitBeSignedToFloat( _data, d->inBuffer, d->inBufferLen );
      
      read = resample( _data, maxLen );
    }
  }
  else if( d->channels == 1 ) {
    if( d->monoBufferSize < maxLen/2 ) {
      if( d->monoBuffer )
	delete [] d->monoBuffer;
      d->monoBuffer = new char[maxLen/2];
    }

    // we simply duplicate every frame
    read = decodeInternal( d->monoBuffer, maxLen/2 );
    for( int i = 0; i < read; i+=2 ) {
      _data[2*i] = _data[2*i+2] = d->monoBuffer[i];
      _data[2*i+1] = _data[2*i+3] = d->monoBuffer[i+1];
    }

    read *= 2;
  }
  else {
    read = decodeInternal( _data, maxLen );
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
      kdDebug() << "(K3bAudioDecoder) we decoded too much. Cutting output." << endl;
      read = length().audioBytes() - d->alreadyDecoded;
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
  d->resampleData->end_of_input = 0;  // FIXME: at the end of the input this needs to be set to 1

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


void K3bAudioDecoder::from8BitTo16BitBeSigned( char* src, char* dest, int samples )
{
  for( int i = 0; i < samples; ++i ) {
    float fval = (float)(Q_UINT8(src[i])-128)/128.0;

    Q_INT16 val = 0;
      
    // clipping
    if( fval > 1.0 )
      val = 32767;
    else if( fval < -1.0 )
      val = -32768;

    else if( fval <= 0 )
      val = (Q_INT16)(fval * 32768.0);
    else
      val = (Q_INT16)(fval * 32767.0);

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
