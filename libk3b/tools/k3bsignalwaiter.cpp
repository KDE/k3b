/*
 *
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bsignalwaiter.h"
#include "k3bjob.h"

#include <qeventloop.h>
#include <qapplication.h>

class K3bSignalWaiter::Private
{
public:
    QEventLoop loop;
};


K3bSignalWaiter::K3bSignalWaiter()
    : QObject(),
      d( new Private() )
{
}


K3bSignalWaiter::~K3bSignalWaiter()
{
    delete d;
}


void K3bSignalWaiter::waitForSignal( QObject* o, const char* signal )
{
    K3bSignalWaiter w;
    connect( o, signal,
             &w, SLOT(slotSignal()) );

    w.d->loop.exec();
}


void K3bSignalWaiter::waitForJob( K3bJob* job )
{
    if( !job->active() )
        return;

    waitForSignal( job, SIGNAL(finished(bool)) );
}


void K3bSignalWaiter::slotSignal()
{
    if( d->loop.isRunning() ) {
        d->loop.exit();
    }
}

#include "k3bsignalwaiter.moc"
