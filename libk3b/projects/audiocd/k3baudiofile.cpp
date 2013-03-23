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
#include "k3baudiodecoder.h"
#include "k3baudiodoc.h"
#include "k3baudiofilereader.h"
#include "k3baudiotrack.h"


class K3b::AudioFile::Private
{
public:
    AudioDoc* doc;
    AudioDecoder* decoder;
};


K3b::AudioFile::AudioFile( K3b::AudioDecoder* decoder, K3b::AudioDoc* doc )
    : K3b::AudioDataSource(),
      d( new Private )
{
    d->doc = doc;
    d->decoder = decoder;
    // FIXME: somehow make it possible to switch docs
    d->doc->increaseDecoderUsage( d->decoder );
}


K3b::AudioFile::AudioFile( const K3b::AudioFile& file )
    : K3b::AudioDataSource( file ),
      d( new Private )
{
    d->doc = file.d->doc;
    d->decoder = file.d->decoder;
    d->doc->increaseDecoderUsage( d->decoder );
}


K3b::AudioFile::~AudioFile()
{
    d->doc->decreaseDecoderUsage( d->decoder );
}


QString K3b::AudioFile::type() const
{
    return d->decoder->fileType();
}


QString K3b::AudioFile::sourceComment() const
{
    return d->decoder->filename().section( '/', -1 );
}


QString K3b::AudioFile::filename() const
{
    return d->decoder->filename();
}


K3b::AudioDecoder* K3b::AudioFile::decoder() const
{
    return d->decoder;
}


K3b::AudioDoc* K3b::AudioFile::doc() const
{
    return d->doc;
}


bool K3b::AudioFile::isValid() const
{
    return d->decoder->isValid();
}


K3b::Msf K3b::AudioFile::originalLength() const
{
    return d->decoder->length();
}


K3b::AudioDataSource* K3b::AudioFile::copy() const
{
    return new K3b::AudioFile( *this );
}


QIODevice* K3b::AudioFile::createReader( QObject* parent )
{
    return new AudioFileReader( *this, parent );
}
