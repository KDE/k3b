/*
    SPDX-FileCopyrightText: 1998-2007 Sebastian Trueg <trueg@k3b.org>
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3baudiodatasourceiterator.h"
#include "k3baudiodoc.h"
#include "k3baudiotrack.h"
#include "k3baudiodatasource.h"


K3b::AudioDataSourceIterator::AudioDataSourceIterator( K3b::AudioDoc* doc )
    : m_doc( doc )
{
    first();
}


K3b::AudioDataSource* K3b::AudioDataSourceIterator::current() const
{
    return m_currentSource;
}


K3b::AudioDataSource* K3b::AudioDataSourceIterator::next()
{
    m_currentSource = m_currentSource->next();
    if( !m_currentSource ) {
        m_currentTrack = m_currentTrack->next();
        if( m_currentTrack )
            m_currentSource = m_currentTrack->firstSource();
    }

    return m_currentSource;
}


bool K3b::AudioDataSourceIterator::hasNext() const
{
    if( !m_currentSource )
        return false;
    if( m_currentSource->next() )
        return true;
    if( m_currentTrack->next() )
        return true;

    return false;
}


K3b::AudioDataSource* K3b::AudioDataSourceIterator::first()
{
    m_currentTrack = m_doc->firstTrack();

    if( m_currentTrack )
        m_currentSource = m_currentTrack->firstSource();
    else
        m_currentSource = 0;

    return m_currentSource;
}
