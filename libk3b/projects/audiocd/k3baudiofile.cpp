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

#include "k3baudiofile.h"
#include "k3baudiodoc.h"
#include "k3baudiotrack.h"

#include <k3baudiodecoder.h>


K3bAudioFile::K3bAudioFile( K3bAudioDecoder* dec, K3bAudioDoc* doc )
  : K3bAudioDataSource(),
    m_doc(doc),
    m_decoder(dec),
    m_startOffset(0),
    m_endOffset(0),
    m_decodedData(0)
{
  // FIXME: somehow make it possible to switch docs
  doc->increaseDecoderUsage( m_decoder );
}


K3bAudioFile::~K3bAudioFile()
{
  m_doc->decreaseDecoderUsage( m_decoder );
}


QString K3bAudioFile::type() const
{
  return m_decoder->fileType();
}


QString K3bAudioFile::sourceComment() const
{
  return m_decoder->filename().section( "/", -1 );
}


const QString& K3bAudioFile::filename() const
{
  return m_decoder->filename();
}


bool K3bAudioFile::isValid() const
{
  return m_decoder->isValid();
}


K3b::Msf K3bAudioFile::length() const
{
  //
  // we need the length to be 0 if the decoder did not finish
  // analysing yet but bigger than 0 otherwise
  //
  if( fileLength() == 0 )
    return 0;
  else if( lastSector() < m_startOffset )
    return 1;
  else
    return lastSector() - m_startOffset + 1;
}


K3b::Msf K3bAudioFile::fileLength() const
{
  return m_decoder->length();
}


K3b::Msf K3bAudioFile::lastSector() const
{
  if( m_endOffset > 0 )
    return m_endOffset-1;
  else
    return fileLength()-1;
}


void K3bAudioFile::setStartOffset( const K3b::Msf& msf )
{
  m_startOffset = msf;
  emitChange();
}


void K3bAudioFile::setEndOffset( const K3b::Msf& msf )
{
  m_endOffset = msf;
  emitChange();
}


bool K3bAudioFile::seek( const K3b::Msf& msf )
{
  fixupOffsets();

  // this is valid once the decoder has been initialized.
  if( m_startOffset + msf <= lastSector() &&
      m_decoder->seek( m_startOffset + msf ) ) {
    m_decodedData = msf.audioBytes();
    return true;
  }
  else
    return false;
}


int K3bAudioFile::read( char* data, unsigned int max )
{
  fixupOffsets();

  // here we can trust on the decoder to always provide enough data
  // see if we decode too much
  if( max + m_decodedData > length().audioBytes() )
    max = length().audioBytes() - m_decodedData;

  int read = m_decoder->decode( data, max );

  if( read > 0 )
    m_decodedData += read;

  return read;
}


// we fixup here to be able to use invalid values before the length has
// been calculated
void K3bAudioFile::fixupOffsets()
{

  // HMMM.... if we emitChange here every opened project will be modified after the length have been calculated.

  if( m_startOffset >= fileLength() ) {
    m_startOffset = 0;
    //    emitChange();
  }
  if( m_endOffset > fileLength() ) {
    m_endOffset = 0; // whole file
    //    emitChange();
  }
  if( m_endOffset > 0 && m_endOffset <= m_startOffset ) {
    m_endOffset = m_startOffset;
    //    emitChange();
  }
}


K3bAudioDataSource* K3bAudioFile::copy() const
{
  K3bAudioFile* file = new K3bAudioFile( decoder(), m_doc );
  file->m_startOffset = m_startOffset;
  file->m_endOffset = m_endOffset;
  return file;
}


K3bAudioDataSource* K3bAudioFile::split( const K3b::Msf& pos )
{
  if( pos < length() ) {
    K3bAudioFile* file = new K3bAudioFile( decoder(), m_doc );
    file->m_startOffset = m_startOffset + pos;
    file->m_endOffset = m_endOffset;
    m_endOffset = m_startOffset + pos;
    file->moveAfter( this );
    emitChange();
    return file;
  }
  else
    return 0;
}
