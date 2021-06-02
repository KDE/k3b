/*

    SPDX-FileCopyrightText: 2004 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
    See the file "COPYING" for the exact licensing terms.
*/

#include "k3baudiozerodata.h"
#include "k3baudiozerodatareader.h"
#include "k3baudiotrack.h"
#include "k3b_i18n.h"


K3b::AudioZeroData::AudioZeroData( const K3b::Msf& len )
    : K3b::AudioDataSource(),
      m_length(len)
{
}


K3b::AudioZeroData::AudioZeroData( const K3b::AudioZeroData& zero )
    : K3b::AudioDataSource( zero ),
      m_length( zero.m_length )
{
}


K3b::AudioZeroData::~AudioZeroData()
{
}


void K3b::AudioZeroData::setLength( const K3b::Msf& msf )
{
    if( msf > 0 )
        m_length = msf;
    else
        m_length = 1; // 1 frame

    emitChange();
}


QString K3b::AudioZeroData::type() const
{
    return i18n("Silence");
}


QString K3b::AudioZeroData::sourceComment() const
{
    return QString();
}


K3b::AudioDataSource* K3b::AudioZeroData::copy() const
{
    return new K3b::AudioZeroData( *this );
}


QIODevice* K3b::AudioZeroData::createReader( QObject* parent )
{
    return new AudioZeroDataReader( *this, parent );
}


void K3b::AudioZeroData::setStartOffset( const K3b::Msf& pos )
{
    if( pos >= length() )
        setLength( 1 );
    else
        setLength( length() - pos );
}


void K3b::AudioZeroData::setEndOffset( const K3b::Msf& pos )
{
    if( pos < 1 )
        setLength( 1 );
    else
        setLength( pos );
}
