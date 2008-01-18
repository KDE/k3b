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
//Added by qt3to4:


class K3bThreadJob::Private
{
public:
    Private()
        : thread( 0 ),
          running( false ) {
    }
    K3bThread* thread;
    bool running;
};


K3bThreadJob::K3bThreadJob( K3bJobHandler* jh, QObject* parent )
    : K3bJob( jh, parent ),
      d( new Private )
{
}


K3bThreadJob::K3bThreadJob( K3bThread* thread, K3bJobHandler* jh, QObject* parent )
    : K3bJob( jh, parent ),
      d( new Private )
{
    setThread(thread);
}


K3bThreadJob::~K3bThreadJob()
{
    delete d;
}


QString K3bThreadJob::jobDescription() const
{
    if( d->thread )
        return d->thread->jobDescription();
    else
        return QString::null;
}


QString K3bThreadJob::jobDetails() const
{
    if( d->thread )
        return d->thread->jobDetails();
    else
        return QString::null;
}


bool K3bThreadJob::active() const
{
    return d->running;
}


K3bThread* K3bThreadJob::thread() const
{
    return d->thread;
}


void K3bThreadJob::setThread( K3bThread* t )
{
    if ( d->thread ) {
        d->thread->disconnect( this );
    }

    d->thread = t;
    d->thread->setProgressInfoEventHandler(this);

    connect( d->thread, SIGNAL(infoMessage( const QString&, int )),
             this, SIGNAL(infoMessage( const QString&, int )) );
    connect( d->thread, SIGNAL(percent( int )),
             this, SIGNAL(percent( int )) );
    connect( d->thread, SIGNAL(subPercent( int )),
             this, SIGNAL(subPercent( int )) );
    connect( d->thread, SIGNAL(processedSize( int, int )),
             this, SIGNAL(processedSize( int, int )) );
    connect( d->thread, SIGNAL(processedSubSize( int, int )),
             this, SIGNAL(processedSubSize( int, int )) );
    connect( d->thread, SIGNAL(newTask( const QString& )),
             this, SIGNAL(newTask( const QString& )) );
    connect( d->thread, SIGNAL(newSubTask( const QString& )),
             this, SIGNAL(newSubTask( const QString& )) );
    connect( d->thread, SIGNAL(debuggingOutput(const QString&, const QString&)),
             this, SIGNAL(debuggingOutput(const QString&, const QString&)) );
    connect( d->thread, SIGNAL(nextTrack( int, int )),
             this, SIGNAL(nextTrack( int, int )) );
    connect( d->thread, SIGNAL(canceled()),
             this, SIGNAL(canceled()) );
    connect( d->thread, SIGNAL(started()),
             this, SIGNAL(started()) );
    connect( d->thread, SIGNAL(finished( bool )),
             this, SIGNAL(finished( bool )) );
}


void K3bThreadJob::start()
{
    if( d->thread ) {
        if( !d->running ) {
            d->thread->setProgressInfoEventHandler(this);
            d->running = true;
            d->thread->init();
            d->thread->start();
        }
        else
            kDebug() << "(K3bThreadJob) thread not finished yet.";
    }
    else {
        kError() << "(K3bThreadJob) no job set." << endl;
        jobFinished(false);
    }
}


void K3bThreadJob::cancel()
{
    d->thread->cancel();
    // wait for the thread to finish
    //  d->thread->wait();
}


void K3bThreadJob::cleanupJob( bool success )
{
    Q_UNUSED( success );
}


void K3bThreadJob::customEvent( QEvent* e )
{
    if( K3bThreadJobCommunicationEvent* ce = dynamic_cast<K3bThreadJobCommunicationEvent*>(e) ) {
        int result = 0;
        switch( ce->type() ) {
        case K3bThreadJobCommunicationEvent::WaitForMedium:
            result = waitForMedia( ce->device(),
                                   ce->wantedMediaState(),
                                   ce->wantedMediaType(),
                                   ce->text() );
            break;
        case K3bThreadJobCommunicationEvent::QuestionYesNo:
            result = questionYesNo( ce->text(),
                                    ce->caption(),
                                    ce->yesText(),
                                    ce->noText() )
                     ? 1 : 0;
            break;
        case K3bThreadJobCommunicationEvent::BlockingInfo:
            blockingInformation( ce->text(), ce->caption() );
            break;
        }
        ce->done( result );
    }
}

#include "k3bthreadjob.moc"
