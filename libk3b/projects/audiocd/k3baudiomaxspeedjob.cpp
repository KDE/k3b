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

#include "k3baudiomaxspeedjob.h"
#include "k3baudiotrack.h"
#include "k3baudiodatasource.h"
#include "k3baudiodoc.h"
#include "k3baudiocdtracksource.h"
#include "k3baudiodatasourceiterator.h"

#include <k3bdevice.h>

#include <k3bthread.h>

#include <kdebug.h>
#include <klocale.h>

#include <qdatetime.h>


class K3b::AudioMaxSpeedJob::Private
{
public:
    int speedTest( K3b::AudioDataSource* source );
    int maxSpeedByMedia() const;

    int maxSpeed;
    K3b::AudioDoc* doc;
    char* buffer;
};


// returns the amount of data read from this source
int K3b::AudioMaxSpeedJob::Private::speedTest( K3b::AudioDataSource* source )
{
    //
    // in case of an audio track source we only test when the cd is inserted since asking the user would
    // confuse him a lot.
    //
    // FIXME: there is still the problem of the spin up time.
    //
    if( K3b::AudioCdTrackSource* cdts = dynamic_cast<K3b::AudioCdTrackSource*>( source ) ) {
        if( K3b::Device::Device* dev = cdts->searchForAudioCD() ) {
            cdts->setDevice( dev );
        }
        else {
            kDebug() << "(K3b::AudioMaxSpeedJob) ignoring audio cd track source.";
            return 0;
        }
    }

    QTime t;
    int dataRead = 0;
    int r = 0;

    // start the timer
    t.start();

    // read ten seconds of audio data. This is some value which seemed about right. :)
    while( dataRead < 2352*75*10 && (r = source->read( buffer, 2352*10 )) > 0 ) {
        dataRead += r;
    }

    // elapsed millisec
    int usedT = t.elapsed();

    if( r < 0 ) {
        kDebug() << "(K3b::AudioMaxSpeedJob) read failure.";
        return -1;
    }

    // KB/sec (add 1 millisecond to avoid division by 0)
    int throughput = (dataRead*1000+usedT)/(usedT+1)/1024;
    kDebug() << "(K3b::AudioMaxSpeedJob) throughput: " << throughput
             << " (" << dataRead << "/" << usedT << ")" << endl;


    return throughput;
}


int K3b::AudioMaxSpeedJob::Private::maxSpeedByMedia() const
{
    int s = 0;

    QList<int> speeds = doc->burner()->determineSupportedWriteSpeeds();
    // simply use what we have and let the writer decide if the speeds are empty
    if( !speeds.isEmpty() ) {
        // start with the highest speed and go down the list until we are below our max
        QList<int>::const_iterator it = speeds.constEnd();
        --it;
        while( *it > maxSpeed && it != speeds.constBegin() )
            --it;

        // this is the first valid speed or the lowest supported one
        s = *it;
        kDebug() << "(K3b::AudioMaxSpeedJob) using speed factor: " << (s/175);
    }

    return s;
}




K3b::AudioMaxSpeedJob::AudioMaxSpeedJob( K3b::AudioDoc* doc, K3b::JobHandler* jh, QObject* parent )
    : K3b::ThreadJob( jh, parent ),
      d( new Private() )
{
    d->doc = doc;
    d->buffer = new char[2352*10];
}


K3b::AudioMaxSpeedJob::~AudioMaxSpeedJob()
{
    delete [] d->buffer;
    delete d;
}


int K3b::AudioMaxSpeedJob::maxSpeed() const
{
    return d->maxSpeedByMedia();
}


bool K3b::AudioMaxSpeedJob::run()
{
    kDebug() << k_funcinfo;

    K3b::AudioDataSourceIterator it( d->doc );

    // count sources for minimal progress info
    int numSources = 0;
    int sourcesDone = 0;
    while( it.current() ) {
        ++numSources;
        it.next();
    }

    bool success = true;
    d->maxSpeed = 175*1000;
    it.first();

    while( it.current() && !canceled() ) {
        if( !it.current()->seek(0) ) {
            kDebug() << "(K3b::AudioMaxSpeedJob) seek failed.";
            success = false;
            break;
        }

        // read some data
        int speed = d->speedTest( it.current() );

        ++sourcesDone;
        emit percent( 100*numSources/sourcesDone );

        if( speed < 0 ) {
            success = false;
            break;
        }
        else if( speed > 0 ) {
            // update the max speed
            d->maxSpeed = qMin( d->maxSpeed, speed );
        }

        it.next();
    }

    if( canceled() ) {
        success = false;
    }

    if( success )
        kDebug() << "(K3b::AudioMaxSpeedJob) max speed: " << d->maxSpeed;

    return success;
}

#include "k3baudiomaxspeedjob.moc"
