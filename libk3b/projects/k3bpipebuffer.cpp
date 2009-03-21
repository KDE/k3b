/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bpipebuffer.h"

#include <k3bthread.h>

#include <klocale.h>
#include <kdebug.h>

#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>


//
// This one is based on the little pipebuf2 program by Peter Osterlund <petero2@telia.com>
//


class K3b::PipeBuffer::Private
{
public:
    Private()
        : buffer(0),
          bufSize(4*1024*1024) {
        outFd = inFd = -1;
        inFdPair[0] = inFdPair[1] = -1;
    }

    ~Private() {
        delete [] buffer;
    }

    bool initFds() {
        if( inFd == -1 ) {
            if( ::socketpair(AF_UNIX, SOCK_STREAM, 0, inFdPair) ) {
                //      if( ::pipe( inFdPair ) ) {
                kDebug() << "(K3b::PipeBuffer::WorkThread) unable to create socketpair";
                inFdPair[0] = inFdPair[1] = -1;
                return false;
            }
            else {
                ::fcntl(inFdPair[0], F_SETFL, O_NONBLOCK);
                ::fcntl(outFd, F_SETFL, O_NONBLOCK);
            }
        }
        else {
            ::fcntl(inFd, F_SETFL, O_NONBLOCK);
        }

        delete [] buffer;
        buffer = new char[bufSize];

        return (buffer != 0);
    }

    char* buffer;
    unsigned int bufSize;
    QIODevice* outIoDevice;
    QIODevice* inIoDevice;
    int inFdPair[2];
};


K3b::PipeBuffer::PipeBuffer( K3b::JobHandler* jh, QObject* parent )
    : K3b::ThreadJob( jh, parent ),
      d( new Private() )
{
}


K3b::PipeBuffer::~PipeBuffer()
{
    delete d;
}


void K3b::PipeBuffer::start()
{
    //
    // Create the socketpair in the gui thread to be sure it's available after
    // this method returns.
    //
    if( !d->initFds() )
        jobFinished(false);
    else
        K3b::ThreadJob::start();
}


void K3b::PipeBuffer::setBufferSize( int mb )
{
    d->bufSize = mb * 1024 * 1024;
}


void K3b::PipeBuffer::readFromFd( int fd )
{
    d->inFd = fd;
}


void K3b::PipeBuffer::writeToFd( int fd )
{
    d->outFd = fd;
}


int K3b::PipeBuffer::inFd() const
{
    if( d->inFd == -1 )
        return d->inFdPair[1];
    else
        return d->inFd;
}

bool K3b::PipeBuffer::run()
{
    int usedInFd = -1;
    if( d->inFd > 0 )
        usedInFd = d->inFd;
    else
        usedInFd = d->inFdPair[0];

    kDebug() << "(K3b::PipeBuffer::WorkThread) reading from " << usedInFd
             << " and writing to " << d->outFd << endl;
    kDebug() << "(K3b::PipeBuffer::WorkThread) using buffer size of " << d->bufSize;

    // start the buffering
    unsigned int bufPos = 0;
    unsigned int dataLen = 0;
    bool eof = false;
    bool error = false;
    int oldPercent = 0;

    static const unsigned int MAX_BUFFER_READ = 2048*3;

    while( !canceled() && !error && (!eof || dataLen > 0) ) {
        //
        // create two fd sets
        //
        fd_set readFds, writeFds;
        FD_ZERO(&readFds);
        FD_ZERO(&writeFds);

        //
        // fill the fd sets
        //
        if( !eof && dataLen < d->bufSize )
            FD_SET(usedInFd, &readFds);
        if( dataLen > 0 )
            FD_SET(d->outFd, &writeFds);

        //
        // wait for data
        //
        int ret = select( qMax(usedInFd, d->outFd) + 1, &readFds, &writeFds, NULL, NULL);

        //
        // Do the buffering
        //
        if( !canceled() && ret > 0 ) {

            int currentPercent = -1;

            //
            // Read from the buffer and write to the output
            //
            if( FD_ISSET(d->outFd, &writeFds) ) {
                unsigned int maxLen = qMin(d->bufSize - bufPos, dataLen);

                ret = ::write( d->outFd, &d->buffer[bufPos], maxLen );

                if( ret < 0 ) {
                    if( (errno != EINTR) && (errno != EAGAIN) ) {
                        kDebug() << "(K3b::PipeBuffer::WorkThread) error while writing to " << d->outFd;
                        error = true;
                    }
                }
                else {
                    //
                    // we always emit before the reading from the buffer since
                    // it makes way more sense to show the buffer before the reading.
                    //
                    currentPercent = (int)((double)dataLen*100.0/(double)d->bufSize);

                    bufPos = (bufPos + ret) % d->bufSize;
                    dataLen -= ret;
                }
            }

            //
            // Read into the buffer
            //
            else if( FD_ISSET(usedInFd, &readFds) ) {
                unsigned int readPos = (bufPos + dataLen) % d->bufSize;
                unsigned int maxLen = qMin(d->bufSize - readPos, d->bufSize - dataLen);
                //
                // never read more than xxx bytes
                // This is some tuning to prevent the reading from blocking the whole thread
                //
                if( maxLen > MAX_BUFFER_READ ) // some dummy value below 1 MB
                    maxLen = MAX_BUFFER_READ;
                ret = ::read( usedInFd, &d->buffer[readPos], maxLen );
                if( ret < 0 ) {
                    if( (errno != EINTR) && (errno != EAGAIN) ) {
                        kDebug() << "(K3b::PipeBuffer::WorkThread) error while reading from " << usedInFd;
                        error = true;
                    }
                }
                else if( ret == 0 ) {
                    kDebug() << "(K3b::PipeBuffer::WorkThread) end of input.";
                    eof = true;
                }
                else {
                    dataLen += ret;

                    currentPercent = (int)((double)dataLen*100.0/(double)d->bufSize);
                }
            }

            // A little hack to keep the buffer display from flickering
            if( currentPercent == 99 )
                currentPercent = 100;

            if( currentPercent != -1 && currentPercent != oldPercent ) {
                emit percent( currentPercent );
                oldPercent = currentPercent;
            }
        }
        else if( !canceled() ) {
            error = true;
            kDebug() << "(K3b::PipeBuffer::WorkThread) select: " << ::strerror(errno);
        }
    }

    if( d->inFd == -1 ) {
        ::close( d->inFdPair[0] );
        ::close( d->inFdPair[1] );
        d->inFdPair[0] = d->inFdPair[1] = -1;
    }

    //
    // close the fd we are writing to (this is need to make growisofs happy
    // TODO: perhaps make this configurable
    //
    ::close( d->outFd );

    return !error && !canceled();
}

#include "k3bpipebuffer.moc"
