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

#include "k3baudiotrackreader.h"
#include "k3baudiodatasource.h"
#include "k3baudiotrack.h"

#include <QLinkedList>

namespace K3b {

namespace {
    typedef QLinkedList< QIODevice* > IODevices;
}

class AudioTrackReader::Private
{
public:
    Private( AudioTrack& t )
    :
        track( t ),
        alreadyReadBytes( 0 ),
        currentReader( readers.end() )
    {
    }

    AudioTrack& track;
    qint64 alreadyReadBytes;
    IODevices readers;
    IODevices::const_iterator currentReader;
};


AudioTrackReader::AudioTrackReader( AudioTrack& track, QObject* parent )
    : QIODevice( parent ),
      d( new Private( track ) )
{
}


AudioTrackReader::~AudioTrackReader()
{
    qDeleteAll( d->readers.begin(), d->readers.end() );
    d->readers.clear();
}


bool AudioTrackReader::open( QIODevice::OpenMode mode )
{
    if( !mode.testFlag( QIODevice::WriteOnly ) && d->readers.empty() && d->track.numberSources() > 0 ) {

        for( AudioDataSource* source = d->track.firstSource(); source != 0; source = source->next() ) {
            d->readers.push_back( source->createReader() );
        }

        d->alreadyReadBytes = 0;
        d->currentReader = d->readers.begin();
        if( d->currentReader != d->readers.end() ) {
            (*d->currentReader)->seek( 0 );
        }

        return QIODevice::open( mode );
    }
    else {
        return false;
    }
}


void AudioTrackReader::close()
{
    qDeleteAll( d->readers.begin(), d->readers.end() );
    d->readers.clear();
    QIODevice::close();
}


bool AudioTrackReader::isSequential() const
{
    return false;
}


qint64 AudioTrackReader::pos() const
{
    return d->alreadyReadBytes;
}


qint64 AudioTrackReader::size() const
{
    return d->track.length().audioBytes();
}


bool AudioTrackReader::seek( qint64 pos )
{
    IODevices::const_iterator reader = d->readers.begin();
    qint64 curPos = 0;

    for( ; reader != d->readers.end() && curPos + (*reader)->size() < pos; ++reader ) {
        curPos += (*reader)->size();
    }

    if( reader != d->readers.end() ) {
        d->currentReader = reader;
        d->alreadyReadBytes = pos;
        return (*reader)->seek( pos - curPos );
    }
    else {
        return false;
    }
}


qint64 AudioTrackReader::writeData( const char* /*data*/, qint64 /*len*/ )
{
    return -1;
}


qint64 AudioTrackReader::readData( char* data, qint64 maxlen )
{
    if( d->currentReader == d->readers.end() ) {
        d->currentReader = d->readers.begin();
        if( d->currentReader != d->readers.end() ) {
            (*d->currentReader)->seek( 0 );
        }
        d->alreadyReadBytes = 0;
    }

    qint64 readData = (*d->currentReader)->read( data, maxlen );
    if( readData == 0 ) {
        ++d->currentReader;
        if( d->currentReader != d->readers.end() ) {
            (*d->currentReader)->seek( 0 );
            return read( data, maxlen ); // read from next source
        }
    }

    d->alreadyReadBytes += readData;

    return readData;
}

} // namespace K3b
