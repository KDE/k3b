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


#include "k3bthread.h"
#include "k3bthreadjob.h"
#include "k3bprogressinfoevent.h"
#include "k3bthreadjobcommunicationevent.h"

#include <kdebug.h>

#include <QtCore/QList>
#include <QtCore/QTimer>


static QList<K3b::Thread*> s_threads;



class K3b::Thread::Private
{
public:
    K3b::ThreadJob* parentJob;
    bool success;
};


K3b::Thread::Thread( K3b::ThreadJob* parent )
    : QThread( parent )
{
    d = new Private;
    d->parentJob = parent;

    s_threads.append(this);
}


K3b::Thread::~Thread()
{
    s_threads.removeAll(this);
    delete d;
}


void K3b::Thread::run()
{
    // default to false in case we need to terminate
    d->success = false;

    // run the job itself
    d->success = d->parentJob->run();
}


bool K3b::Thread::success() const
{
    return d->success;
}


void K3b::Thread::ensureDone()
{
    // we wait for 5 seconds before we terminate the thread
    QTimer::singleShot( 5000, this, SLOT( slotEnsureDoneTimeout() ) );
}


void K3b::Thread::slotEnsureDoneTimeout()
{
    if ( isRunning() ) {
        terminate();
        wait();
    }
}


void K3b::Thread::waitUntilFinished()
{
    foreach( K3b::Thread* thread, s_threads ) {
        kDebug() << "Waiting for thread " << thread << endl;
        thread->wait();
    }

    kDebug() << "Thread waiting done." << endl;
}

#include "k3bthread.moc"
