/*
 *
 *
 * Copyright (C) 2004 Sebastian Trueg <trueg@k3b.org>
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

  //
  // we allocate additional MAD_BUFFER_GUARD bytes to always be able to append the
  // zero bytes needed for decoding the last frame.
  //
  m_inputBuffer = new unsigned char[INPUT_BUFFER_SIZE+MAD_BUFFER_GUARD];
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

  m_inputFile.setFileName( filename );

  if( !m_inputFile.open( QIODevice::ReadOnly ) ) {
    kError() << "(K3bMad) could not open file " << m_inputFile.fileName() << endl;
    return false;
  }

  initMad();

  memset( m_inputBuffer, 0, INPUT_BUFFER_SIZE+MAD_BUFFER_GUARD );

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
    qint64 result = m_inputFile.read( (char*)readStart, readSize );
    if( result < 0 ) {
      kDebug() << "(K3bMad) read error on bitstream)";
      m_bInputError = true;
      return false;
    }
    else if( result == 0 ) {
      kDebug() << "(K3bMad) end of input stream";
      return false;
    }
    else {
      readStart += result;

      if( eof() ) {
	kDebug() << "(K3bMad::fillStreamBuffer) MAD_BUFFER_GUARD";
	memset( readStart, 0, MAD_BUFFER_GUARD );
	result += MAD_BUFFER_GUARD;
      }

      // Pipe the new buffer content to libmad's stream decoder facility.
      mad_stream_buffer( madStream, m_inputBuffer, result + remaining );
      madStream->error = MAD_ERROR_NONE;
    }
  }

  return true;
}


bool K3bMad::skipTag()
{
  // skip the tag at the beginning of the file
  m_inputFile.seek( 0 );

  //
  // now check if the file starts with an id3 tag and skip it if so
  //
  char buf[4096];
  int bufLen = 4096;
  if( m_inputFile.read( buf, bufLen ) < bufLen ) {
    kDebug() << "(K3bMad) unable to read " << bufLen << " bytes from "
	      << m_inputFile.fileName() << endl;
    return false;
  }

  if( ( buf[0] == 'I' && buf[1] == 'D' && buf[2] == '3' ) &&
      ( (unsigned short)buf[3] < 0xff && (unsigned short)buf[4] < 0xff ) ) {
    // do we have a footer?
    bool footer = (buf[5] & 0x10);

    // the size is saved as a synched int meaning bit 7 is always cleared to 0
    unsigned int size =
      ( (buf[6] & 0x7f) << 21 ) |
      ( (buf[7] & 0x7f) << 14 ) |
      ( (buf[8] & 0x7f) << 7) |
      (buf[9] & 0x7f);
    unsigned int offset = size + 10;
    if( footer )
      offset += 10;

    kDebug() << "(K3bMad) skipping past ID3 tag to " << offset;

    // skip the id3 tag
    if( !m_inputFile.seek(offset) ) {
      kDebug() << "(K3bMad) " << m_inputFile.fileName()
		<< ": couldn't seek to " << offset << endl;
      return false;
    }
  }
  else {
    // reset file
    return m_inputFile.seek( 0 );
  }

  return true;
}


bool K3bMad::seekFirstHeader()
{
  //
  // A lot of mp3 files start with a lot of junk which confuses mad.
  // We "allow" an mp3 file to start with at most 1 KB of junk. This is just
  // some random value since we do not want to search the hole file. That would
  // take way to long for non-mp3 files.
  //
  bool headerFound = findNextHeader();
  qint64 inputPos = streamPos();
  while( !headerFound &&
	 !m_inputFile.atEnd() &&
	 streamPos() <= inputPos+1024 ) {
    headerFound = findNextHeader();
  }

  // seek back to the begin of the frame
  if( headerFound ) {
    int streamSize = madStream->bufend - madStream->buffer;
    int bytesToFrame = madStream->this_frame - madStream->buffer;
    m_inputFile.seek( m_inputFile.pos() - streamSize + bytesToFrame );

    kDebug() << "(K3bMad) found first header at " << m_inputFile.pos();
  }

  // reset the stream to make sure mad really starts decoding at out seek position
  mad_stream_finish( madStream );
  mad_stream_init( madStream );

  return headerFound;
}


bool K3bMad::eof() const
{
  return m_inputFile.atEnd();
}


qint64 K3bMad::inputPos() const
{
  return m_inputFile.pos();
}


qint64 K3bMad::streamPos() const
{
  return inputPos() - (madStream->bufend - madStream->this_frame + 1);
}


bool K3bMad::inputSeek( qint64 pos )
{
  return m_inputFile.seek( pos );
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
    kDebug() << "(K3bMad) cleanup at offset: "
	      << "Input file at: " << m_inputFile.pos() << " "
	      << "Input file size: " << m_inputFile.size() << " "
	      << "stream pos: "
	      << ( m_inputFile.pos() - (madStream->bufend - madStream->this_frame + 1) )
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
// LOSTSYNC could happen when mad encounters the id3 tag...
//
bool K3bMad::findNextHeader()
{
  if( !fillStreamBuffer() )
    return false;

  //
  // MAD_RECOVERABLE == true:  frame was read, decoding failed (about to skip frame)
  // MAD_RECOVERABLE == false: frame was not read, need data
  //

  if( mad_header_decode( &madFrame->header, madStream ) < 0 ) {
    if( MAD_RECOVERABLE( madStream->error ) ||
	madStream->error == MAD_ERROR_BUFLEN ) {
      return findNextHeader();
    }
    else
      kDebug() << "(K3bMad::findNextHeader) error: " << mad_stream_errorstr( madStream );

    // FIXME probably we should not do this here since we don't do it
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

  if( mad_frame_decode( madFrame, madStream ) < 0 ) {
    if( MAD_RECOVERABLE( madStream->error ) ||
 	madStream->error == MAD_ERROR_BUFLEN ) {
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


