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

#include <QList>
#include <QMutex>
#include <QMutexLocker>

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
    void slotTrackChanged();

    AudioTrackReader& q;
    AudioTrack& track;
    qint64 pos;
    IODevices readers;
    int current;

    // used to make sure that no seek and read operation occur in parallel
    QMutex mutex;
};


AudioTrackReader::Private::Private( AudioTrackReader& audioTrackReader, AudioTrack& t )
:
    q( audioTrackReader ),
    track( t ),
    pos( 0 ),
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


void AudioTrackReader::Private::slotTrackChanged()
{
    QMutexLocker locker( &mutex );
    if( pos >= q.size() && pos > 0 ) {
        pos = q.size() - 1LL;
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
    qDeleteAll( d->readers.begin(), d->readers.end() );
    d->readers.clear();
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
            d->readers.back()->open( mode );
        }

        d->pos = 0;
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
    return d->pos;
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
        d->pos = pos;
        return d->readers.at( next )->seek( pos - curPos );
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

//     if( d->current < 0 || d->current >= d->readers.size() ) {
//         d->current = 0;
//         if(  d->current < d->readers.size() ) {
//             d->readers.at( d->current )->seek( 0 );
//         }
//         d->pos = 0;
//     }

    if( d->current >= 0 && d->current < d->readers.size() ) {
        qint64 readData = d->readers.at( d->current )->read( data, maxlen );
        if( readData == 0 ) {
            ++d->current;
            if( d->current >= 0 && d->current < d->readers.size() ) {
                d->readers.at( d->current )->seek( 0 );
                return read( data, maxlen ); // read from next source
            }
        }

        d->pos += readData;

        return readData;
    }
    else {
        return -1;
    }
}

} // namespace K3b

#include "k3baudiotrackreader.moc"
