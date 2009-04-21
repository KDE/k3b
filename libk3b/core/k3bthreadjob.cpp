/*
 *
 * Copyright (C) 2003-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bthreadjob.h"
#include "k3bthread.h"
#include "k3bprogressinfoevent.h"
#include "k3bthreadjobcommunicationevent.h"

#include <kdebug.h>
#include <kapplication.h>


class K3b::ThreadJob::Private
{
public:
    Private()
        : thread( 0 ),
          running( false ),
          canceled( false ) {
    }
    K3b::Thread* thread;
    bool running;
    bool canceled;
};


K3b::ThreadJob::ThreadJob( K3b::JobHandler* jh, QObject* parent )
    : K3b::Job( jh, parent ),
      d( new Private )
{
    d->thread = new K3b::Thread( this );
    connect( d->thread, SIGNAL(finished()),
             this, SLOT(slotThreadFinished()) );
}


K3b::ThreadJob::~ThreadJob()
{
    delete d;
}


bool K3b::ThreadJob::active() const
{
    return d->running;
}


void K3b::ThreadJob::start()
{
    if( !d->running ) {
        d->canceled = false;
        d->running = true;
        jobStarted();
        d->thread->start();
    }
    else {
        kDebug() << "(K3b::ThreadJob) thread not finished yet.";
    }
}


void K3b::ThreadJob::slotThreadFinished()
{
    d->running = false;
    if( canceled() )
        emit canceled();
    jobFinished( d->thread->success() );
}


void K3b::ThreadJob::cancel()
{
    d->canceled = true;
    d->thread->ensureDone();
}


bool K3b::ThreadJob::canceled() const
{
    return d->canceled;
}


K3b::Device::MediaType K3b::ThreadJob::waitForMedia( K3b::Device::Device* device,
                                                     Device::MediaStates mediaState,
                                                     Device::MediaTypes mediaType,
                                                     const QString& message )
{
    K3b::ThreadJobCommunicationEvent* event = K3b::ThreadJobCommunicationEvent::waitForMedium( device,
                                                                                               mediaState,
                                                                                               mediaType,
                                                                                               message );
    QApplication::postEvent( this, event );
    event->wait();
    return (Device::MediaType)event->intResult();
}


bool K3b::ThreadJob::questionYesNo( const QString& text,
                                    const QString& caption,
                                    const QString& yesText,
                                    const QString& noText )
{
    K3b::ThreadJobCommunicationEvent* event = K3b::ThreadJobCommunicationEvent::questionYesNo( text,
                                                                                               caption,
                                                                                               yesText,
                                                                                               noText );
    QApplication::postEvent( this, event );
    event->wait();
    return event->boolResult();
}


void K3b::ThreadJob::blockingInformation( const QString& text,
                                          const QString& caption )
{
    K3b::ThreadJobCommunicationEvent* event = K3b::ThreadJobCommunicationEvent::blockingInformation( text,
                                                                                                     caption );
    QApplication::postEvent( this, event );
    event->wait();
}


void K3b::ThreadJob::customEvent( QEvent* e )
{
    if( K3b::ThreadJobCommunicationEvent* ce = dynamic_cast<K3b::ThreadJobCommunicationEvent*>(e) ) {
        int result = 0;
        switch( ce->type() ) {
        case K3b::ThreadJobCommunicationEvent::WaitForMedium:
            result = K3b::Job::waitForMedia( ce->device(),
                                             ce->wantedMediaState(),
                                             ce->wantedMediaType(),
                                             ce->text() );
            break;

        case K3b::ThreadJobCommunicationEvent::QuestionYesNo:
            result = K3b::Job::questionYesNo( ce->text(),
                                              ce->caption(),
                                              ce->yesText(),
                                              ce->noText() )
                     ? 1 : 0;
            break;

        case K3b::ThreadJobCommunicationEvent::BlockingInfo:
            K3b::Job::blockingInformation( ce->text(), ce->caption() );
            break;
        }
        ce->done( result );
    }
}


void K3b::ThreadJob::waitUntilFinished()
{
    K3b::Thread::waitUntilFinished();
}

#include "k3bthreadjob.moc"
