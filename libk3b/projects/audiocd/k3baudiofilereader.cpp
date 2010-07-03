/*
 *
 * Copyright (C) 2005-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2010 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3baudiofilereader.h"
#include "k3baudiofile.h"
#include "k3baudiodecoder.h"


namespace K3b {

class AudioFileReader::Private
{
public:
    Private( AudioFile& s )
    :
        source( s ),
        decodedData( 0 )
    {
    }

    AudioFile& source;
    qint64 decodedData;
};


AudioFileReader::AudioFileReader( AudioFile& source, QObject* parent )
    : QIODevice( parent ),
      d( new Private( source ) )
{
}


AudioFileReader::~AudioFileReader()
{
}


bool AudioFileReader::open( OpenMode mode )
{
    if( !mode.testFlag( QIODevice::WriteOnly ) ) {
        return QIODevice::open( mode );
    }
    else {
        return false;
    }
}


void AudioFileReader::close()
{
    QIODevice::close();
}


bool AudioFileReader::isSequential() const
{
    return false;
}


qint64 AudioFileReader::pos() const
{
    return d->decodedData;
}


qint64 AudioFileReader::size() const
{
    return d->source.length().audioBytes();
}


bool AudioFileReader::seek( qint64 pos )
{
    Msf msf = Msf::fromAudioBytes( pos );
    // this is valid once the decoder has been initialized.
    if( d->source.startOffset() + msf <= d->source.lastSector() &&
        d->source.decoder()->seek( d->source.startOffset() + msf ) ) {
        d->decodedData = pos;
        return true;
    }
    else {
        return false;
    }
}


qint64 AudioFileReader::writeData( const char* /*data*/, qint64 /*len*/ )
{
    return -1;
}


qint64 AudioFileReader::readData( char* data, qint64 maxlen )
{
    // here we can trust on the decoder to always provide enough data
    // see if we decode too much
    if( maxlen + d->decodedData > size() )
        maxlen = size() - d->decodedData;

    qint64 read = d->source.decoder()->decode( data, maxlen );

    if( read > 0 )
        d->decodedData += read;

    return read;
}

} // namespace K3b
