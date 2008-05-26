/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
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


#include "k3btoc.h"
#include "kdebug.h"

#include <qstring.h>


K3bDevice::Toc::Toc()
    : QList<K3bDevice::Track>()
{
}


K3bDevice::Toc::Toc( const Toc& toc )
    : QList<K3bDevice::Track>( toc )
{
    m_mcn = toc.m_mcn;
}


K3bDevice::Toc::~Toc()
{
}


K3bDevice::Toc& K3bDevice::Toc::operator=( const Toc& toc )
{
    if( &toc == this ) return *this;

    m_mcn = toc.m_mcn;

    QList<K3bDevice::Track>::operator=( toc );

    return *this;
}


K3b::Msf K3bDevice::Toc::firstSector() const
{
    return isEmpty() ? K3b::Msf() : first().firstSector();
}


K3b::Msf K3bDevice::Toc::lastSector() const
{
    if( isEmpty() )
        return 0;
    // the last track's last sector should be the last sector of the entire cd
    return last().lastSector();
}


K3b::Msf K3bDevice::Toc::length() const
{
    // +1 since the last sector is included
    return lastSector() - firstSector() + 1;
}


unsigned int K3bDevice::Toc::discId() const
{
    // calculate cddb-id
    unsigned int id = 0;
    for( Toc::const_iterator it = constBegin(); it != constEnd(); ++it ) {
        unsigned int n = (*it).firstSector().lba() + 150;
        n /= 75;
        while( n > 0 ) {
            id += n % 10;
            n /= 10;
        }
    }
    unsigned int l = length().lba();
    if ( !empty() )
        l -= first().firstSector().lba();
    l /= 75;
    id = ( ( id % 0xff ) << 24 ) | ( l << 8 ) | count();

    return id;
}


int K3bDevice::Toc::contentType() const
{
    int audioCnt = 0, dataCnt = 0;
    for( Toc::const_iterator it = constBegin(); it != constEnd(); ++it ) {
        if( (*it).type() == K3bDevice::Track::AUDIO )
            audioCnt++;
        else
            dataCnt++;
    }

    if( audioCnt + dataCnt == 0 )
        return K3bDevice::NONE;
    if( audioCnt == 0 )
        return K3bDevice::DATA;
    if( dataCnt == 0 )
        return K3bDevice::AUDIO;
    return K3bDevice::MIXED;
}


int K3bDevice::Toc::sessions() const
{
    if( isEmpty() )
        return 0;
    else if( last().session() == 0 )
        return 1; // default if unknown
    else
        return last().session();
}


QByteArray K3bDevice::Toc::mcn() const
{
    return m_mcn;
}


void K3bDevice::Toc::setMcn( const QByteArray& mcn )
{
    m_mcn = mcn;
}


void K3bDevice::Toc::clear()
{
    QList<Track>::clear();
    m_mcn.resize( 0 );
}


void K3bDevice::Toc::debug() const
{
    kDebug() << count() << " in " << sessions() << " sessions";
    int sessionN = 0;
    int trackN = 0;
    for( Toc::const_iterator it = begin(); it != end(); ++it ) {
        ++trackN;
        if( sessionN != (*it).session() ) {
            sessionN = (*it).session();
            kDebug() << "Session Number " << sessionN;
        }
        kDebug() << "  Track " << trackN << ( (*it).type() == Track::AUDIO ? " AUDIO" : " DATA" )
                 << " " << (*it).firstSector().lba() << " - " << (*it).lastSector().lba()
                 << " (" << (*it).length().lba() << ")" << endl;
    }
}


bool K3bDevice::Toc::operator==( const Toc& other ) const
{
    return( QList<Track>::operator==( other ) );
}


bool K3bDevice::Toc::operator!=( const Toc& other ) const
{
    return( QList<Track>::operator!=( other ) );
}
