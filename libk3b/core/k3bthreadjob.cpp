/*
 *
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bthreadjob.h"
#include "k3bthread.h"
#include "k3bprogressinfoevent.h"
#include "k3bthreadjobcommunicationevent.h"

#include <kdebug.h>
#include <kapplication.h>


class K3bThreadJob::Private
{
public:
    Private()
        : thread( 0 ),
          running( false ),
          canceled( false ) {
    }
    K3bThread* thread;
    bool running;
    bool canceled;
};


K3bThreadJob::K3bThreadJob( K3bJobHandler* jh, QObject* parent )
    : K3bJob( jh, parent ),
      d( new Private )
{
    d->thread = new K3bThread( this );
    connect( d->thread, SIGNAL(finished()),
             this, SLOT(slotThreadFinished()) );
}


K3bThreadJob::~K3bThreadJob()
{
    delete d;
}


bool K3bThreadJob::active() const
{
    return d->running;
}


void K3bThreadJob::start()
{
    if( !d->running ) {
        d->canceled = false;
        d->running = true;
        jobStarted();
        d->thread->start();
    }
    else {
        kDebug() << "(K3bThreadJob) thread not finished yet.";
    }
}


void K3bThreadJob::slotThreadFinished()
{
    d->running = false;
    if( canceled() )
        emit canceled();
    jobFinished( d->thread->success() );
}


void K3bThreadJob::cancel()
{
    d->canceled = true;
    d->thread->ensureDone();
}


bool K3bThreadJob::canceled() const
{
    return d->canceled;
}


int K3bThreadJob::waitForMedia( K3bDevice::Device* device,
                                int mediaState,
                                int mediaType,
                                const QString& message )
{
    K3bThreadJobCommunicationEvent* event = K3bThreadJobCommunicationEvent::waitForMedium( device,
                                                                                           mediaState,
                                                                                           mediaType,
                                                                                           message );
    QApplication::postEvent( this, event );
    event->wait();
    return event->intResult();
}


bool K3bThreadJob::questionYesNo( const QString& text,
                                  const QString& caption,
                                  const QString& yesText,
                                  const QString& noText )
{
    K3bThreadJobCommunicationEvent* event = K3bThreadJobCommunicationEvent::questionYesNo( text,
                                                                                           caption,
                                                                                           yesText,
                                                                                           noText );
    QApplication::postEvent( this, event );
    event->wait();
    return event->boolResult();
}


void K3bThreadJob::blockingInformation( const QString& text,
                                        const QString& caption )
{
    K3bThreadJobCommunicationEvent* event = K3bThreadJobCommunicationEvent::blockingInformation( text,
                                                                                                 caption );
    QApplication::postEvent( this, event );
    event->wait();
}


void K3bThreadJob::customEvent( QEvent* e )
{
    if( K3bThreadJobCommunicationEvent* ce = dynamic_cast<K3bThreadJobCommunicationEvent*>(e) ) {
        int result = 0;
        switch( ce->type() ) {
        case K3bThreadJobCommunicationEvent::WaitForMedium:
            result = K3bJob::waitForMedia( ce->device(),
                                           ce->wantedMediaState(),
                                           ce->wantedMediaType(),
                                           ce->text() );
            break;

        case K3bThreadJobCommunicationEvent::QuestionYesNo:
            result = K3bJob::questionYesNo( ce->text(),
                                            ce->caption(),
                                            ce->yesText(),
                                            ce->noText() )
                     ? 1 : 0;
            break;

        case K3bThreadJobCommunicationEvent::BlockingInfo:
            K3bJob::blockingInformation( ce->text(), ce->caption() );
            break;
        }
        ce->done( result );
    }
}


void K3bThreadJob::waitUntilFinished()
{
    K3bThread::waitUntilFinished();
}

#include "k3bthreadjob.moc"
