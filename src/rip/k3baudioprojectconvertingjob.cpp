/*
 *
 * Copyright (C) 2005-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010-2011 Michal Malek <michalm@jabster.pl>
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


#include "k3baudioprojectconvertingjob.h"
#include "k3baudiodoc.h"
#include "k3baudioencoder.h"
#include "k3baudiotrack.h"
#include "k3baudiotrackreader.h"

#include <KCddb/Cdinfo>

#include <KLocalizedString>


namespace K3b {

class AudioProjectConvertingJob::Private
{
public:
    Private( AudioDoc* d )
        : doc(d) {
    }

    AudioDoc* doc;
};


AudioProjectConvertingJob::AudioProjectConvertingJob( AudioDoc* doc, JobHandler* hdl, QObject* parent )
    : MassAudioEncodingJob( true, hdl,  parent ),
      d( new Private( doc ) )
{
}


AudioProjectConvertingJob::~AudioProjectConvertingJob()
{
}


QString AudioProjectConvertingJob::jobDescription() const
{
    if( cddbEntry().get( KCDDB::Title ).toString().isEmpty() )
        return i18n( "Converting Audio Tracks" );
    else
        return i18n( "Converting Audio Tracks From '%1'",
                     cddbEntry().get( KCDDB::Title ).toString() );
}


bool AudioProjectConvertingJob::init()
{
    emit newTask( i18n("Converting Audio Tracks")  );
    emit infoMessage( i18n("Starting audio conversion."), Job::MessageInfo );
    return true;
}


Msf AudioProjectConvertingJob::trackLength( int trackIndex ) const
{
    if( AudioTrack* track = d->doc->getTrack( trackIndex ) )
        return track->length();
    else
        return Msf();
}


QIODevice* AudioProjectConvertingJob::createReader( int trackIndex ) const
{
    if( AudioTrack* track = d->doc->getTrack( trackIndex ) )
        return new AudioTrackReader( *track );
    else
        return 0;
}


void AudioProjectConvertingJob::trackStarted( int trackIndex )
{
    if( !cddbEntry().track( trackIndex-1 ).get( KCDDB::Artist ).toString().isEmpty() &&
        !cddbEntry().track( trackIndex-1 ).get( KCDDB::Title ).toString().isEmpty() )
        emit newSubTask( i18n( "Converting track %1 (%2 - %3)",
                               trackIndex,
                               cddbEntry().track( trackIndex-1 ).get( KCDDB::Artist ).toString(),
                               cddbEntry().track( trackIndex-1 ).get( KCDDB::Title ).toString() ) );
    else
        emit newSubTask( i18n("Converting track %1", trackIndex) );
}


void AudioProjectConvertingJob::trackFinished( int trackIndex, const QString& /*filename*/ )
{
    emit infoMessage( i18n("Successfully converted track %1.", trackIndex), Job::MessageInfo );
}

} // namespace K3b


