/*
 *
 * Copyright (C) 2006-2009 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bchecksumpipe.h"

#include <kcodecs.h>
#include <kdebug.h>

#include <unistd.h>


class K3b::ChecksumPipe::Private
{
public:
    Private()
        : checksumType(MD5) {
    }

    void update( const char* in, qint64 len ) {
        switch( checksumType ) {
        case MD5:
            md5.update( in, len );
            break;
        }
    }

    void reset() {
        switch( checksumType ) {
        case MD5:
            md5.reset();
            break;
        }
    }

    int checksumType;

    KMD5 md5;
};


K3b::ChecksumPipe::ChecksumPipe()
    : K3b::ActivePipe()
{
    d = new Private();
}


K3b::ChecksumPipe::~ChecksumPipe()
{
    delete d;
}


bool K3b::ChecksumPipe::open( bool closeWhenDone )
{
    return open( MD5, closeWhenDone );
}


bool K3b::ChecksumPipe::open( Type type, bool closeWhenDone )
{
    d->reset();
    d->checksumType = type;
    return K3b::ActivePipe::open( closeWhenDone );
}


QByteArray K3b::ChecksumPipe::checksum() const
{
    switch( d->checksumType ) {
    case MD5:
        return d->md5.hexDigest();
    }

    return QByteArray();
}


qint64 K3b::ChecksumPipe::writeData( const char* data, qint64 max )
{
    d->update( data, max );
    return K3b::ActivePipe::writeData( data, max );
}


bool K3b::ChecksumPipe::open( OpenMode mode )
{
    return ActivePipe::open( mode );
}
