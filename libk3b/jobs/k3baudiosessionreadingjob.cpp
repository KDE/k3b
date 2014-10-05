/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
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

#include "k3baudiosessionreadingjob.h"

#include "k3bthread.h"
#include "k3btoc.h"
#include "k3bcdparanoialib.h"
#include "k3bwavefilewriter.h"
#include "k3bglobals.h"
#include "k3bdevice.h"
#include "k3bcore.h"
#include "k3b_i18n.h"

#include <QtCore/QDebug>

#include <unistd.h>


class K3b::AudioSessionReadingJob::Private
{
public:
    Private();
    ~Private();

    QIODevice* ioDev;
    K3b::CdparanoiaLib* paranoia;
    K3b::Device::Device* device;
    K3b::Device::Toc toc;
    K3b::WaveFileWriter* waveFileWriter;
    QStringList filenames;
    int paranoiaMode;
    int retries;
    bool neverSkip;
};


K3b::AudioSessionReadingJob::Private::Private()
    : ioDev( 0 ),
      paranoia(0),
      waveFileWriter(0),
      paranoiaMode(0),
      retries(50),
      neverSkip(false)
{
}


K3b::AudioSessionReadingJob::Private::~Private()
{
    delete waveFileWriter;
    delete paranoia;
}


K3b::AudioSessionReadingJob::AudioSessionReadingJob( K3b::JobHandler* jh, QObject* parent )
    : K3b::ThreadJob( jh, parent ),
      d( new Private() )
{
}


K3b::AudioSessionReadingJob::~AudioSessionReadingJob()
{
    delete d;
}


void K3b::AudioSessionReadingJob::setDevice( K3b::Device::Device* dev )
{
    d->device = dev;
    d->toc = K3b::Device::Toc();
}


void K3b::AudioSessionReadingJob::setToc( const K3b::Device::Toc& toc )
{
    d->toc = toc;
}


void K3b::AudioSessionReadingJob::writeTo( QIODevice* ioDev )
{
    d->ioDev = ioDev;
}

void K3b::AudioSessionReadingJob::setImageNames( const QStringList& l )
{
    d->filenames = l;
    d->ioDev = 0;
}


void K3b::AudioSessionReadingJob::setParanoiaMode( int m )
{
    d->paranoiaMode = m;
}


void K3b::AudioSessionReadingJob::setReadRetries( int r )
{
    d->retries = r;
}

void K3b::AudioSessionReadingJob::setNeverSkip( bool b )
{
    d->neverSkip = b;
}


void K3b::AudioSessionReadingJob::start()
{
    k3bcore->blockDevice( d->device );
    K3b::ThreadJob::start();
}


void K3b::AudioSessionReadingJob::jobFinished( bool success )
{
    k3bcore->unblockDevice( d->device );
    K3b::ThreadJob::jobFinished( success );
}


bool K3b::AudioSessionReadingJob::run()
{
    if( !d->paranoia )
        d->paranoia = K3b::CdparanoiaLib::create();

    if( !d->paranoia ) {
        emit infoMessage( i18n("Could not load libcdparanoia."), K3b::Job::MessageError );
        return false;
    }

    if( d->toc.isEmpty() )
        d->toc = d->device->readToc();

    if( !d->paranoia->initParanoia( d->device, d->toc ) ) {
        emit infoMessage( i18n("Could not open device %1", d->device->blockDeviceName()),
                          K3b::Job::MessageError );
        return false;
    }

    if( !d->paranoia->initReading() ) {
        emit infoMessage( i18n("Error while initializing audio ripping."), K3b::Job::MessageError );
        return false;
    }

    d->device->block( true );

    // init settings
    d->paranoia->setMaxRetries( d->retries );
    d->paranoia->setParanoiaMode( d->paranoiaMode );
    d->paranoia->setNeverSkip( d->neverSkip );

    bool writeError = false;
    unsigned int trackNum = 1;
    unsigned int currentTrack = 0;
    unsigned long trackRead = 0;
    unsigned long totalRead = 0;
    unsigned int lastTrackPercent = 0;
    unsigned int lastTotalPercent = 0;
    bool newTrack = true;
    int status = 0;
    char* buffer = 0;
    while( !canceled() && (buffer = d->paranoia->read( &status, &trackNum, !d->ioDev /*when writing to a wav be want little endian */ )) ) {

        if( currentTrack != trackNum ) {
            emit nextTrack( trackNum, d->paranoia->toc().count() );
            trackRead = 0;
            lastTrackPercent = 0;

            currentTrack = trackNum;
            newTrack = true;
        }

        if( d->ioDev ) {
            if( d->ioDev->write( buffer, CD_FRAMESIZE_RAW ) != CD_FRAMESIZE_RAW ) {
                qDebug() << "(K3b::AudioSessionCopyJob::WorkThread) error while writing to device " << d->ioDev;
                writeError = true;
                break;
            }
        }
        else {
            if( newTrack ) {
                newTrack = false;

                if( !d->waveFileWriter )
                    d->waveFileWriter = new K3b::WaveFileWriter();

                if( d->filenames.count() < ( int )currentTrack ) {
                    qDebug() << "(K3b::AudioSessionCopyJob) not enough image filenames given: " << currentTrack;
                    writeError = true;
                    break;
                }

                if( !d->waveFileWriter->open( d->filenames[currentTrack-1] ) ) {
                    emit infoMessage( i18n("Unable to open '%1' for writing.", d->filenames[currentTrack-1]), K3b::Job::MessageError );
                    writeError = true;
                    break;
                }
            }

            d->waveFileWriter->write( buffer,
                                      CD_FRAMESIZE_RAW,
                                      K3b::WaveFileWriter::LittleEndian );
        }

        trackRead++;
        totalRead++;

        unsigned int trackPercent = 100 * trackRead / d->toc[currentTrack-1].length().lba();
        if( trackPercent > lastTrackPercent ) {
            lastTrackPercent = trackPercent;
            emit subPercent( lastTrackPercent );
        }
        unsigned int totalPercent = 100 * totalRead / d->paranoia->rippedDataLength();
        if( totalPercent > lastTotalPercent ) {
            lastTotalPercent = totalPercent;
            emit percent( lastTotalPercent );
        }
    }

    if( d->waveFileWriter )
        d->waveFileWriter->close();

    d->paranoia->close();

    d->device->block( false );

    if( status != K3b::CdparanoiaLib::S_OK ) {
        emit infoMessage( i18n("Unrecoverable error while ripping track %1.", trackNum), K3b::Job::MessageError );
        return false;
    }

    return !writeError && !canceled();
}


