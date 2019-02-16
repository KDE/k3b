/*
 *
 * Copyright (C) 2005-2008 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bmusicbrainzjob.h"
#include "k3bmusicbrainztrackloopupjob.h"

#include "k3baudiotrack.h"
#include "k3baudiodatasource.h"
#include "k3bsimplejobhandler.h"

#include <KLocalizedString>
#include <KMessageBox>

#include <QInputDialog>


class K3b::MusicBrainzJob::Private
{
public:
    QList<K3b::AudioTrack*> tracks;
    int currentTrackIndex;
    bool canceled;
    K3b::MusicBrainzTrackLookupJob* mbTrackLookupJob;
};


// cannot use this as parent for the K3b::SimpleJobHandler since this has not been constructed yet
K3b::MusicBrainzJob::MusicBrainzJob( QWidget* parent )
    : K3b::Job( new K3b::SimpleJobHandler( 0 ), parent ),
      d( new Private() )
{
    d->canceled = false;
    d->mbTrackLookupJob = new K3b::MusicBrainzTrackLookupJob( this, this );

    connect( d->mbTrackLookupJob, SIGNAL(percent(int)), this, SIGNAL(subPercent(int)), Qt::QueuedConnection );
    connect( d->mbTrackLookupJob, SIGNAL(percent(int)), this, SLOT(slotTrmPercent(int)), Qt::QueuedConnection );
    connect( d->mbTrackLookupJob, SIGNAL(finished(bool)), this, SLOT(slotMbJobFinished(bool)), Qt::QueuedConnection );
    connect( d->mbTrackLookupJob, SIGNAL(infoMessage(QString,int)), this, SIGNAL(infoMessage(QString,int)), Qt::QueuedConnection );
}


K3b::MusicBrainzJob::~MusicBrainzJob()
{
    delete jobHandler();
    delete d;
}


bool K3b::MusicBrainzJob::hasBeenCanceled() const
{
    return d->canceled;
}


void K3b::MusicBrainzJob::setTracks( const QList<K3b::AudioTrack*>& tracks )
{
    d->tracks = tracks;
}


void K3b::MusicBrainzJob::start()
{
    jobStarted();

    d->canceled = false;
    d->currentTrackIndex = 0;

    d->mbTrackLookupJob->setAudioTrack( d->tracks.first() );
    d->mbTrackLookupJob->start();
}


void K3b::MusicBrainzJob::cancel()
{
    d->canceled = true;
    d->mbTrackLookupJob->cancel();
}


void K3b::MusicBrainzJob::slotTrmPercent( int p )
{
    // the easy way (inaccurate)
    emit percent( (100*d->currentTrackIndex + p) / d->tracks.count() );
}


void K3b::MusicBrainzJob::slotMbJobFinished( bool success )
{
    if( hasBeenCanceled() ) {
        emit canceled();
        jobFinished(false);
    }
    else {
        K3b::AudioTrack* currentTrack = d->tracks.at( d->currentTrackIndex );

        if( success ) {
            // found entries
            QStringList resultStrings, resultStringsUnique;
            for( int i = 0; i < d->mbTrackLookupJob->results(); ++i )
                resultStrings.append( d->mbTrackLookupJob->artist(i) + " - " + d->mbTrackLookupJob->title(i) );

            // since we are only using the title and the artist a lot of entries are alike to us
            // so to not let the user have to choose between two equal entries we trim the list down
            for( QStringList::const_iterator it = resultStrings.constBegin();
                 it != resultStrings.constEnd(); ++it )
                if( !resultStringsUnique.contains( *it ) )
                    resultStringsUnique.append( *it );

            QString s;
            bool ok = true;
            if( resultStringsUnique.count() > 1 )
                s = QInputDialog::getItem( dynamic_cast<QWidget*>(parent()),
                                           i18n("MusicBrainz Query"),
                                           i18n("Found multiple matches for track %1 (%2). Please select one.",
                                                currentTrack->trackNumber(),
                                                currentTrack->firstSource()->sourceComment()),
                                           resultStringsUnique,
                                           0,
                                           false,
                                           &ok );
            else
                s = resultStringsUnique.first();

            if( ok ) {
                int i = resultStrings.lastIndexOf( s );
                currentTrack->setTitle( d->mbTrackLookupJob->title(i) );
                currentTrack->setArtist( d->mbTrackLookupJob->artist(i) );
            }
        }

        emit trackFinished( currentTrack, success );

        // query next track
        ++d->currentTrackIndex;
        if( d->currentTrackIndex < d->tracks.count() ) {
            d->mbTrackLookupJob->setAudioTrack( d->tracks.at( d->currentTrackIndex ) );
            d->mbTrackLookupJob->start();
        }
        else {
            jobFinished( true );
        }
    }
}


