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


#include "k3btrack.h"

#include <QSharedData>

class K3b::Device::Track::Private : public QSharedData
{
public:
    Private( const K3b::Msf& fs = K3b::Msf(),
             const K3b::Msf& ls = K3b::Msf(),
             TrackType t = TYPE_UNKNOWN,
             DataMode m = UNKNOWN )
        : firstSector( fs ),
          lastSector( ls ),
          type( t ),
          mode( m ),
          copyPermitted(true),
          preEmphasis(false),
          session(0) {
    }

    K3b::Msf firstSector;
    K3b::Msf lastSector;
    K3b::Msf index0;

    K3b::Msf nextWritableAddress;
    K3b::Msf freeBlocks;

    TrackType type;
    DataMode mode;
    bool copyPermitted;
    bool preEmphasis;

    int session;

    QList<K3b::Msf> indices;

    QByteArray isrc;
};


K3b::Device::Track::Track()
    : d( new Private() )
{
}


K3b::Device::Track::Track( const Track& track )
{
    d = track.d;
}


K3b::Device::Track::Track( const K3b::Msf& firstSector,
                           const K3b::Msf& lastSector,
                           TrackType type,
                           DataMode mode )
    : d( new Private( firstSector,
                      lastSector,
                      type,
                      mode ) )
{
}


K3b::Device::Track::~Track()
{
}


K3b::Device::Track& K3b::Device::Track::operator=( const Track& track )
{
    d = track.d;
    return *this;
}


K3b::Msf K3b::Device::Track::length() const
{
    // +1 since the last sector is included
    return d->lastSector - d->firstSector + 1;
}


K3b::Device::Track::TrackType K3b::Device::Track::type() const
{
    return d->type;
}


void K3b::Device::Track::setType( TrackType t )
{
    d->type = t;
}


K3b::Device::Track::DataMode K3b::Device::Track::mode() const
{
    return d->mode;
}


void K3b::Device::Track::setMode( DataMode m )
{
    d->mode = m;
}


bool K3b::Device::Track::copyPermitted() const
{
    return d->copyPermitted;
}


void K3b::Device::Track::setCopyPermitted( bool b )
{
    d->copyPermitted = b;
}


bool K3b::Device::Track::preEmphasis() const
{
    return d->preEmphasis;
}


void K3b::Device::Track::setPreEmphasis( bool b )
{
    d->preEmphasis = b;
}


bool K3b::Device::Track::recordedIncremental() const
{
    return d->preEmphasis;
}


bool K3b::Device::Track::recordedUninterrupted() const
{
    return !recordedIncremental();
}


QByteArray K3b::Device::Track::isrc() const
{
    return d->isrc;
}


void K3b::Device::Track::setIsrc( const QByteArray& s )
{
    d->isrc = s;
}


K3b::Msf K3b::Device::Track::firstSector() const
{
    return d->firstSector;
}


K3b::Msf K3b::Device::Track::lastSector() const
{
    return d->lastSector;
}


void K3b::Device::Track::setFirstSector( const K3b::Msf& msf )
{
    d->firstSector = msf;
}


void K3b::Device::Track::setLastSector( const K3b::Msf& msf )
{
    d->lastSector = msf;
}


K3b::Msf K3b::Device::Track::nextWritableAddress() const
{
    return d->nextWritableAddress;
}


void K3b::Device::Track::setNextWritableAddress( const K3b::Msf& m )
{
    d->nextWritableAddress = m;
}


void K3b::Device::Track::setFreeBlocks( const K3b::Msf& m )
{
    d->freeBlocks = m;
}


K3b::Msf K3b::Device::Track::freeBlocks() const
{
    return d->freeBlocks;
}


K3b::Msf K3b::Device::Track::realAudioLength() const
{
    if( index0() > 0 )
        return index0();
    else
        return length();
}


int K3b::Device::Track::session() const
{
    return d->session;
}


void K3b::Device::Track::setSession( int s )
{
    d->session = s;
}


K3b::Msf K3b::Device::Track::index0() const
{
    return d->index0;
}


QList<K3b::Msf> K3b::Device::Track::indices() const
{
    return d->indices;
}


void K3b::Device::Track::setIndices( const QList<K3b::Msf>& il )
{
    d->indices = il;
}


void K3b::Device::Track::setIndex0( const K3b::Msf& msf )
{
    if( msf <= d->lastSector-d->firstSector )
        d->index0 = msf;
}


int K3b::Device::Track::indexCount() const
{
    return d->indices.count()-1;
}


bool K3b::Device::Track::operator==( const Track& other ) const
{
    return( d->firstSector == other.d->firstSector &&
            d->lastSector == other.d->lastSector &&
            d->index0 == other.d->index0 &&
            d->nextWritableAddress == other.d->nextWritableAddress &&
            d->freeBlocks == other.d->freeBlocks &&
            d->type == other.d->type &&
            d->mode == other.d->mode &&
            d->copyPermitted == other.d->copyPermitted &&
            d->preEmphasis == other.d->preEmphasis &&
            d->session == other.d->session &&
            d->indices == other.d->indices &&
            d->isrc == other.d->isrc );
}


bool K3b::Device::Track::operator!=( const Track& other ) const
{
    return !operator==( other );
}


QDebug operator<<( QDebug s, const K3b::Device::Track& track )
{
    s.nospace() << ( track.type() == K3b::Device::Track::TYPE_AUDIO ? " AUDIO" : " DATA" )
                << " " << track.firstSector().lba() << " - " << track.lastSector().lba()
                << " (" << track.length().lba() << ")";
    return s;
}


uint qHash( const K3b::Device::Track& key )
{
    // this is a dummy implementation to make it compile on windows
	return qHash((long)&key);
}
