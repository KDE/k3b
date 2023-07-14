/*
    SPDX-FileCopyrightText: 1998-2008 Sebastian Trueg <trueg@k3b.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/


#include "k3bthread.h"
#include "k3bthreadjob.h"
#include "k3bprogressinfoevent.h"
#include "k3bthreadjobcommunicationevent.h"

#include <QDebug>
#include <QList>
#include <QTimer>


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
    QTimer::singleShot( 5000, this, SLOT(slotEnsureDoneTimeout()) );
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
        qDebug() << "Waiting for thread " << thread << Qt::endl;
        thread->wait();
    }

    qDebug() << "Thread waiting done." << Qt::endl;
}

#include "moc_k3bthread.cpp"
