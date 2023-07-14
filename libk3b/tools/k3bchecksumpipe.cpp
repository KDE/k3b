/*
    SPDX-FileCopyrightText: 2006-2009 Sebastian Trueg <trueg@k3b.org>
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3bchecksumpipe.h"

#include <QDebug>
#include <QCryptographicHash>

#include <unistd.h>


class K3b::ChecksumPipe::Private
{
public:
    Private()
		: checksumType(MD5), md5(QCryptographicHash::Md5) {
    }

    void update( const char* in, qint64 len ) {
        switch( checksumType ) {
        case MD5:
			md5.addData( in, len );
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

	QCryptographicHash md5;
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
		return d->md5.result().toHex();
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

#include "moc_k3bchecksumpipe.cpp"
