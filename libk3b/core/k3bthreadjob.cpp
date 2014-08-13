/*
 *
 * Copyright (C) 2003-2010 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2010 Sebastian Trueg <trueg@k3b.org>
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

#include <QtCore/QDebug>
#include <KApplication>

#include <QSharedPointer>
#include <QThread>


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
        qDebug() << "(K3b::ThreadJob) thread not finished yet.";
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


K3b::Device::MediaType K3b::ThreadJob::waitForMedium( K3b::Device::Device* device,
                                                      Device::MediaStates mediaState,
                                                      Device::MediaTypes mediaType,
                                                      const K3b::Msf& minMediaSize,
                                                      const QString& message )
{
    K3b::ThreadJobCommunicationEvent* event = K3b::ThreadJobCommunicationEvent::waitForMedium( device,
                                                                                               mediaState,
                                                                                               mediaType,
                                                                                               minMediaSize,
                                                                                               message );
    QSharedPointer<K3b::ThreadJobCommunicationEvent::Data> data( event->data() );
    QApplication::postEvent( this, event );
    data->wait();
    return (Device::MediaType)data->intResult();
}


bool K3b::ThreadJob::questionYesNo( const QString& text,
                                    const QString& caption,
                                    const KGuiItem& buttonYes,
                                    const KGuiItem& buttonNo )
{
    K3b::ThreadJobCommunicationEvent* event = K3b::ThreadJobCommunicationEvent::questionYesNo( text,
                                                                                               caption,
                                                                                               buttonYes,
                                                                                               buttonNo );
    QSharedPointer<K3b::ThreadJobCommunicationEvent::Data> data( event->data() );
    QApplication::postEvent( this, event );
    data->wait();
    return data->boolResult();
}


void K3b::ThreadJob::blockingInformation( const QString& text,
                                          const QString& caption )
{
    K3b::ThreadJobCommunicationEvent* event = K3b::ThreadJobCommunicationEvent::blockingInformation( text,
                                                                                                     caption );
    QSharedPointer<K3b::ThreadJobCommunicationEvent::Data> data( event->data() );
    QApplication::postEvent( this, event );
    data->wait();
}


void K3b::ThreadJob::customEvent( QEvent* e )
{
    if( K3b::ThreadJobCommunicationEvent* ce = dynamic_cast<K3b::ThreadJobCommunicationEvent*>(e) ) {
        K3b::ThreadJobCommunicationEvent::Data* data = ce->data();
        int result = 0;
        switch( ce->type() ) {
        case K3b::ThreadJobCommunicationEvent::WaitForMedium:
            result = K3b::Job::waitForMedium( data->device(),
                                              data->wantedMediaState(),
                                              data->wantedMediaType(),
                                              data->wantedMediaSize(),
                                              data->text() );
            break;

        case K3b::ThreadJobCommunicationEvent::QuestionYesNo:
            result = K3b::Job::questionYesNo( data->text(),
                                              data->caption(),
                                              data->buttonYes(),
                                              data->buttonNo() )
                     ? 1 : 0;
            break;

        case K3b::ThreadJobCommunicationEvent::BlockingInfo:
            K3b::Job::blockingInformation( data->text(), data->caption() );
            break;
        }
        data->done( result );
    }
}


bool K3b::ThreadJob::wait( unsigned long time )
{
    return d->thread->wait( time );
}


