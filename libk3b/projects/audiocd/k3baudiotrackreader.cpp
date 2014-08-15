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

#include <QtCore/QList>
#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>

namespace K3b {

namespace {
    typedef QList< QIODevice* > IODevices;
}

class AudioTrackReader::Private
{
public:
    Private( AudioTrackReader& audioTrackReader, AudioTrack& t );
    void slotSourceAdded( int position );
    void slotSourceAboutToBeRemoved( int position );

    AudioTrackReader& q;
    AudioTrack& track;
    IODevices readers;
    int current;

    // used to make sure that no seek and read operation occur in parallel
    QMutex mutex;
};


AudioTrackReader::Private::Private( AudioTrackReader& audioTrackReader, AudioTrack& t )
:
    q( audioTrackReader ),
    track( t ),
    current( -1 )
{
}


void AudioTrackReader::Private::slotSourceAdded( int position )
{
    if( q.isOpen() ) {
        QMutexLocker locker( &mutex );
        if( position >= 0 && position <= readers.size() ) { // No mistake here, "position" can have size() value
            if( AudioDataSource* source = track.getSource( position ) ) {
                readers.insert( position, source->createReader() );
                readers.at( position )->open( q.openMode() );
                if( position == current )
                    readers.at( position )->seek( 0 );
            }
        }
    }
}


void AudioTrackReader::Private::slotSourceAboutToBeRemoved( int position )
{
    if( q.isOpen() ) {
        QMutexLocker locker( &mutex );
        if( position >= 0 && position < readers.size() ) {
            if( position == current )
                ++current;
            readers.removeAt( position );
        }
    }
}


AudioTrackReader::AudioTrackReader( AudioTrack& track, QObject* parent )
    : QIODevice( parent ),
      d( new Private( *this, track ) )
{
    connect( &track, SIGNAL(sourceAdded(int)),
             this, SLOT(slotSourceAdded(int)) );
    connect( &track, SIGNAL(sourceAboutToBeRemoved(int)),
             this, SLOT(slotSourceAboutToBeRemoved(int)) );
    connect( &track, SIGNAL(changed()),
             this, SLOT(slotTrackChanged()) );
}


AudioTrackReader::~AudioTrackReader()
{
    close();
    d->current = -1;
}


const AudioTrack& AudioTrackReader::track() const
{
    return d->track;
}


AudioTrack& AudioTrackReader::track()
{
    return d->track;
}


bool AudioTrackReader::open( QIODevice::OpenMode mode )
{
    if( !mode.testFlag( QIODevice::WriteOnly ) && d->readers.empty() && d->track.numberSources() > 0 ) {

        for( AudioDataSource* source = d->track.firstSource(); source != 0; source = source->next() ) {
            d->readers.push_back( source->createReader() );
            if( !d->readers.back()->open( mode ) ) {
                d->readers.clear();
                return false;
            }
        }

        QIODevice::seek( 0 );
        if( !d->readers.isEmpty() ) {
            d->current = 0;
            d->readers.at( d->current )->seek( 0 );
        }

        return QIODevice::open( mode );
    }
    else {
        return false;
    }
}


void AudioTrackReader::close()
{
    qDeleteAll( d->readers );
    d->readers.clear();
    d->current = -1;
    QIODevice::close();
}


bool AudioTrackReader::isSequential() const
{
    return false;
}


qint64 AudioTrackReader::size() const
{
    return d->track.length().audioBytes();
}


bool AudioTrackReader::seek( qint64 pos )
{
    QMutexLocker locker( &d->mutex );

    int next = 0;
    qint64 curPos = 0;

    for( ; next < d->readers.size() && curPos + d->readers.at( next )->size() < pos; ++next ) {
        curPos += d->readers.at( next )->size();
    }

    if( next < d->readers.size() ) {
        d->current = next;
        d->readers.at( next )->seek( pos - curPos );
        return QIODevice::seek( pos );
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
    QMutexLocker locker( &d->mutex );

    while( d->current >= 0 && d->current < d->readers.size() ) {
        qint64 readData = d->readers.at( d->current )->read( data, maxlen );

        if( readData >= 0 ) {
            return readData;
        }
        else {
            ++d->current;
            if( d->current >= 0 && d->current < d->readers.size() ) {
                d->readers.at( d->current )->seek( 0 );
            }
        }
    }

    return -1;
}


void AudioTrackReader::slotTrackChanged()
{
    QMutexLocker locker( &d->mutex );
    if( pos() >= size() && pos() > 0 ) {
        QIODevice::seek( size() - 1LL );
    }
}

} // namespace K3b

#include "moc_k3baudiotrackreader.cpp"
