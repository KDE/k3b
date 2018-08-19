/*
 *
 * Copyright (C) 2005-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
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

#include "k3bmusicbrainztrackloopupjob.h"
#include "k3btrm.h"
#include "k3bmusicbrainz.h"
#include "k3baudiotrack.h"
#include "k3baudiotrackreader.h"

#include <KLocalizedString>


class K3b::MusicBrainzTrackLookupJob::Private
{
public:
    K3b::AudioTrack* track;
    K3b::TRM trm;
    K3b::MusicBrainz mb;
    int results;
};


K3b::MusicBrainzTrackLookupJob::MusicBrainzTrackLookupJob( K3b::JobHandler* hdl, QObject* parent )
    : K3b::ThreadJob( hdl, parent ),
      d( new Private() )
{
    d->track = 0;
}


K3b::MusicBrainzTrackLookupJob::~MusicBrainzTrackLookupJob()
{
    delete d;
}


void K3b::MusicBrainzTrackLookupJob::setAudioTrack( K3b::AudioTrack* track )
{
    d->track = track;
}


int K3b::MusicBrainzTrackLookupJob::results()
{
    return d->results;
}


QString K3b::MusicBrainzTrackLookupJob::title( int i ) const
{
    return d->mb.title( i );
}


QString K3b::MusicBrainzTrackLookupJob::artist( int i ) const
{
    return d->mb.artist( i );
}


bool K3b::MusicBrainzTrackLookupJob::run()
{
    if ( !d->track ) {
        emit infoMessage( "Internal error: no track set. This is a bug!", MessageError );
        return false;
    }

    emit infoMessage( i18n("Generating fingerprint for track %1.",
                           d->track->trackNumber()), MessageInfo );

    AudioTrackReader trackReader( *d->track );
    d->trm.start( d->track->length() );

    char buffer[2352*10];
    qint64 len = 0;
    qint64 dataRead = 0;
    while( !canceled() &&
           (len = trackReader.read( buffer, sizeof(buffer) )) > 0 ) {

        dataRead += len;

        // swap data
        char buf;
        for( qint64 i = 0; i < len-1; i+=2 ) {
            buf = buffer[i];
            buffer[i] = buffer[i+1];
            buffer[i+1] = buf;
        }

        if( d->trm.generate( buffer, len ) ) {
            len = 0;
            break;
        }

        // FIXME: useless since libmusicbrainz does never need all the data
        emit percent( 100LL*dataRead/trackReader.size() );
    }

    if( canceled() ) {
        return false;
    }
    else if( len != 0 ||
             !d->trm.finalize() ) {
        return false;
    }

    emit infoMessage( i18n("Querying MusicBrainz for track %1.",
                           d->track->trackNumber()), MessageInfo );

    d->results = d->mb.query( d->trm.signature() );
    return( d->results > 0 );
}


