/*

    SPDX-FileCopyrightText: 2004-2008 Sebastian Trueg <trueg@k3b.org>

    This file is part of the K3b project.
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3baudiodatasource.h"
#include "k3baudiotrack.h"
#include "k3baudiodoc.h"


K3b::AudioDataSource::AudioDataSource()
    : QObject(),
      m_track(0),
      m_prev(0),
      m_next(0)
{
}


K3b::AudioDataSource::AudioDataSource( const K3b::AudioDataSource& source )
    : QObject(),
      m_track( 0 ),
      m_prev( 0 ),
      m_next( 0 ),
      m_startOffset( source.m_startOffset ),
      m_endOffset( source.m_endOffset )
{
}


K3b::AudioDataSource::~AudioDataSource()
{
    take();
}


K3b::AudioDoc* K3b::AudioDataSource::doc() const
{
    if( m_track )
        return m_track->doc();
    else
        return 0;
}


K3b::AudioDataSource* K3b::AudioDataSource::take()
{
    // if we do not have a track we are not in any list
    if( m_track ) {
        m_track->emitSourceAboutToBeRemoved(this);

        // sets the first source of the track, if necessary
        if( m_prev )
            m_prev->m_next = m_next;
        if( m_next )
            m_next->m_prev = m_prev;

        // the emitSourceRemoved() function will take care of setting the
        // first source in the track (to avoid the track accessing a deleted
        // source, or a source accessing a deleted track
        m_track->emitSourceRemoved(this);

        m_prev = m_next = 0;
        m_track = 0;
    }

    return this;
}


void K3b::AudioDataSource::moveAfter( K3b::AudioDataSource* source )
{
    // cannot create a list outside a track!
    if( !source->track() )
        return;

    if( source == this )
        return;

    source->track()->emitSourceAboutToBeAdded( source->sourceIndex()+1 );

    // remove this from the list
    take();

    K3b::AudioDataSource* oldNext = source->m_next;

    // set track as prev
    source->m_next = this;
    m_prev = source;

    // set oldNext as next
    if( oldNext )
        oldNext->m_prev = this;
    m_next = oldNext;

    m_track = source->track();

    m_track->emitSourceAdded( this );
}


void K3b::AudioDataSource::moveAhead( K3b::AudioDataSource* source )
{
    // cannot create a list outside a track!
    if( !source->track() )
        return;

    if( source == this )
        return;

    source->track()->emitSourceAboutToBeAdded( source->sourceIndex() );

    // remove this from the list
    take();

    K3b::AudioDataSource* oldPrev = source->m_prev;

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

    m_track->emitSourceAdded( this );
}


void K3b::AudioDataSource::emitChange()
{
    emit changed();
    if( m_track )
        m_track->sourceChanged( this );
}


K3b::AudioDataSource* K3b::AudioDataSource::split( const K3b::Msf& pos )
{
    if( pos < length() ) {
        K3b::AudioDataSource* s = copy();
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


K3b::Msf K3b::AudioDataSource::lastSector() const
{
    if( endOffset() > 0 )
        return endOffset()-1;
    else
        return originalLength()-1;
}


K3b::Msf K3b::AudioDataSource::length() const
{
    if( originalLength() == 0 )
        return 0;
    else if( lastSector() < m_startOffset )
        return 1;
    else
        return lastSector() - m_startOffset + 1;
}


int K3b::AudioDataSource::sourceIndex() const
{
    if (!m_prev)
        return 0;

    return m_prev->sourceIndex() + 1;
}


void K3b::AudioDataSource::setStartOffset( const K3b::Msf& msf )
{
    m_startOffset = msf;
    fixupOffsets();
    emitChange();
}


void K3b::AudioDataSource::setEndOffset( const K3b::Msf& msf )
{
    m_endOffset = msf;
    fixupOffsets();
    emitChange();
}


void K3b::AudioDataSource::fixupOffsets()
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


