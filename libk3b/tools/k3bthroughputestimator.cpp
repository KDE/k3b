/*
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3bthroughputestimator.h"

#include <qdatetime.h>
#include <QtCore/QDebug>


class K3b::ThroughputEstimator::Private
{
public:
    Private()
        : started(false) {
    }

    QTime firstDataTime;
    unsigned long firstData;
    QTime lastDataTime;
    unsigned long lastData;

    int lastThroughput;

    bool started;
};


K3b::ThroughputEstimator::ThroughputEstimator( QObject* parent )
    : QObject( parent )
{
    d = new Private();
}


K3b::ThroughputEstimator::~ThroughputEstimator()
{
    delete d;
}


int K3b::ThroughputEstimator::average() const
{
    int msecs = d->firstDataTime.msecsTo( d->lastDataTime );
    if( msecs > 0 )
        return (int)( 1000.0*(double)(d->lastData - d->firstData)/(double)msecs);
    else
        return 0;
}


void K3b::ThroughputEstimator::reset()
{
    d->started = false;
}


void K3b::ThroughputEstimator::dataWritten( unsigned long data )
{
    if( !d->started ) {
        d->started = true;
        d->firstData = d->lastData = data;
        d->firstDataTime.start();
        d->lastDataTime.start();
        d->lastThroughput = 0;
    }
    else if( data > d->lastData ) {
        unsigned long diff = data - d->lastData;
        int msecs = d->lastDataTime.elapsed();

        //if( msecs > 0 ) {
        // down the update sequence a little bit
        if( msecs > 500 ) {
            d->lastData = data;
            d->lastDataTime.start();
            int t = (int)(1000.0*(double)diff/(double)msecs);
            if( t != d->lastThroughput ) {
                d->lastThroughput = t;
                emit throughput( t );
            }
        }
    }
}


#include "k3bthroughputestimator.moc"
