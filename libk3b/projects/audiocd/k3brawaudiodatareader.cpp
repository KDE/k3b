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

#include "k3brawaudiodatareader.h"
#include "k3brawaudiodatasource.h"

#include <QFile>

namespace K3b {

class RawAudioDataReader::Private
{
public:
    Private( RawAudioDataSource& s )
    :
        source( s )
    {
    }

    RawAudioDataSource& source;
    QFile imageFile;
};


RawAudioDataReader::RawAudioDataReader( RawAudioDataSource& source, QObject* parent )
    : QIODevice( parent ),
      d( new Private( source ) )
{
    d->imageFile.setFileName( source.path() );
}


RawAudioDataReader::~RawAudioDataReader()
{
}


bool RawAudioDataReader::open( OpenMode mode )
{
    return d->imageFile.open( mode ) && QIODevice::open( mode );
}


void RawAudioDataReader::close()
{
    d->imageFile.close();
    QIODevice::close();
}


bool RawAudioDataReader::isSequential() const
{
    return false;
}


qint64 RawAudioDataReader::pos() const
{
    return d->imageFile.pos();
}


qint64 RawAudioDataReader::size() const
{
    return d->imageFile.size();
}


bool RawAudioDataReader::seek( qint64 pos )
{
    return d->imageFile.seek( pos );
}


qint64 RawAudioDataReader::writeData( const char* data, qint64 len )
{
    return d->imageFile.write( data, len );
}


qint64 RawAudioDataReader::readData( char* data, qint64 maxlen )
{
    return d->imageFile.read( data, maxlen );
}

} // namespace K3b
