/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009      Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
 * Copyright (C) 2010      Michal Malek <michalm@jabster.pl>
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
#include "k3baudiotrackreader.h"

#include "k3baudiodecoder.h"
#include "k3bcore.h"
#include "k3bcdtextvalidator.h"

#include <QString>

#include <QtCore/QDebug>



class K3b::AudioTrack::Private
{
public:
    Private( AudioDoc* p = 0 )
    :
      parent(p),
      copy(false),
      preEmp(false),
      index0Offset(150),
      prev(0),
      next(0),
      firstSource(0),
      currentlyDeleting(false) {
        cdTextValidator = new K3b::CdTextValidator();
    }

    ~Private() {
        delete cdTextValidator;
    }

    AudioDoc* parent;

    /** copy protection */
    bool copy;
    bool preEmp;

    Msf index0Offset;

    Device::TrackCdText cdText;

    // list
    AudioTrack* prev;
    AudioTrack* next;

    AudioDataSource* firstSource;

    bool currentlyDeleting;

    K3b::CdTextValidator* cdTextValidator;
};


K3b::AudioTrack::AudioTrack()
    : QObject(),
      d( new Private )
{
}


K3b::AudioTrack::AudioTrack( K3b::AudioDoc* parent )
    : QObject(),
      d( new Private( parent ) )
{
}


K3b::AudioTrack::~AudioTrack()
{
    qDebug() << this;

    d->currentlyDeleting = true;

    // fix the list
    take();

    qDebug() << "deleting sources.";

    // delete all sources
    while( d->firstSource )
        delete d->firstSource;

    qDebug() << "finished";

    delete d;
}


K3b::AudioDoc* K3b::AudioTrack::doc() const
{
    return d->parent;
}


void K3b::AudioTrack::emitChanged()
{
    emit changed();

    if( d->parent && !d->currentlyDeleting )
        d->parent->slotTrackChanged( this );
}


void K3b::AudioTrack::setArtist( const QString& a )
{
    setPerformer( a );
}


void K3b::AudioTrack::setPerformer( const QString& a )
{
    if( performer() != a ) {
        QString s( a );
        d->cdTextValidator->fixup( s );
        d->cdText.setPerformer(s);
        emitChanged();
    }
}


void K3b::AudioTrack::setTitle( const QString& t )
{
    if( title() != t ) {
        QString s( t );
        d->cdTextValidator->fixup( s );
        d->cdText.setTitle(s);
        emitChanged();
    }
}


void K3b::AudioTrack::setArranger( const QString& t )
{
    if( arranger() != t ) {
        QString s( t );
        d->cdTextValidator->fixup( s );
        d->cdText.setArranger(s);
        emitChanged();
    }
}


void K3b::AudioTrack::setSongwriter( const QString& t )
{
    if( songwriter() != t ) {
        QString s( t );
        d->cdTextValidator->fixup( s );
        d->cdText.setSongwriter(s);
        emitChanged();
    }
}


void K3b::AudioTrack::setComposer( const QString& t )
{
    if( composer() != t ) {
        QString s( t );
        d->cdTextValidator->fixup( s );
        d->cdText.setComposer(s);
        emitChanged();
    }
}


void K3b::AudioTrack::setIsrc( const QString& t )
{
    if( isrc() != t ) {
        d->cdText.setIsrc(t);
        emitChanged();
    }
}


void K3b::AudioTrack::setCdTextMessage( const QString& t )
{
    if( cdTextMessage() != t ) {
        QString s( t );
        d->cdTextValidator->fixup( s );
        d->cdText.setMessage(s);
        emitChanged();
    }
}


void K3b::AudioTrack::setCdText( const K3b::Device::TrackCdText& cdtext )
{
    d->cdText = cdtext;
    emitChanged();
}


void K3b::AudioTrack::setPreEmp( bool b )
{
    if( d->preEmp != b ) {
        d->preEmp = b;
        emitChanged();
    }
}


void K3b::AudioTrack::setCopyProtection( bool b )
{
    if( d->copy != b ) {
        d->copy = b;
        emitChanged();
    }
}


K3b::AudioDataSource* K3b::AudioTrack::firstSource() const
{
    return d->firstSource;
}


K3b::AudioDataSource* K3b::AudioTrack::lastSource() const
{
    K3b::AudioDataSource* s = d->firstSource;
    while( s && s->next() )
        s = s->next();
    return s;
}


bool K3b::AudioTrack::inList() const
{
    if( doc() )
        return ( doc()->firstTrack() == this || d->prev != 0 );
    else
        return false;
}


K3b::Msf K3b::AudioTrack::length() const
{
    K3b::Msf length;
    K3b::AudioDataSource* source = d->firstSource;
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

QString K3b::AudioTrack::artist() const
{
    return d->cdText.performer();
}


QString K3b::AudioTrack::performer() const
{
    return d->cdText.performer();
}


QString K3b::AudioTrack::title() const
{
    return d->cdText.title();
}


QString K3b::AudioTrack::arranger() const
{
    return d->cdText.arranger();
}


QString K3b::AudioTrack::songwriter() const
{
    return d->cdText.songwriter();
}


QString K3b::AudioTrack::composer() const
{
    return d->cdText.composer();
}


QString K3b::AudioTrack::isrc() const
{
    return d->cdText.isrc();
}


QString K3b::AudioTrack::cdTextMessage() const
{
    return d->cdText.message();
}


K3b::Device::TrackCdText K3b::AudioTrack::cdText() const
{
    return d->cdText;
}


bool K3b::AudioTrack::copyProtection() const
{
    return d->copy;
}


bool K3b::AudioTrack::preEmp() const
{
    return d->preEmp;
}


unsigned int K3b::AudioTrack::trackNumber() const
{
    if( d->prev )
        return d->prev->trackNumber() + 1;
    else
        return 1;
}


K3b::Msf K3b::AudioTrack::index0() const
{
    // we save the index0Offset as length of the resulting pregap
    // this way the length of the track does not need to be ready
    // when creating the track.
    return length() - d->index0Offset;
}


K3b::Msf K3b::AudioTrack::postGap() const
{
    if( next() )
        return d->index0Offset;
    else
        return 0;
}


void K3b::AudioTrack::setIndex0( const K3b::Msf& msf )
{
    if( msf == 0 )
        d->index0Offset = 0;
    else
        d->index0Offset = length() - msf;
}


K3b::AudioTrack* K3b::AudioTrack::take()
{
    if( inList() ) {
        const int position = trackNumber() - 1;
        if ( doc() )
            emit doc()->trackAboutToBeRemoved( position );

        if( !d->prev )
            doc()->setFirstTrack( d->next );
        if( !d->next )
            doc()->setLastTrack( d->prev );

        if( d->prev )
            d->prev->d->next = d->next;
        if( d->next )
            d->next->d->prev = d->prev;

        d->prev = d->next = 0;

        // remove from doc
        if( doc() )
            doc()->slotTrackRemoved( position );

        d->parent = 0;
    }

    return this;
}


void K3b::AudioTrack::moveAfter( K3b::AudioTrack* track )
{
    qDebug() << "(K3b::AudioTrack::moveAfter( " << track << " )";
    if( !track ) {
        if( !doc() ) {
            qDebug() << "(K3b::AudioTrack::moveAfter) no parent set";
            return;
        }

        // make sure we do not mess up the list
        if( doc()->lastTrack() )
            moveAfter( doc()->lastTrack() );
        else {
            emit doc()->trackAboutToBeAdded( 0 );
            doc()->setFirstTrack( take() );
            doc()->setLastTrack( this );
            emit doc()->trackAdded( 0 );
        }
    }
    else if( track == this ) {
        qDebug() << "(K3b::AudioTrack::moveAfter) trying to move this after this.";
        return;
    }
    else {
        // remove this from the list
        take();

        emit track->doc()->trackAboutToBeAdded( track->trackNumber()-1 );

        // set the new parent doc
        d->parent = track->doc();

        K3b::AudioTrack* oldNext = track->d->next;

        // set track as prev
        track->d->next = this;
        d->prev = track;

        // set oldNext as next
        if( oldNext )
            oldNext->d->prev = this;
        d->next = oldNext;

        if( !d->prev )
            doc()->setFirstTrack( this );
        if( !d->next )
            doc()->setLastTrack( this );

        emit doc()->trackAdded( track->trackNumber()-1 );
    }

    emitChanged();
}


void K3b::AudioTrack::moveAhead( K3b::AudioTrack* track )
{
    if( !track ) {
        if( !doc() ) {
            qDebug() << "(K3b::AudioTrack::moveAfter) no parent set";
            return;
        }

        // make sure we do not mess up the list
        if( doc()->firstTrack() )
            moveAhead( doc()->firstTrack() );
        else {
            emit doc()->trackAboutToBeAdded( 0 );
            doc()->setFirstTrack( take() );
            doc()->setLastTrack( this );
            emit doc()->trackAdded( 0 );
        }
    }
    else if( track == this ) {
        qDebug() << "(K3b::AudioTrack::moveAhead) trying to move this ahead of this.";
        return;
    }
    else {
        // remove this from the list
        take();

        emit track->doc()->trackAboutToBeAdded( track->trackNumber()-1 );

        // set the new parent doc
        d->parent = track->doc();

        K3b::AudioTrack* oldPrev = track->d->prev;

        // set track as next
        d->next = track;
        track->d->prev = this;

        // set oldPrev as prev
        d->prev = oldPrev;
        if( oldPrev )
            oldPrev->d->next = this;

        if( !d->prev )
            doc()->setFirstTrack( this );
        if( !d->next )
            doc()->setLastTrack( this );

        emit doc()->trackAdded( track->trackNumber()-1 );
    }

    emitChanged();
}


void K3b::AudioTrack::merge( K3b::AudioTrack* trackToMerge, K3b::AudioDataSource* sourceAfter )
{
    qDebug() << "(K3b::AudioTrack::merge) " << trackToMerge << " into " << this;
    if( this == trackToMerge ) {
        qDebug() << "(K3b::AudioTrack::merge) trying to merge this with this.";
        return;
    }

    // remove the track to merge to make sure it does not get deleted by the doc too early
    trackToMerge->take();

    // in case we prepend all of trackToMerge's sources
    if( !sourceAfter ) {
        qDebug() << "(K3b::AudioTrack::merge) merging " << trackToMerge->firstSource();
        if( d->firstSource ) {
            trackToMerge->firstSource()->moveAhead( d->firstSource );
        }
        else {
            addSource( trackToMerge->firstSource()->take() );
        }
        sourceAfter = d->firstSource;
    }

    qDebug() << "(K3b::AudioTrack::merge) now merge the other sources.";
    // now merge all sources into this track
    while( trackToMerge->firstSource() ) {
        K3b::AudioDataSource* s = trackToMerge->firstSource();
        qDebug() << "(K3b::AudioTrack::merge) merging source " << s << " from track " << s->track() << " into track "
                 << this << " after source " << sourceAfter << endl;
        s->moveAfter( sourceAfter );
        sourceAfter = s;
    }

    // TODO: should we also merge the indices?

    // now we can safely delete the track we merged
    delete trackToMerge;

    qDebug() << "(K3b::AudioTrack::merge) finished";

    emitChanged();
}


K3b::AudioTrack* K3b::AudioTrack::prev() const
{
    return d->prev;
}


K3b::AudioTrack* K3b::AudioTrack::next() const
{
    return d->next;
}


void K3b::AudioTrack::setFirstSource( K3b::AudioDataSource* source )
{
    d->firstSource = source;
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

    K3b::AudioDataSource* s = d->firstSource;
    while( s && s->next() )
        s = s->next();
    if( s )
        source->moveAfter( s );
    else
        setFirstSource( source->take() );
}


void K3b::AudioTrack::sourceChanged( K3b::AudioDataSource* )
{
    if( d->currentlyDeleting )
        return;

    // TODO: update indices

    if( d->index0Offset > length() )
        d->index0Offset = length()-1;

    emitChanged();
}


int K3b::AudioTrack::numberSources() const
{
    K3b::AudioDataSource* source = d->firstSource;
    int i = 0;
    while( source ) {
        source = source->next();
        ++i;
    }
    return i;
}


K3b::AudioTrack* K3b::AudioTrack::copy() const
{
    K3b::AudioTrack* track = new K3b::AudioTrack();

    track->d->copy = d->copy;
    track->d->preEmp = d->preEmp;
    track->d->index0Offset = d->index0Offset;
    track->d->cdText = d->cdText;
    K3b::AudioDataSource* source = d->firstSource;
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
        splitTrack->d->cdText = d->cdText;
        source = splitSource;
        while( source ) {
            K3b::AudioDataSource* addSource = source;
            source = source->next();
            splitTrack->addSource( addSource );
        }

        qDebug() << "(K3b::AudioTrack) moving track " << splitTrack << " after this (" << this << ") with parent " << doc();
        splitTrack->moveAfter( this );

        return splitTrack;
    }
    else
        return 0;
}


QIODevice* K3b::AudioTrack::createReader( QObject* parent )
{
    return new AudioTrackReader( *this, parent );
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
    qDebug() << "Track " << this << endl
             << "  Prev: " << d->prev << endl
             << "  Next: " << d->next << endl
             << "  Sources:" << endl;
    K3b::AudioDataSource* s = d->firstSource;
    while( s ) {
        qDebug() << "  " << s << " - Prev: " << s->prev() << " Next: " << s->next();
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


void K3b::AudioTrack::emitSourceAboutToBeRemoved( AudioDataSource* source )
{
    emit sourceAboutToBeRemoved( source->sourceIndex() );

    if ( doc() ) {
        emit doc()->sourceAboutToBeRemoved( this, source->sourceIndex() );
    }
}


void K3b::AudioTrack::emitSourceRemoved( K3b::AudioDataSource* source )
{
    if ( doc() ) {
        // set the first source by hand (without using setFirstSource() )
        // just to avoid the model to read invalid firstSources
        if ( !source->prev() )
            d->firstSource = source->next();

        emit doc()->sourceRemoved( this, source->sourceIndex() );
    }

    emit sourceRemoved( source->sourceIndex() );

    // and now call the setFirstSource() to make sure the proper signals
    // are emitted
    if ( !source->prev() )
        setFirstSource( source->next() );
}


void K3b::AudioTrack::emitSourceAboutToBeAdded( int position )
{
    emit sourceAboutToBeAdded( position );

    if ( doc() ) {
        emit doc()->sourceAboutToBeAdded( this, position );
    }
}


void K3b::AudioTrack::emitSourceAdded( AudioDataSource* source )
{
    if ( doc() ) {
        emit doc()->sourceAdded( this, source->sourceIndex() );
        doc()->slotTrackChanged( this );
    }

    emit sourceAdded( source->sourceIndex() );
}


void K3b::AudioTrack::setIndex0Offset( const Msf& index0Offset )
{
    d->index0Offset = index0Offset;
}


void K3b::AudioTrack::setParent( K3b::AudioDoc* parent )
{
    d->parent = parent;
}


#include "k3baudiotrack.moc"
