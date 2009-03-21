/*
 *
 * Copyright (C) 2006-2008 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bactivepipe.h"

#include <kdebug.h>

#include <qthread.h>
#include <qiodevice.h>


class K3b::ActivePipe::Private : public QThread
{
public:
    Private( K3b::ActivePipe* pipe ) :
        m_pipe( pipe ),
        sourceIODevice(0),
        sinkIODevice(0),
        closeSinkIODevice( false ),
        closeSourceIODevice( false ) {
    }

    void run() {
        kDebug() << "(K3b::ActivePipe) writing from" << sourceIODevice << "to" << sinkIODevice;

        bytesRead = bytesWritten = 0;
        buffer.resize( 10*2048 );

        bool fail = false;
        qint64 r = 0;
        while( !fail && ( r = m_pipe->readData( buffer.data(), buffer.size() ) ) > 0 ) {
            bytesRead += r;

            ssize_t w = 0;
            ssize_t ww = 0;
            while( w < r ) {
                if( ( ww = m_pipe->write( buffer.data()+w, r-w ) ) > 0 ) {
                    w += ww;
                    bytesWritten += ww;
                }
                else {
                    kDebug() << "write failed." << sinkIODevice->errorString();
                    fail = true;
                    break;
                }
            }
        }

        if ( r < 0 ) {
            kDebug() << "Read failed:" << sourceIODevice->errorString();
        }

        kDebug() << "thread done: " << fail << " (total bytes read/written: " << bytesRead << "/" << bytesWritten << ")";
    }

    void _k3b_close() {
        kDebug();
        if ( closeWhenDone )
            m_pipe->close();
    }

private:
    K3b::ActivePipe* m_pipe;

public:
    QIODevice* sourceIODevice;
    QIODevice* sinkIODevice;

    bool closeWhenDone;
    bool closeSinkIODevice;
    bool closeSourceIODevice;

    QByteArray buffer;

    quint64 bytesRead;
    quint64 bytesWritten;
};


K3b::ActivePipe::ActivePipe()
{
    d = new Private( this );
    connect( d, SIGNAL(finished()), this, SLOT(_k3b_close()) );
}


K3b::ActivePipe::~ActivePipe()
{
    delete d;
}


bool K3b::ActivePipe::open( OpenMode mode )
{
    return QIODevice::open( mode );
}


bool K3b::ActivePipe::open( bool closeWhenDone )
{
    if( d->isRunning() )
        return false;

    QIODevice::open( ReadWrite|Unbuffered );

    d->closeWhenDone = closeWhenDone;

    if( d->sourceIODevice && !d->sourceIODevice->isOpen() ) {
        kDebug() << "Need to open source device:" << d->sourceIODevice;
        if( !d->sourceIODevice->open( QIODevice::ReadOnly ) )
            return false;
    }

    if( d->sinkIODevice && !d->sinkIODevice->isOpen()  ) {
        kDebug() << "Need to open sink device:" << d->sinkIODevice;
        if( !d->sinkIODevice->open( QIODevice::WriteOnly ) )
            return false;
    }

    kDebug() << "(K3b::ActivePipe) successfully opened pipe.";

    // we only do active piping if both devices are set.
    // Otherwise we only work as a conduit
    if ( d->sourceIODevice && d->sinkIODevice ) {
        d->start();
    }

    return true;
}


void K3b::ActivePipe::close()
{
    kDebug();
    if( d->sourceIODevice && d->closeSourceIODevice )
        d->sourceIODevice->close();
    if( d->sinkIODevice && d->closeSinkIODevice )
        d->sinkIODevice->close();
    d->wait();
}


void K3b::ActivePipe::readFrom( QIODevice* dev, bool close )
{
    d->sourceIODevice = dev;
    d->closeSourceIODevice = close;
}


void K3b::ActivePipe::writeTo( QIODevice* dev, bool close )
{
    d->sinkIODevice = dev;
    d->closeSinkIODevice = close;
}


qint64 K3b::ActivePipe::readData( char* data, qint64 max )
{
    if( d->sourceIODevice ) {
        return d->sourceIODevice->read( data, max );
    }

    return -1;
}


qint64 K3b::ActivePipe::writeData( const char* data, qint64 max )
{
    if( d->sinkIODevice ) {
        return d->sinkIODevice->write( data, max );
    }
    else
        return -1;
}


quint64 K3b::ActivePipe::bytesRead() const
{
    return d->bytesRead;
}


quint64 K3b::ActivePipe::bytesWritten() const
{
    return d->bytesWritten;
}

#include "k3bactivepipe.moc"
