/*
 *
 * Copyright (C) 2004-2008 Sebastian Trueg <trueg@k3b.org>
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

#include "k3baudiodatasource.h"
#include "k3baudiotrack.h"
#include "k3baudiodoc.h"


K3bAudioDataSource::K3bAudioDataSource()
    : QObject(),
      m_track(0),
      m_prev(0),
      m_next(0)
{
}


K3bAudioDataSource::K3bAudioDataSource( const K3bAudioDataSource& source )
    : QObject(),
      m_track( 0 ),
      m_prev( 0 ),
      m_next( 0 ),
      m_startOffset( source.m_startOffset ),
      m_endOffset( source.m_endOffset )
{
}


K3bAudioDataSource::~K3bAudioDataSource()
{
    take();
}


K3bAudioDoc* K3bAudioDataSource::doc() const
{
    if( m_track )
        return m_track->doc();
    else
        return 0;
}


K3bAudioDataSource* K3bAudioDataSource::take()
{
    // if we do not have a track we are not in any list
    if( m_track ) {
        if( !m_prev )
            m_track->setFirstSource( m_next );

        if( m_prev )
            m_prev->m_next = m_next;
        if( m_next )
            m_next->m_prev = m_prev;

        m_prev = m_next = 0;

        emitChange();
        m_track = 0;
    }

    return this;
}


void K3bAudioDataSource::moveAfter( K3bAudioDataSource* source )
{
    // cannot create a list outside a track!
    if( !source->track() )
        return;

    if( source == this )
        return;

    // remove this from the list
    take();

    K3bAudioDataSource* oldNext = source->m_next;

    // set track as prev
    source->m_next = this;
    m_prev = source;

    // set oldNext as next
    if( oldNext )
        oldNext->m_prev = this;
    m_next = oldNext;

    m_track = source->track();
    emitChange();
}


void K3bAudioDataSource::moveAhead( K3bAudioDataSource* source )
{
    // cannot create a list outside a track!
    if( !source->track() )
        return;

    if( source == this )
        return;

    // remove this from the list
    take();

    K3bAudioDataSource* oldPrev = source->m_prev;

    // set track as next
    m_next = source;
    source->m_prev = this;

    // set oldPrev as prev
    m_prev = oldPrev;
    if( oldPrev )
        oldPrev->m_next = this;

    m_track = source->track();

    if( !m_prev )
        m_track->setFirstSource( this );

    emitChange();
}


void K3bAudioDataSource::emitChange()
{
    if( m_track )
        m_track->sourceChanged( this );
}


K3bAudioDataSource* K3bAudioDataSource::split( const K3b::Msf& pos )
{
    if( pos < length() ) {
        K3bAudioDataSource* s = copy();
        s->setStartOffset( startOffset() + pos );
        s->setEndOffset( endOffset() );
        setEndOffset( startOffset() + pos );
        s->moveAfter( this );
        emitChange();
        return s;
    }
    else
        return 0;
}


K3b::Msf K3bAudioDataSource::lastSector() const
{
    if( endOffset() > 0 )
        return endOffset()-1;
    else
        return originalLength()-1;
}


K3b::Msf K3bAudioDataSource::length() const
{
    if( originalLength() == 0 )
        return 0;
    else if( lastSector() < m_startOffset )
        return 1;
    else
        return lastSector() - m_startOffset + 1;
}


void K3bAudioDataSource::setStartOffset( const K3b::Msf& msf )
{
    m_startOffset = msf;
    fixupOffsets();
    emitChange();
}


void K3bAudioDataSource::setEndOffset( const K3b::Msf& msf )
{
    m_endOffset = msf;
    fixupOffsets();
    emitChange();
}


void K3bAudioDataSource::fixupOffsets()
{
    // no length available yet
    if( originalLength() == 0 )
        return;

    if( startOffset() >= originalLength() ) {
        setStartOffset( 0 );
    }
    if( endOffset() > originalLength() ) {
        setEndOffset( 0 ); // whole source
    }
    if( endOffset() > 0 && endOffset() <= startOffset() ) {
        setEndOffset( startOffset() );
    }
}

#include "k3baudiodatasource.moc"
