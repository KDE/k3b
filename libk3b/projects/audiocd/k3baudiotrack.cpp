/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 *           (C) 2009      Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
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


#include "k3baudiotrack.h"
#include "k3baudiodoc.h"
#include "k3baudiodatasource.h"

#include <k3baudiodecoder.h>
#include <k3bcore.h>
#include <k3bcdtextvalidator.h>

#include <qstring.h>

#include <kdebug.h>



class K3b::AudioTrack::Private
{
public:
    Private() {
        cdTextValidator = new K3b::CdTextValidator();
    }

    ~Private() {
        delete cdTextValidator;
    }

    K3b::CdTextValidator* cdTextValidator;
};


K3b::AudioTrack::AudioTrack()
    : QObject(),
      m_parent(0),
      m_copy(false),
      m_preEmp(false),
      m_index0Offset(150),
      m_prev(0),
      m_next(0),
      m_firstSource(0),
      m_currentSource(0),
      m_alreadyReadBytes(0),
      m_currentlyDeleting(false)
{
    d = new Private;
}


K3b::AudioTrack::AudioTrack( K3b::AudioDoc* parent )
    : QObject(),
      m_parent(parent),
      m_copy(false),
      m_preEmp(false),
      m_index0Offset(150),
      m_prev(0),
      m_next(0),
      m_firstSource(0),
      m_currentSource(0),
      m_alreadyReadBytes(0),
      m_currentlyDeleting(false)
{
    d = new Private;
}


K3b::AudioTrack::~AudioTrack()
{
    kDebug() << this;
    //
    // It is crucial that we do not emit the changed signal here because otherwise
    // the doc will delete us again once we are empty!
    //
    m_currentlyDeleting = true;

    // fix the list
    take();

    kDebug() << "deleting sources.";

    // delete all sources
    while( m_firstSource )
        delete m_firstSource->take();

    kDebug() << "finished";

    delete d;
}


void K3b::AudioTrack::emitChanged()
{
    if( m_parent )
        m_parent->slotTrackChanged( this );
}


void K3b::AudioTrack::setArtist( const QString& a )
{
    setPerformer( a );
}


void K3b::AudioTrack::setPerformer( const QString& a )
{
    QString s( a );
    d->cdTextValidator->fixup( s );
    m_cdText.setPerformer(s);
    emitChanged();
}


void K3b::AudioTrack::setTitle( const QString& t )
{
    QString s( t );
    d->cdTextValidator->fixup( s );
    m_cdText.setTitle(s);
    emitChanged();
}


void K3b::AudioTrack::setArranger( const QString& t )
{
    QString s( t );
    d->cdTextValidator->fixup( s );
    m_cdText.setArranger(s);
    emitChanged();
}


void K3b::AudioTrack::setSongwriter( const QString& t )
{
    QString s( t );
    d->cdTextValidator->fixup( s );
    m_cdText.setSongwriter(s);
    emitChanged();
}


void K3b::AudioTrack::setComposer( const QString& t )
{
    QString s( t );
    d->cdTextValidator->fixup( s );
    m_cdText.setComposer(s);
    emitChanged();
}


void K3b::AudioTrack::setIsrc( const QString& t )
{
    m_cdText.setIsrc(t);
    emitChanged();
}


void K3b::AudioTrack::setCdTextMessage( const QString& t )
{
    QString s( t );
    d->cdTextValidator->fixup( s );
    m_cdText.setMessage(s);
    emitChanged();
}


void K3b::AudioTrack::setCdText( const K3b::Device::TrackCdText& cdtext )
{
    m_cdText = cdtext;
    emitChanged();
}


K3b::AudioDataSource* K3b::AudioTrack::lastSource() const
{
    K3b::AudioDataSource* s = m_firstSource;
    while( s && s->next() )
        s = s->next();
    return s;
}


bool K3b::AudioTrack::inList() const
{
    if( doc() )
        return ( doc()->firstTrack() == this || m_prev != 0 );
    else
        return false;
}


K3b::Msf K3b::AudioTrack::length() const
{
    K3b::Msf length;
    K3b::AudioDataSource* source = m_firstSource;
    while( source ) {
        length += source->length();
        source = source->next();
    }
    return length;
}


KIO::filesize_t K3b::AudioTrack::size() const
{
    return length().audioBytes();
}


unsigned int K3b::AudioTrack::trackNumber() const
{
    if( m_prev )
        return m_prev->trackNumber() + 1;
    else
        return 1;
}


K3b::Msf K3b::AudioTrack::index0() const
{
    // we save the index0Offset as length of the resulting pregap
    // this way the length of the track does not need to be ready
    // when creating the track.
    return length() - m_index0Offset;
}


K3b::Msf K3b::AudioTrack::postGap() const
{
    if( next() )
        return m_index0Offset;
    else
        return 0;
}


void K3b::AudioTrack::setIndex0( const K3b::Msf& msf )
{
    if( msf == 0 )
        m_index0Offset = 0;
    else
        m_index0Offset = length() - msf;
}


K3b::AudioTrack* K3b::AudioTrack::take()
{
    if( inList() ) {
        if ( m_parent )
            m_parent->slotAboutToRemoveTrack(this);

        if( !m_prev )
            doc()->setFirstTrack( m_next );
        if( !m_next )
            doc()->setLastTrack( m_prev );

        if( m_prev )
            m_prev->m_next = m_next;
        if( m_next )
            m_next->m_prev = m_prev;

        m_prev = m_next = 0;

        // remove from doc
        if( m_parent )
            m_parent->slotTrackRemoved(this);
        m_parent = 0;
    }

    return this;
}


void K3b::AudioTrack::moveAfter( K3b::AudioTrack* track )
{
    kDebug() << "(K3b::AudioTrack::moveAfter( " << track << " )";
    if( !track ) {
        if( !doc() ) {
            kDebug() << "(K3b::AudioTrack::moveAfter) no parent set";
            return;
        }

        // make sure we do not mess up the list
        if( doc()->lastTrack() )
            moveAfter( doc()->lastTrack() );
        else {
            doc()->setFirstTrack( take() );
            doc()->setLastTrack( this );
        }
    }
    else if( track == this ) {
        kDebug() << "(K3b::AudioTrack::moveAfter) trying to move this after this.";
        return;
    }
    else {
        // remove this from the list
        take();

        // set the new parent doc
        m_parent = track->doc();

        K3b::AudioTrack* oldNext = track->m_next;

        // set track as prev
        track->m_next = this;
        m_prev = track;

        // set oldNext as next
        if( oldNext )
            oldNext->m_prev = this;
        m_next = oldNext;

        if( !m_prev )
            doc()->setFirstTrack( this );
        if( !m_next )
            doc()->setLastTrack( this );
    }

    emitChanged();
}


void K3b::AudioTrack::moveAhead( K3b::AudioTrack* track )
{
    if( !track ) {
        if( !doc() ) {
            kDebug() << "(K3b::AudioTrack::moveAfter) no parent set";
            return;
        }

        // make sure we do not mess up the list
        if( doc()->firstTrack() )
            moveAhead( doc()->firstTrack() );
        else {
            doc()->setFirstTrack( take() );
            doc()->setLastTrack( this );
        }
    }
    else if( track == this ) {
        kDebug() << "(K3b::AudioTrack::moveAhead) trying to move this ahead of this.";
        return;
    }
    else {
        // remove this from the list
        take();

        // set the new parent doc
        m_parent = track->doc();

        K3b::AudioTrack* oldPrev = track->m_prev;

        // set track as next
        m_next = track;
        track->m_prev = this;

        // set oldPrev as prev
        m_prev = oldPrev;
        if( oldPrev )
            oldPrev->m_next = this;

        if( !m_prev )
            doc()->setFirstTrack( this );
        if( !m_next )
            doc()->setLastTrack( this );
    }

    emitChanged();
}


void K3b::AudioTrack::merge( K3b::AudioTrack* trackToMerge, K3b::AudioDataSource* sourceAfter )
{
    kDebug() << "(K3b::AudioTrack::merge) " << trackToMerge << " into " << this;
    if( this == trackToMerge ) {
        kDebug() << "(K3b::AudioTrack::merge) trying to merge this with this.";
        return;
    }

    // remove the track to merge to make sure it does not get deleted by the doc too early
    trackToMerge->take();

    // in case we prepend all of trackToMerge's sources
    if( !sourceAfter ) {
        kDebug() << "(K3b::AudioTrack::merge) merging " << trackToMerge->firstSource();
        if( m_firstSource ) {
            trackToMerge->firstSource()->moveAhead( m_firstSource );
        }
        else {
            addSource( trackToMerge->firstSource()->take() );
        }
        sourceAfter = m_firstSource;
    }

    kDebug() << "(K3b::AudioTrack::merge) now merge the other sources.";
    // now merge all sources into this track
    while( trackToMerge->firstSource() ) {
        K3b::AudioDataSource* s = trackToMerge->firstSource();
        kDebug() << "(K3b::AudioTrack::merge) merging source " << s << " from track " << s->track() << " into track "
                 << this << " after source " << sourceAfter << endl;
        s->moveAfter( sourceAfter );
        sourceAfter = s;
    }

    // TODO: should we also merge the indices?

    // now we can safely delete the track we merged
    delete trackToMerge;

    kDebug() << "(K3b::AudioTrack::merge) finished";

    emitChanged();
}


void K3b::AudioTrack::setFirstSource( K3b::AudioDataSource* source )
{
    // reset the reading stuff since this might be a completely new source list
    m_currentSource = 0;
    m_alreadyReadBytes = 0;

    m_firstSource = source;
    while( source ) {
        source->m_track = this;
        source = source->next();
    }

    emitChanged();
}


void K3b::AudioTrack::addSource( K3b::AudioDataSource* source )
{
    if( !source )
        return;

    K3b::AudioDataSource* s = m_firstSource;
    while( s && s->next() )
        s = s->next();
    if( s )
        source->moveAfter( s );
    else
        setFirstSource( source->take() );
}


void K3b::AudioTrack::sourceChanged( K3b::AudioDataSource* )
{
    if( m_currentlyDeleting )
        return;

    // TODO: update indices

    if( m_index0Offset > length() )
        m_index0Offset = length()-1;

    emitChanged();
}


int K3b::AudioTrack::numberSources() const
{
    K3b::AudioDataSource* source = m_firstSource;
    int i = 0;
    while( source ) {
        source = source->next();
        ++i;
    }
    return i;
}


bool K3b::AudioTrack::seek( const K3b::Msf& msf )
{
    K3b::AudioDataSource* source = m_firstSource;

    K3b::Msf pos;
    while( source && pos + source->length() < msf ) {
        pos += source->length();
        source = source->next();
    }

    if( source ) {
        m_currentSource = source;
        m_alreadyReadBytes = msf.audioBytes();
        return source->seek( msf - pos );
    }
    else
        return false;
}


int K3b::AudioTrack::read( char* data, unsigned int max )
{
    if( !m_currentSource ) {
        m_currentSource = m_firstSource;
        if( m_currentSource )
            m_currentSource->seek(0);
        m_alreadyReadBytes = 0;
    }

    int readData = m_currentSource->read( data, max );
    if( readData == 0 ) {
        m_currentSource = m_currentSource->next();
        if( m_currentSource ) {
            m_currentSource->seek(0);
            return read( data, max ); // read from next source
        }
    }

    m_alreadyReadBytes += readData;

    return readData;
}


K3b::AudioTrack* K3b::AudioTrack::copy() const
{
    K3b::AudioTrack* track = new K3b::AudioTrack();

    track->m_copy = m_copy;
    track->m_preEmp = m_preEmp;
    track->m_index0Offset = m_index0Offset;
    track->m_cdText = m_cdText;
    K3b::AudioDataSource* source = m_firstSource;
    while( source ) {
        track->addSource( source->copy() );
        source = source->next();
    }

    return track;
}


K3b::AudioTrack* K3b::AudioTrack::split( const K3b::Msf& pos )
{
    if( pos < length() ) {
        // search the source
        // pos will be the first sector of the new track
        K3b::Msf currentPos;
        K3b::AudioDataSource* source = firstSource();
        while( source && currentPos + source->length() <= pos ) {
            currentPos += source->length();
            source = source->next();
        }

        K3b::AudioDataSource* splitSource = 0;
        if( currentPos > 0 && currentPos == pos ) {
            // no need to split a source
            splitSource = source;
        }
        else {
            splitSource = source->split( pos - currentPos );
        }

        // the new track should include all sources from splitSource and below
        K3b::AudioTrack* splitTrack = new K3b::AudioTrack();
        splitTrack->m_cdText = m_cdText;
        source = splitSource;
        while( source ) {
            K3b::AudioDataSource* addSource = source;
            source = source->next();
            splitTrack->addSource( addSource );
        }

        kDebug() << "(K3b::AudioTrack) moving track " << splitTrack << " after this (" << this << ") with parent " << doc();
        splitTrack->moveAfter( this );

        return splitTrack;
    }
    else
        return 0;
}


K3b::Device::Track K3b::AudioTrack::toCdTrack() const
{
    if( !inList() )
        return K3b::Device::Track();

    K3b::Msf firstSector;
    K3b::AudioTrack* track = doc()->firstTrack();
    while( track != this ) {
        firstSector += track->length();
        track = track->next();
    }

    K3b::Device::Track cdTrack( firstSector,
                              firstSector + length() - 1,
                              K3b::Device::Track::TYPE_AUDIO );

    // FIXME: auch im audiotrack copy permitted
    cdTrack.setCopyPermitted( !copyProtection() );
    cdTrack.setPreEmphasis( preEmp() );

    // FIXME: add indices != 0

    // no index 0 for the last track. Or should we allow this???
    if( doc()->lastTrack() != this )
        cdTrack.setIndex0( index0() );

    // FIXME: convert to QCString
    //    cdTrack.setIsrc( isrc() );

    return cdTrack;
}


void K3b::AudioTrack::debug()
{
    kDebug() << "Track " << this << endl
             << "  Prev: " << m_prev << endl
             << "  Next: " << m_next << endl
             << "  Sources:" << endl;
    K3b::AudioDataSource* s = m_firstSource;
    while( s ) {
        kDebug() << "  " << s << " - Prev: " << s->prev() << " Next: " << s->next();
        s = s->next();
    }
}


K3b::AudioDataSource* K3b::AudioTrack::getSource( int index ) const
{
    int i = 0;
    K3b::AudioDataSource* source = firstSource();
    while ( source && i < index ) {
        source = source->next();
        ++i;
    }
    return source;
}

#include "k3baudiotrack.moc"
