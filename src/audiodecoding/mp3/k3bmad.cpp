/* 
 *
 * $Id$
 * Copyright (C) 2004 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bmad.h"

#include <qfile.h>
#include <kdebug.h>


static const int INPUT_BUFFER_SIZE = 5*8192;


K3bMad::K3bMad()
  : m_madStructuresInitialized(false),
    m_bInputError(false)
{
  madStream = new mad_stream;
  madFrame  = new mad_frame;
  madSynth  = new mad_synth;
  madTimer  = new mad_timer_t;

  m_inputBuffer = new unsigned char[INPUT_BUFFER_SIZE];
}


K3bMad::~K3bMad()
{
  cleanup();

  delete madStream;
  delete madFrame;
  delete madSynth;
  delete madTimer;

  delete [] m_inputBuffer;
}


bool K3bMad::open( const QString& filename )
{
  cleanup();

  m_bInputError = false;
  m_channels = m_sampleRate = 0;

  m_inputFile.setName( filename );
   
  if( !m_inputFile.open( IO_ReadOnly ) ) {
    kdError() << "(K3bMad) could not open file " << m_inputFile.name() << endl;
    return false;
  }

  initMad();

  memset( m_inputBuffer, 0, INPUT_BUFFER_SIZE );

  return true;
}


bool K3bMad::inputError() const
{
  return m_bInputError;
}


bool K3bMad::fillStreamBuffer()
{
  /* The input bucket must be filled if it becomes empty or if
   * it's the first execution of the loop.
   */
  if( madStream->buffer == 0 || madStream->error == MAD_ERROR_BUFLEN ) {
    if( eof() )
      return false;

    long readSize, remaining;
    unsigned char* readStart;

    if( madStream->next_frame != 0 ) {
      remaining = madStream->bufend - madStream->next_frame;
      memmove( m_inputBuffer, madStream->next_frame, remaining );
      readStart = m_inputBuffer + remaining;
      readSize = INPUT_BUFFER_SIZE - remaining;
    }
    else {
      readSize  = INPUT_BUFFER_SIZE;
      readStart = m_inputBuffer;
      remaining = 0;
    }
			
    // Fill-in the buffer. 
    Q_LONG result = m_inputFile.readBlock( (char*)readStart, readSize );
    if( result < 0 ) {
      kdDebug() << "(K3bMad) read error on bitstream)" << endl;
      m_bInputError = true;
      return false;
    }
    else if( result == 0 ) {
      kdDebug() << "(K3bMad) end of input stream" << endl;
      return false;
    }
    else {
      // Pipe the new buffer content to libmad's stream decoder facility.
      mad_stream_buffer( madStream, m_inputBuffer, result + remaining );
      madStream->error = MAD_ERROR_NONE;
    }
  }

  return true;
}


bool K3bMad::skipTag()
{
  //
  // now check if the file starts with an id3 tag and skip it if so
  //
  char buf[4096];
  int bufLen = 4096;
  if( m_inputFile.readBlock( buf, bufLen ) < bufLen ) {
    kdDebug() << "(K3bMad) unable to read " << bufLen << " bytes from " 
	      << m_inputFile.name() << endl;
    return false;
  }

  int offset = 0;

  //
  // We use a loop here since an mp3 file may contain multible id3 tags
  //
  while( ( buf[0] == 'I' && buf[1] == 'D' && buf[2] == '3' ) &&
	 ( (unsigned short)buf[3] < 0xff && (unsigned short)buf[4] < 0xff ) ) {
    kdDebug() << "(K3bMad) found id3 magic: ID3 " 
	      << (unsigned short)buf[3] << "." << (unsigned short)buf[4] 
	      << " at offset " << offset << endl;

    offset = ((buf[6]<<21)|(buf[7]<<14)|(buf[8]<<7)|buf[9]) + 10;

    kdDebug() << "(K3bMad) skipping past ID3 tag to " << offset << endl;

    // skip the id3 tag
    if( !m_inputFile.at(offset) ) {
      kdDebug() << "(K3bMad) " << m_inputFile.name()
		<< ": couldn't seek to " << offset << endl;
      return false;
    }

    // read further to check for additional id3 tags
    if( m_inputFile.readBlock( buf, bufLen ) < bufLen ) {
      kdDebug() << "(K3bMad) unable to read " << bufLen 
		<< " bytes from " << m_inputFile.name() << endl;
      return false;
    }
  }

  // skip any id3 stuff
  if( !m_inputFile.at(offset) ) {
    kdDebug() << "(K3bMad) " << m_inputFile.name()
	      << ": couldn't seek to " << offset << endl;
    return false;
  }

  return true;
}


bool K3bMad::eof() const
{ 
  return m_inputFile.atEnd();
}


QIODevice::Offset K3bMad::inputPos() const
{
  return m_inputFile.at();
}


bool K3bMad::inputSeek( QIODevice::Offset pos )
{
  return m_inputFile.at( pos );
}


void K3bMad::initMad()
{
  if( !m_madStructuresInitialized ) {
    mad_stream_init( madStream );
    mad_timer_reset( madTimer );
    mad_frame_init( madFrame );
    mad_synth_init( madSynth );

    m_madStructuresInitialized = true;
  }
}


void K3bMad::cleanup()
{
  if( m_inputFile.isOpen() ) {
    kdDebug() << "(K3bMad) cleanup at offset: " 
	      << "Input file at: " << m_inputFile.at() << " "
	      << "Input file size: " << m_inputFile.size() << " "
	      << "stream pos: " 
	      << ( m_inputFile.at() - (madStream->bufend - madStream->this_frame + 1) )
	      << endl;
    m_inputFile.close();
  }
    
  if( m_madStructuresInitialized ) {
    mad_frame_finish( madFrame );
    mad_synth_finish( madSynth );
    mad_stream_finish( madStream );
  }
    
  m_madStructuresInitialized = false;
}


//
// This is from the arts mad decoder and I did not yet understand it fully. :(
//
// LOSTSYNC could happen when mad encounters the id3 tag...
//
bool K3bMad::findNextHeader()
{
  if( !fillStreamBuffer() )
    return false;

  if( mad_header_decode( &madFrame->header, madStream ) < 0 ) {
    if( !MAD_RECOVERABLE( madStream->error ) ||
	madStream->error == MAD_ERROR_LOSTSYNC ) {
      return findNextHeader();
    }

    // FIXME probably we should not do this here since we son't do it
    // in the frame decoding
//     if( !checkFrameHeader( &madFrame->header ) )
//       return findNextHeader();

    return false;
  }

  if( !m_channels ) {
    m_channels = MAD_NCHANNELS(&madFrame->header);
    m_sampleRate = madFrame->header.samplerate;
  }

  mad_timer_add( madTimer, madFrame->header.duration );

  return true;
}


bool K3bMad::decodeNextFrame()
{
  if( !fillStreamBuffer() )
    return false;

  //
  // MAD_RECOVERABLE == true:  frame was read, decoding failed (about to skip frame)
  // MAD_RECOVERABLE == false: frame was not read, need data
  //

  if( mad_frame_decode( madFrame, madStream ) < 0 ) {
    if( !MAD_RECOVERABLE( madStream->error ) ||
	madStream->error == MAD_ERROR_LOSTSYNC ) {
      return decodeNextFrame();
    }

    return false;
  }

  if( !m_channels ) {
    m_channels = MAD_NCHANNELS(&madFrame->header);
    m_sampleRate = madFrame->header.samplerate;
  }

  mad_timer_add( madTimer, madFrame->header.duration );

  return true;
}


//
// This is from the arts mad decoder
//
bool K3bMad::checkFrameHeader( mad_header* header ) const
{
  int frameSize = MAD_NSBSAMPLES( header ) * 32;

  if( frameSize <= 0 )
    return false;

  if( m_channels && m_channels != MAD_NCHANNELS(header) )
    return false;
    
  return true;
}


