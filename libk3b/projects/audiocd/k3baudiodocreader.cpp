/*
    SPDX-FileCopyrightText: 2004 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2010 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3baudiodocreader.h"
#include "k3baudiodoc.h"
#include "k3baudiotrack.h"
#include "k3baudiotrackreader.h"

#include <QList>
#include <QMutex>
#include <QMutexLocker>

namespace K3b {

namespace {
    typedef QList< AudioTrackReader* > AudioTrackReaders;
}

class AudioDocReader::Private
{
public:
    Private( AudioDocReader& audioDocReader, AudioDoc& d );
    void setCurrentReader( int position );
    void slotTrackAdded( int position );
    void slotTrackAboutToBeRemoved( int position );

    AudioDocReader& q;
    AudioDoc& doc;
    AudioTrackReaders readers;
    int current;

    // used to make sure that no seek and read operation occur in parallel
    QMutex mutex;
};


AudioDocReader::Private::Private( AudioDocReader& audioDocReader, AudioDoc& d )
:
    q( audioDocReader ),
    doc( d ),
    current( -1 )
{
}


void AudioDocReader::Private::setCurrentReader( int position )
{
    if( position >= 0 && position < readers.size() && position != current ) {
        emit q.currentTrackChanged( readers.at( position )->track() );
    }
    current = position;
}


void AudioDocReader::Private::slotTrackAdded( int position )
{
    QMutexLocker locker( &mutex );
    if( q.isOpen() && position >= 0 && position <= readers.size() ) { // No mistake here, "position" can have size() value
        if( AudioTrack* track = doc.getTrack( position + 1 ) ) {
            readers.insert( position, new AudioTrackReader( *track ) );
            readers.at( position )->open( q.openMode() );
            if( position == current )
                readers.at( position )->seek( 0 );
        }
    }
}


void AudioDocReader::Private::slotTrackAboutToBeRemoved( int position )
{
    QMutexLocker locker( &mutex );
    if( q.isOpen() ) {
        if( position >= 0 && position < readers.size() ) {
            readers.removeAt( position );
            if( position == current ) {
                if( current < readers.size() - 1 )
                    setCurrentReader( ++current );
                else
                    setCurrentReader( --current );
            }
        }
    }
}


AudioDocReader::AudioDocReader( AudioDoc& doc, QObject* parent )
    : QIODevice( parent ),
      d( new Private( *this, doc ) )
{
    connect( &doc, SIGNAL(trackAdded(int)),
             this, SLOT(slotTrackAdded(int)) );
    connect( &doc, SIGNAL(trackAboutToBeRemoved(int)),
             this, SLOT(slotTrackAboutToBeRemoved(int)) );
}


AudioDocReader::~AudioDocReader()
{
    close();
}


AudioTrackReader* AudioDocReader::currentTrackReader() const
{
    if( d->current >=0 && d->current < d->readers.size() )
        return d->readers.at( d->current );
    else
        return nullptr;
}


bool AudioDocReader::setCurrentTrack( const AudioTrack& track )
{
    for( int position = 0; position < d->readers.size(); ++position ) {
        AudioTrackReader* reader = d->readers.at( position );
        if( &reader->track() == &track ) {
            d->setCurrentReader( position );
            updatePos();
            reader->seek( 0 );
            return true;
        }
    }
    return false;
}


bool AudioDocReader::open( QIODevice::OpenMode mode )
{
    if( !mode.testFlag( QIODevice::WriteOnly ) && d->readers.empty() && d->doc.numOfTracks() > 0 ) {

        for( AudioTrack* track = d->doc.firstTrack(); track != nullptr; track = track->next() ) {
            d->readers.push_back( new AudioTrackReader( *track ) );
            if( !d->readers.back()->open( mode ) ) {
                close();
                return false;
            }
        }

        QIODevice::seek( 0 );
        d->setCurrentReader( 0 );
        if( d->current >=0 && d->current < d->readers.size() ) {
            d->readers.at( d->current )->seek( 0 );
        }

        return QIODevice::open( mode );
    }
    else {
        return false;
    }
}


void AudioDocReader::close()
{
    qDeleteAll( d->readers );
    d->readers.clear();
    d->current = -1;
    QIODevice::close();
}


bool AudioDocReader::isSequential() const
{
    return false;
}


qint64 AudioDocReader::size() const
{
    return d->doc.length().audioBytes();
}


bool AudioDocReader::seek( qint64 pos )
{
    QMutexLocker locker( &d->mutex );

    int reader = 0;
    qint64 curPos = 0;

    for( ; reader < d->readers.size() && curPos + d->readers.at( reader )->size() < pos; ++reader ) {
        curPos += d->readers.at( reader )->size();
    }

    if( reader < d->readers.size() ) {
        d->setCurrentReader( reader );
        d->readers.at( reader )->seek( pos - curPos );
        return QIODevice::seek( pos );
    }
    else {
        return false;
    }
}


void AudioDocReader::nextTrack()
{
    QMutexLocker locker( &d->mutex );
    if( d->current >= 0 && d->current < d->readers.size() ) {
        d->setCurrentReader( d->current + 1 );
        updatePos();
        if( d->current >= 0 && d->current < d->readers.size() ) {
            d->readers.at( d->current )->seek( 0 );
        }
    }
}


void AudioDocReader::previousTrack()
{
    QMutexLocker locker( &d->mutex );
    if( d->current >= 0 && d->current < d->readers.size() ) {
        d->setCurrentReader( d->current - 1 );
        updatePos();
        if( d->current >= 0 && d->current < d->readers.size() ) {
            d->readers.at( d->current )->seek( 0 );
        }
    }
}


qint64 AudioDocReader::writeData( const char* /*data*/, qint64 /*len*/ )
{
    return -1;
}


qint64 AudioDocReader::readData( char* data, qint64 maxlen )
{
    QMutexLocker locker( &d->mutex );

    while( d->current >= 0 && d->current < d->readers.size() ) {
        qint64 readData = d->readers.at( d->current )->read( data, maxlen );

        if( readData >= 0 ) {
            return readData;
        }
        else {
            d->setCurrentReader( d->current + 1 );
            if( d->current >= 0 && d->current < d->readers.size() ) {
                d->readers.at( d->current )->seek( 0 );
            }
        }
    }

    return -1;
}


void AudioDocReader::updatePos()
{
    if( d->current >= 0 && d->current < d->readers.size() ) {
        qint64 newPos = 0LL;
        Q_FOREACH( AudioTrackReader* reader, d->readers ) {
            if( reader != d->readers.at( d->current ) )
                newPos += reader->size();
            else
                break;
        }
        QIODevice::seek( newPos );
    }
}

} // namespace K3b

#include "moc_k3baudiodocreader.cpp"
