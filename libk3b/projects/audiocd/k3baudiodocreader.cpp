/*
 *
 * Copyright (C) 2004 Sebastian Trueg <trueg@k3b.org>
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

#include "k3baudiodocreader.h"
#include "k3baudiodoc.h"
#include "k3baudiotrack.h"
#include "k3baudiotrackreader.h"

#include <QLinkedList>

namespace K3b {

namespace {
    typedef QLinkedList< AudioTrackReader* > AudioTrackReaders;
    typedef AudioTrackReaders::const_iterator AudioTrackReaderIterator;
}

class AudioDocReader::Private
{
public:
    Private( AudioDoc& d )
    :
        doc( d ),
        pos( 0 ),
        currentReader( readers.end() )
    {
    }

    void updatePos();

    AudioDoc& doc;
    qint64 pos;
    AudioTrackReaders readers;
    AudioTrackReaderIterator currentReader;
};


void AudioDocReader::Private::updatePos()
{
    if( currentReader != readers.end() ) {
        qint64 newPos = 0LL;
        Q_FOREACH( AudioTrackReader* reader, readers ) {
            if( reader != *currentReader )
                newPos += reader->size();
            else
                break;
        }
        pos = newPos;
    }
}


AudioDocReader::AudioDocReader( AudioDoc& doc, QObject* parent )
    : QIODevice( parent ),
      d( new Private( doc ) )
{
}


AudioDocReader::~AudioDocReader()
{
    qDeleteAll( d->readers.begin(), d->readers.end() );
    d->readers.clear();
}


AudioTrackReader* AudioDocReader::currentTrackReader() const
{
    if( d->currentReader != d->readers.end() )
        return *d->currentReader;
    else
        return 0;
}


bool AudioDocReader::setCurrentTrack( const AudioTrack& track )
{
    qint64 newPos = 0LL;

    for( AudioTrackReaderIterator reader = d->readers.begin();
         reader != d->readers.end(); ++reader ) {
        if( &(*reader)->track() == &track ) {
            d->pos = newPos;
            d->currentReader = reader;
            emit currentTrackChanged( (*d->currentReader)->track() );
            (*d->currentReader)->seek( 0 );
            return true;
        }
        else {
            newPos += (*reader)->size();
        }
    }
    return false;
}


bool AudioDocReader::open( QIODevice::OpenMode mode )
{
    if( !mode.testFlag( QIODevice::WriteOnly ) && d->readers.empty() && d->doc.numOfTracks() > 0 ) {

        for( AudioTrack* track = d->doc.firstTrack(); track != 0; track = track->next() ) {
            d->readers.push_back( new AudioTrackReader( *track ) );
            d->readers.back()->open( mode );
        }

        d->pos = 0;
        d->currentReader = d->readers.begin();
        if( d->currentReader != d->readers.end() ) {
            emit currentTrackChanged( (*d->currentReader)->track() );
            (*d->currentReader)->seek( 0 );
        }

        return QIODevice::open( mode );
    }
    else {
        return false;
    }
}


void AudioDocReader::close()
{
    qDeleteAll( d->readers.begin(), d->readers.end() );
    d->readers.clear();
    QIODevice::close();
}


bool AudioDocReader::isSequential() const
{
    return false;
}


qint64 AudioDocReader::pos() const
{
    return d->pos;
}


qint64 AudioDocReader::size() const
{
    return d->doc.length().audioBytes();
}


bool AudioDocReader::seek( qint64 pos )
{
    AudioTrackReaderIterator reader = d->readers.begin();
    qint64 curPos = 0;

    for( ; reader != d->readers.end() && curPos + (*reader)->size() < pos; ++reader ) {
        curPos += (*reader)->size();
    }

    if( reader != d->readers.end() ) {
        d->currentReader = reader;
        d->pos = pos;
        emit currentTrackChanged( (*d->currentReader)->track() );
        return (*reader)->seek( pos - curPos );
    }
    else {
        return false;
    }
}


void AudioDocReader::nextTrack()
{
    if( d->currentReader != d->readers.end() ) {
        ++d->currentReader;
        d->updatePos();
        if( d->currentReader != d->readers.end() ) {
            emit currentTrackChanged( (*d->currentReader)->track() );
            (*d->currentReader)->seek( 0 );
        }
    }
}


void AudioDocReader::previousTrack()
{
    if( d->currentReader != d->readers.end() ) {
        --d->currentReader;
        d->updatePos();
        if( d->currentReader != d->readers.end() ) {
            emit currentTrackChanged( (*d->currentReader)->track() );
            (*d->currentReader)->seek( 0 );
        }
    }
}


qint64 AudioDocReader::writeData( const char* /*data*/, qint64 /*len*/ )
{
    return -1;
}


qint64 AudioDocReader::readData( char* data, qint64 maxlen )
{
    if( d->currentReader == d->readers.end() ) {
        d->currentReader = d->readers.begin();
        if( d->currentReader != d->readers.end() ) {
            emit currentTrackChanged( (*d->currentReader)->track() );
            (*d->currentReader)->seek( 0 );
        }
        d->pos = 0;
    }

    qint64 readData = (*d->currentReader)->read( data, maxlen );
    if( readData < 0 ) {
        ++d->currentReader;
        if( d->currentReader != d->readers.end() ) {
            emit currentTrackChanged( (*d->currentReader)->track() );
            (*d->currentReader)->seek( 0 );
            return read( data, maxlen ); // read from next source
        }
    }

    d->pos += readData;

    return readData;
}

} // namespace K3b

#include "k3baudiodocreader.moc"
