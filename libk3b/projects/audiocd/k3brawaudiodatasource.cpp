/*
    SPDX-FileCopyrightText: 2009 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3brawaudiodatasource.h"
#include "k3brawaudiodatareader.h"
#include "k3b_i18n.h"

#include <QFileInfo>


class K3b::RawAudioDataSource::Private
{
public:
    QString path;
};


K3b::RawAudioDataSource::RawAudioDataSource()
    : AudioDataSource(),
      d( new Private() )
{
}


K3b::RawAudioDataSource::RawAudioDataSource( const QString& path )
    : AudioDataSource(),
      d( new Private() )
{
    d->path = path;
}


K3b::RawAudioDataSource::RawAudioDataSource( const RawAudioDataSource& other )
    : AudioDataSource( other ),
      d( new Private() )
{
    d->path = other.d->path;
}


K3b::RawAudioDataSource::~RawAudioDataSource()
{
    delete d;
}


QString K3b::RawAudioDataSource::path() const
{
    return d->path;
}


K3b::Msf K3b::RawAudioDataSource::originalLength() const
{
    return Msf::fromAudioBytes( QFileInfo( d->path ).size() );
}


QString K3b::RawAudioDataSource::type() const
{
    return i18n( "Raw Audio CD Image" );
}


QString K3b::RawAudioDataSource::sourceComment() const
{
    return type();
}


K3b::AudioDataSource* K3b::RawAudioDataSource::copy() const
{
    return new RawAudioDataSource( *this );
}


QIODevice* K3b::RawAudioDataSource::createReader( QObject* parent )
{
    return new RawAudioDataReader( *this, parent );
}
