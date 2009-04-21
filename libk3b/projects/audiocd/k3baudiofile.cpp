/*
 *
 * Copyright (C) 2004-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
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

#include "k3baudiodecoder.h"


K3b::AudioFile::AudioFile( K3b::AudioDecoder* dec, K3b::AudioDoc* doc )
    : K3b::AudioDataSource(),
      m_doc(doc),
      m_decoder(dec),
      m_decodedData(0)
{
    // FIXME: somehow make it possible to switch docs
    doc->increaseDecoderUsage( m_decoder );
}


K3b::AudioFile::AudioFile( const K3b::AudioFile& file )
    : K3b::AudioDataSource( file ),
      m_doc( file.m_doc ),
      m_decoder( file.m_decoder ),
      m_decodedData(0)
{
    m_doc->increaseDecoderUsage( m_decoder );
}


K3b::AudioFile::~AudioFile()
{
    m_doc->decreaseDecoderUsage( m_decoder );
}


QString K3b::AudioFile::type() const
{
    return m_decoder->fileType();
}


QString K3b::AudioFile::sourceComment() const
{
    return m_decoder->filename().section( "/", -1 );
}


QString K3b::AudioFile::filename() const
{
    return m_decoder->filename();
}


bool K3b::AudioFile::isValid() const
{
    return m_decoder->isValid();
}


K3b::Msf K3b::AudioFile::originalLength() const
{
    return m_decoder->length();
}


bool K3b::AudioFile::seek( const K3b::Msf& msf )
{
    // this is valid once the decoder has been initialized.
    if( startOffset() + msf <= lastSector() &&
        m_decoder->seek( startOffset() + msf ) ) {
        m_decodedData = msf.audioBytes();
        return true;
    }
    else
        return false;
}


int K3b::AudioFile::read( char* data, unsigned int max )
{
    // here we can trust on the decoder to always provide enough data
    // see if we decode too much
    if( max + m_decodedData > length().audioBytes() )
        max = length().audioBytes() - m_decodedData;

    int read = m_decoder->decode( data, max );

    if( read > 0 )
        m_decodedData += read;

    return read;
}


K3b::AudioDataSource* K3b::AudioFile::copy() const
{
    return new K3b::AudioFile( *this );
}
