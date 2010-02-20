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

#include "k3baudioimager.h"
#include "k3baudiodoc.h"
#include "k3baudiojobtempdata.h"
#include "k3baudiotrack.h"
#include "k3baudiodatasource.h"

#include "k3bthread.h"
#include "k3bwavefilewriter.h"

#include <klocale.h>
#include <kdebug.h>

#include <qfile.h>
#include <QtCore/QIODevice>

#include <unistd.h>


class K3b::AudioImager::Private
{
public:
    Private()
        : ioDev(0) {
    }

    QIODevice* ioDev;
    AudioImager::ErrorType lastError;
    AudioDoc* doc;
    AudioJobTempData* tempData;
};



K3b::AudioImager::AudioImager( AudioDoc* doc, AudioJobTempData* tempData, JobHandler* jh, QObject* parent )
    : K3b::ThreadJob( jh, parent ),
      d( new Private() )
{
    d->doc = doc;
    d->tempData = tempData;
}


K3b::AudioImager::~AudioImager()
{
    delete d;
}


void K3b::AudioImager::writeTo( QIODevice* dev )
{
    d->ioDev = dev;
}


K3b::AudioImager::ErrorType K3b::AudioImager::lastErrorType() const
{
    return d->lastError;
}


bool K3b::AudioImager::run()
{
    d->lastError = K3b::AudioImager::ERROR_UNKNOWN;

    K3b::WaveFileWriter waveFileWriter;

    unsigned long long totalSize = d->doc->length().audioBytes();
    unsigned long long totalRead = 0;
    char buffer[2352 * 10];

    for( AudioTrack* track = d->doc->firstTrack(); track != 0; track = track->next() ) {

        emit nextTrack( track->trackNumber(), d->doc->numOfTracks() );

        //
        // Seek to the beginning of the track
        //
        if( !track->seek(0) ) {
            emit infoMessage( i18n("Unable to seek in track %1.", track->trackNumber()), K3b::Job::MessageError );
            return false;
        }

        //
        // Initialize the reading
        //
        int read = 0;
        unsigned long long trackRead = 0;

        //
        // Create the image file
        //
        if( !d->ioDev ) {
            QString imageFile = d->tempData->bufferFileName( track );
            if( !waveFileWriter.open( imageFile ) ) {
                emit infoMessage( i18n("Could not open %1 for writing", imageFile), K3b::Job::MessageError );
                return false;
            }
        }

        //
        // Read data from the track
        //
        while( (read = track->read( buffer, sizeof(buffer) )) > 0 ) {
            if( !d->ioDev ) {
                waveFileWriter.write( buffer, read, K3b::WaveFileWriter::BigEndian );
            }
            else {
                qint64 w = d->ioDev->write( buffer, read );
                if ( w != read ) {
                    kDebug() << "(K3b::AudioImager::WorkThread) writing to device" << d->ioDev << "failed:" << read << w;
                    d->lastError = K3b::AudioImager::ERROR_FD_WRITE;
                    return false;
                }
            }

            if( canceled() ) {
                return false;
            }

            //
            // Emit progress
            //
            totalRead += read;
            trackRead += read;

            emit subPercent( 100*trackRead/track->length().audioBytes() );
            emit percent( 100*totalRead/totalSize );
            emit processedSubSize( trackRead/1024/1024, track->length().audioBytes()/1024/1024 );
            emit processedSize( totalRead/1024/1024, totalSize/1024/1024 );
        }

        if( read < 0 ) {
            emit infoMessage( i18n("Error while decoding track %1.", track->trackNumber()), K3b::Job::MessageError );
            kDebug() << "(K3b::AudioImager::WorkThread) read error on track " << track->trackNumber()
                     << " at pos " << K3b::Msf(trackRead/2352) << endl;
            d->lastError = K3b::AudioImager::ERROR_DECODING_TRACK;
            return false;
        }
    }

    return true;
}

#include "k3baudioimager.moc"
