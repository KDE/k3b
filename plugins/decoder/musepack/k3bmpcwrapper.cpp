/* 
 *
 * $Id$
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bmpcwrapper.h"

#include <kdebug.h>
#include <qfile.h>


mpc_int32_t read_impl( void* data, void* ptr, mpc_int32_t size )
{
  QFile* input = static_cast<QFile*>( data );
  return input->readBlock( (char*)ptr, size );
}


mpc_bool_t seek_impl( void* data, mpc_int32_t offset )
{
  QFile* input = static_cast<QFile*>( data );
  return input->at( offset );
}

mpc_int32_t tell_impl( void* data )
{
  QFile* input = static_cast<QFile*>( data );
  return input->at();
}

mpc_int32_t get_size_impl( void* data )
{
  QFile* input = static_cast<QFile*>( data );
  return input->size();
}

mpc_bool_t canseek_impl( void* )
{
  return true;
}


#ifdef MPC_FIXED_POINT
static int shift_signed( MPC_SAMPLE_FORMAT val, int shift )
{
  if(shift > 0)
    val <<= shift;
  else if(shift < 0)
    val >>= -shift;
  return (int) val;
}
#endif


K3bMpcWrapper::K3bMpcWrapper()
{
  m_input = new QFile();

  m_reader           = new mpc_reader;
  m_reader->read     = read_impl;
  m_reader->seek     = seek_impl;
  m_reader->tell     = tell_impl;
  m_reader->get_size = get_size_impl;
  m_reader->canseek  = canseek_impl;
  m_reader->data     = m_input;

  m_decoder          = new mpc_decoder;

  m_info             = new mpc_streaminfo;
}


K3bMpcWrapper::~K3bMpcWrapper()
{
  close();

  delete m_reader;
  delete m_decoder;
  delete m_info;
  delete m_input;
}


bool K3bMpcWrapper::open( const QString& filename )
{
  close();

  m_input->setName( filename );

  if( m_input->open( QIODevice::ReadOnly ) ) {
    mpc_streaminfo_init( m_info );
    if( mpc_streaminfo_read( m_info, m_reader ) != ERROR_CODE_OK ) {
      kDebug() << "(K3bMpcWrapper) Not a valid musepack file: \"" << filename << "\"" << endl;
      return false;
    }
    else {
      mpc_decoder_setup( m_decoder, m_reader );
      if( !mpc_decoder_initialize( m_decoder, m_info ) ) {
	kDebug() << "(K3bMpcWrapper) failed to initialize the Musepack decoder." << endl;
	close();
	return false;
      }
      else {
	kDebug() << "(K3bMpcWrapper) valid musepack file. " 
		  << channels() << " Channels and Samplerate: " << samplerate() << endl;
	return true;
      }
    }
  }
  else
    return false;
}


void K3bMpcWrapper::close()
{
  m_input->close();
}


int K3bMpcWrapper::decode( char* data, int max )
{
  // FIXME: make this a member variable
  MPC_SAMPLE_FORMAT sample_buffer[MPC_DECODER_BUFFER_LENGTH];

  unsigned int samples = mpc_decoder_decode( m_decoder, sample_buffer, 0, 0 );

  if( samples*channels()*2 > (unsigned int)max ) {
    kDebug() << "(K3bMpcWrapper) buffer not big enough." << endl;
    return -1;
  }

  static const unsigned int bps = 16;
  static const int clip_min = -1 << (bps - 1);
  static const int clip_max = (1 << (bps - 1)) - 1;
  static const int float_scale = 1 << (bps - 1);

  for( unsigned int n = 0; n < samples*channels(); ++n ) {
    int val = 0;

#ifdef MPC_FIXED_POINT
    val = shift_signed( sample_buffer[n],
			bps - MPC_FIXED_POINT_SCALE_SHIFT);
#else
    val = (int)(sample_buffer[n] * float_scale);
#endif

    if( val < clip_min )
      val = clip_min;
    else if( val > clip_max )
      val = clip_max;

    data[2*n]   = (val>>8) & 0xff;
    data[2*n+1] = val & 0xff;
  }

  return samples*channels()*2;
}


bool K3bMpcWrapper::seek( const K3b::Msf& msf )
{
  return mpc_decoder_seek_seconds( m_decoder, (double)msf.totalFrames()/75.0 );
}


K3b::Msf K3bMpcWrapper::length() const
{
  return K3b::Msf::fromSeconds( mpc_streaminfo_get_length( m_info ) );
}


int K3bMpcWrapper::samplerate() const
{
  return m_info->sample_freq;
}


unsigned int K3bMpcWrapper::channels() const
{
  return m_info->channels;
}

