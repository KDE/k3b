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
#include "k3bprogressinfoevent.h"
#include "k3bthreadjobcommunicationevent.h"

#include <kdebug.h>

#include <QApplication>
#include <QList>


static QList<K3bThread*> s_threads;


void K3bThread::waitUntilFinished()
{
    foreach( K3bThread* thread, s_threads ) {
        kDebug() << "Waiting for thread " << thread;
        thread->wait();
    }

    kDebug() << "Thread waiting done.";
}


class K3bThread::Private
{
public:
    Private()
        : eventHandler( 0 ) {
    }

    QObject* eventHandler;
};


K3bThread::K3bThread( QObject* eventHandler, QObject* parent )
    : QThread( parent )
{
    d = new Private;
    d->eventHandler = eventHandler;

    s_threads.append(this);
}


K3bThread::~K3bThread()
{
    s_threads.removeAll(this);
    delete d;
}


void K3bThread::setProgressInfoEventHandler( QObject* eventHandler )
{
    d->eventHandler = eventHandler;
}

QString K3bThread::jobDescription() const
{
    return QString::null;
}


QString K3bThread::jobDetails() const
{
    return QString::null;
}


void K3bThread::init()
{
    // do nothing...
}


void K3bThread::cancel()
{
    if( isRunning() ) {
        terminate();
        if( d->eventHandler ) {
            emitCanceled();
            emitFinished(false);
        }
    }
}


void K3bThread::emitInfoMessage( const QString& msg, int type )
{
    emit infoMessage( msg, type );
}

void K3bThread::emitPercent( int p )
{
    emit percent( p );
}

void K3bThread::emitSubPercent( int p )
{
    emit subPercent( p );
}

void K3bThread::emitStarted()
{
    emit started();
}

void K3bThread::emitCanceled()
{
    emit canceled();
}

void K3bThread::emitFinished( bool success )
{
    emit finished( success );
}

void K3bThread::emitProcessedSize( int p, int size )
{
    emit processedSize( p, size );
}

void K3bThread::emitProcessedSubSize( int p, int size )
{
    emit processedSubSize( p, size );
}

void K3bThread::emitNewTask( const QString& job )
{
    emit newTask( job );
}

void K3bThread::emitNewSubTask( const QString& job )
{
    emit newSubTask( job );
}

void K3bThread::emitDebuggingOutput(const QString& group, const QString& text)
{
    emit debuggingOutput( group, text );
}


void K3bThread::emitNextTrack( int t, int n )
{
    emit nextTrack( t, n );
}


int K3bThread::waitForMedia( K3bDevice::Device* device,
                             int mediaState,
                             int mediaType,
                             const QString& message )
{
    if( d->eventHandler ) {
        K3bThreadJobCommunicationEvent* event = K3bThreadJobCommunicationEvent::waitForMedium( device,
                                                                                               mediaState,
                                                                                               mediaType,
                                                                                               message );
        QApplication::postEvent( d->eventHandler, event );
        event->wait();
        return event->intResult();
    }
    else {
        kWarning() << "(K3bThread) call to waitForMedium() without eventHandler.";
        return 0;
    }
}


bool K3bThread::questionYesNo( const QString& text,
                               const QString& caption,
                               const QString& yesText,
                               const QString& noText )
{
    if( d->eventHandler ) {
        K3bThreadJobCommunicationEvent* event = K3bThreadJobCommunicationEvent::questionYesNo( text,
                                                                                               caption,
                                                                                               yesText,
                                                                                               noText );
        QApplication::postEvent( d->eventHandler, event );
        event->wait();
        return event->boolResult();
    }
    else {
        kWarning() << "(K3bThread) call to questionYesNo() without eventHandler.";
        return false;
    }
}


void K3bThread::blockingInformation( const QString& text,
                                     const QString& caption )
{
    if( d->eventHandler ) {
        K3bThreadJobCommunicationEvent* event = K3bThreadJobCommunicationEvent::blockingInformation( text,
                                                                                                     caption );
        QApplication::postEvent( d->eventHandler, event );
        event->wait();
    }
    else {
        kWarning() << "(K3bThread) call to blockingInformation() without eventHandler.";
    }
}

#include "k3bthread.moc"
