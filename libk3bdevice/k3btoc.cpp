/*

    SPDX-FileCopyrightText: 2003-2009 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2009 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/


#include "k3btoc.h"
#include <QDebug>
#include <QString>



K3b::Device::Toc::Toc()
    : QList<K3b::Device::Track>()
{
}


K3b::Device::Toc::Toc( const Toc& toc )
    : QList<K3b::Device::Track>( toc )
{
    m_mcn = toc.m_mcn;
}


K3b::Device::Toc::~Toc()
{
}


K3b::Device::Toc& K3b::Device::Toc::operator=( const Toc& toc )
{
    if( &toc == this ) return *this;

    m_mcn = toc.m_mcn;

    QList<K3b::Device::Track>::operator=( toc );

    return *this;
}


K3b::Msf K3b::Device::Toc::firstSector() const
{
    return isEmpty() ? K3b::Msf() : first().firstSector();
}


K3b::Msf K3b::Device::Toc::lastSector() const
{
    if( isEmpty() )
        return 0;
    // the last track's last sector should be the last sector of the entire cd
    return last().lastSector();
}


K3b::Msf K3b::Device::Toc::length() const
{
    // +1 since the last sector is included
    return lastSector() - firstSector() + 1;
}


unsigned int K3b::Device::Toc::discId() const
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


K3b::Device::ContentsType K3b::Device::Toc::contentType() const
{
    int audioCnt = 0, dataCnt = 0;
    for( Toc::const_iterator it = constBegin(); it != constEnd(); ++it ) {
        if( (*it).type() == K3b::Device::Track::TYPE_AUDIO )
            audioCnt++;
        else
            dataCnt++;
    }

    if( audioCnt + dataCnt == 0 )
        return K3b::Device::NONE;
    if( audioCnt == 0 )
        return K3b::Device::DATA;
    if( dataCnt == 0 )
        return K3b::Device::AUDIO;
    return K3b::Device::MIXED;
}


int K3b::Device::Toc::sessions() const
{
    if( isEmpty() )
        return 0;
    else if( last().session() == 0 )
        return 1; // default if unknown
    else
        return last().session();
}


QByteArray K3b::Device::Toc::mcn() const
{
    return m_mcn;
}


void K3b::Device::Toc::setMcn( const QByteArray& mcn )
{
    m_mcn = mcn;
}


void K3b::Device::Toc::clear()
{
    QList<Track>::clear();
    m_mcn.resize( 0 );
}


bool K3b::Device::Toc::operator==( const Toc& other ) const
{
    return( QList<Track>::operator==( other ) );
}


bool K3b::Device::Toc::operator!=( const Toc& other ) const
{
    return( QList<Track>::operator!=( other ) );
}


QDebug operator<<( QDebug s, const K3b::Device::Toc& toc )
{
    s.nospace() << toc.count() << " in " << toc.sessions() << " sessions";
    int sessionN = 0;
    int trackN = 0;
    for( K3b::Device::Toc::const_iterator it = toc.constBegin(); it != toc.constEnd(); ++it ) {
        ++trackN;
        if( sessionN != it->session() ) {
            sessionN = it->session();
            s.nospace() << "Session Number " << sessionN;
        }
        s.nospace() << "  Track " << trackN << *it;
    }
    return s;
}
