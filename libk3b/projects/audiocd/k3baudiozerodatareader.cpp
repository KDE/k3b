/*
 *
 *  Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
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

#include "k3baudiozerodatareader.h"
#include "k3baudiozerodata.h"

#include <cstring>

namespace K3b {


class AudioZeroDataReader::Private
{
public:
    Private( AudioZeroData& s )
        : source( s )
    {
    }

    AudioZeroData& source;
};


AudioZeroDataReader::AudioZeroDataReader( AudioZeroData& source, QObject* parent )
    : QIODevice( parent ),
      d( new Private( source ) )
{
}


AudioZeroDataReader::~AudioZeroDataReader()
{
    close();
}


bool AudioZeroDataReader::open( QIODevice::OpenMode mode )
{
    if( !mode.testFlag( QIODevice::WriteOnly ) ) {
        return QIODevice::open( mode );
    }
    else {
        return false;
    }
}


bool AudioZeroDataReader::isSequential() const
{
    return false;
}


qint64 AudioZeroDataReader::size() const
{
    return d->source.length().audioBytes();
}


qint64 AudioZeroDataReader::writeData( const char* /*data*/, qint64 /*len*/ )
{
    return -1;
}


qint64 AudioZeroDataReader::readData( char* data, qint64 maxlen )
{
    if( pos() + maxlen > size() )
        maxlen = size() - pos();

    ::memset( data, 0, maxlen );

    return maxlen;
}

} // namespace K3b
