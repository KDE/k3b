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

#include <k3bpipe.h>

#include <kdebug.h>

#include <qthread.h>
#include <qiodevice.h>

#include <unistd.h>


class K3b::ActivePipe::Private : public QThread
{
public:
    Private( K3b::ActivePipe* pipe ) :
        m_pipe( pipe ),
        fdToWriteTo(-1),
        sourceIODevice(0),
        sinkIODevice(0),
        closeFdToWriteTo(false) {
    }

    void run() {
        kDebug() << "(K3b::ActivePipe) started thread.";
        bytesRead = bytesWritten = 0;
        buffer.resize( 10*2048 );
        ssize_t r = 0;
        while( ( r = m_pipe->read( buffer.data(), buffer.size() ) ) > 0 ) {

            bytesRead += r;

            // write it out
            ssize_t w = 0;
            ssize_t ww = 0;
            while( w < r ) {
                if( ( ww = m_pipe->write( buffer.data()+w, r-w ) ) > 0 ) {
                    w += ww;
                    bytesWritten += ww;
                }
            }
        }
        //    kDebug() << "(K3b::ActivePipe) thread done: " << r << " (total bytes read/written: " << bytesRead << "/" << bytesWritten << ")";
        close( closeWhenDone );
    }

    int readFd() const {
        return pipeIn.out();
    }

    int writeFd() const {
        if( fdToWriteTo == -1 )
            return pipeOut.in();
        else
            return fdToWriteTo;
    }

    void close( bool closeAll ) {
        if( sourceIODevice )
            sourceIODevice->close();
        if( sinkIODevice )
            sinkIODevice->close();

        if( closeAll ) {
            pipeIn.close();
            pipeOut.close();
            if( fdToWriteTo != -1 &&
                closeFdToWriteTo )
                ::close( fdToWriteTo );
        }
    }

private:
    K3b::ActivePipe* m_pipe;

public:
    int fdToWriteTo;
    K3b::Pipe pipeIn;
    K3b::Pipe pipeOut;

    QIODevice* sourceIODevice;
    QIODevice* sinkIODevice;

    bool closeWhenDone;
    bool closeFdToWriteTo;

    QByteArray buffer;

    quint64 bytesRead;
    quint64 bytesWritten;
};


K3b::ActivePipe::ActivePipe()
{
    d = new Private( this );
}


K3b::ActivePipe::~ActivePipe()
{
    delete d;
}


bool K3b::ActivePipe::open( bool closeWhenDone )
{
    if( d->isRunning() )
        return false;

    d->closeWhenDone = closeWhenDone;

    if( d->sourceIODevice ) {
        if( !d->sourceIODevice->open( QIODevice::ReadOnly ) )
            return false;
    }
    else if( !d->pipeIn.open() ) {
        return false;
    }

    if( d->sinkIODevice ) {
        if( !d->sinkIODevice->open( QIODevice::WriteOnly ) )
            return false;
    }
    else if( d->fdToWriteTo == -1 && !d->pipeOut.open() ) {
        close();
        return false;
    }

    kDebug() << "(K3b::ActivePipe) successfully opened pipe.";

    d->start();
    return true;
}


void K3b::ActivePipe::close()
{
    d->pipeIn.closeIn();
    d->wait();
    d->close( true );
}


void K3b::ActivePipe::writeToFd( int fd, bool close )
{
    d->fdToWriteTo = fd;
    d->sinkIODevice = 0;
    d->closeFdToWriteTo = close;
}


void K3b::ActivePipe::readFromIODevice( QIODevice* dev )
{
    d->sourceIODevice = dev;
}


void K3b::ActivePipe::writeToIODevice( QIODevice* dev )
{
    d->fdToWriteTo = -1;
    d->sinkIODevice = dev;
}


int K3b::ActivePipe::in() const
{
    return d->pipeIn.in();
}


int K3b::ActivePipe::read( char* data, int max )
{
    if( d->sourceIODevice )
        return d->sourceIODevice->read( data, max );
    else
        return ::read( d->readFd(), data, max );
}


int K3b::ActivePipe::write( char* data, int max )
{
    if( d->sinkIODevice )
        return d->sinkIODevice->write( data, max );
    else
        return ::write( d->writeFd(), data, max );
}


quint64 K3b::ActivePipe::bytesRead() const
{
    return d->bytesRead;
}


quint64 K3b::ActivePipe::bytesWritten() const
{
    return d->bytesWritten;
}
