/*
    SPDX-FileCopyrightText: 2010 Michal Malek <michalm@jabster.pl>
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "k3baudioimager.h"
#include "k3baudiodoc.h"
#include "k3baudiojobtempdata.h"
#include "k3baudiotrack.h"
#include "k3baudiotrackreader.h"
#include "k3baudiodatasource.h"
#include "k3bthread.h"
#include "k3bwavefilewriter.h"
#include "k3b_i18n.h"

#include <QDebug>
#include <QIODevice>
#include <QFile>

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

    qint64 totalSize = d->doc->length().audioBytes();
    qint64 totalRead = 0;
    char buffer[2352 * 10];

    for( AudioTrack* track = d->doc->firstTrack(); track != 0; track = track->next() ) {

        emit nextTrack( track->trackNumber(), d->doc->numOfTracks() );

        //
        // Create track reader
        //
        AudioTrackReader trackReader( *track );
        if( !trackReader.open() ) {
            emit infoMessage( i18n("Unable to read track %1.", track->trackNumber()), K3b::Job::MessageError );
            return false;
        }

        //
        // Initialize the reading
        //
        qint64 read = 0;
        qint64 trackRead = 0;

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
        while( !trackReader.atEnd() && (read = trackReader.read( buffer, sizeof(buffer) )) > 0 ) {
            if( !d->ioDev ) {
                waveFileWriter.write( buffer, read, K3b::WaveFileWriter::BigEndian );
            }
            else {
                qint64 w = d->ioDev->write( buffer, read );
                if ( w != read ) {
                    qDebug() << "(K3b::AudioImager::WorkThread) writing to device" << d->ioDev << "failed:" << read << w;
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

            emit subPercent( 100LL*trackRead/trackReader.size() );
            emit percent( 100LL*totalRead/totalSize );
            emit processedSubSize( trackRead/1024LL/1024LL, trackReader.size()/1024LL/1024LL );
            emit processedSize( totalRead/1024LL/1024LL, totalSize/1024LL/1024LL );
        }

        if( read < 0 ) {
            emit infoMessage( i18n("Error while decoding track %1.", track->trackNumber()), K3b::Job::MessageError );
            qDebug() << "(K3b::AudioImager::WorkThread) read error on track " << track->trackNumber()
                     << " at pos " << K3b::Msf(trackRead/2352) << Qt::endl;
            d->lastError = K3b::AudioImager::ERROR_DECODING_TRACK;
            return false;
        }
    }

    return true;
}


