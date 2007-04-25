/*
 *
 * $Id$
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


#include "k3bthread.h"
#include "k3bprogressinfoevent.h"
#include "k3bthreadjobcommunicationevent.h"

#include <kdebug.h>

#include <qapplication.h>


static QPtrList<K3bThread> s_threads;


void K3bThread::waitUntilFinished()
{
  QPtrListIterator<K3bThread> it( s_threads );
  while( it.current() ) {
    kdDebug() << "Waiting for thread " << it.current() << endl;
    it.current()->wait();
    ++it;
  }

  kdDebug() << "Thread waiting done." << endl;
}


class K3bThread::Private
{
public:
  Private()
    : eventHandler( 0 ) {
  }

  QObject* eventHandler;
};


K3bThread::K3bThread( QObject* eventHandler )
  : QThread()
{
  d = new Private;
  d->eventHandler = eventHandler;

  s_threads.append(this);
}


K3bThread::K3bThread( unsigned int stackSize, QObject* eventHandler )
  : QThread( stackSize )
{
  d = new Private;
  d->eventHandler = eventHandler;

  s_threads.append(this);
}


K3bThread::~K3bThread()
{
  s_threads.removeRef(this);
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
  if( running() ) {
    terminate();
    if( d->eventHandler ) {
      emitCanceled();
      emitFinished(false);
    }
  }
}


void K3bThread::emitInfoMessage( const QString& msg, int type )
{
  if( d->eventHandler )
    QApplication::postEvent( d->eventHandler,
			     new K3bProgressInfoEvent( K3bProgressInfoEvent::InfoMessage, msg, QString::null, type ) );
  else
    kdWarning() << "(K3bThread) call to emitInfoMessage() without eventHandler." << endl;
}

void K3bThread::emitPercent( int p )
{
  if( d->eventHandler )
    QApplication::postEvent( d->eventHandler,
			     new K3bProgressInfoEvent( K3bProgressInfoEvent::Progress, p ) );
  else
    kdWarning() << "(K3bThread) call to emitPercent() without eventHandler." << endl;
}

void K3bThread::emitSubPercent( int p )
{
  if( d->eventHandler )
    QApplication::postEvent( d->eventHandler,
			     new K3bProgressInfoEvent( K3bProgressInfoEvent::SubProgress, p ) );
  else
    kdWarning() << "(K3bThread) call to emitSubPercent() without eventHandler." << endl;
}

void K3bThread::emitStarted()
{
  if( d->eventHandler )
    QApplication::postEvent( d->eventHandler, new K3bProgressInfoEvent( K3bProgressInfoEvent::Started ) );
  else
    kdWarning() << "(K3bThread) call to emitStarted() without eventHandler." << endl;
}

void K3bThread::emitCanceled()
{
  if( d->eventHandler )
    QApplication::postEvent( d->eventHandler, new K3bProgressInfoEvent( K3bProgressInfoEvent::Canceled ) );
  else
    kdWarning() << "(K3bThread) call to emitCanceled() without eventHandler." << endl;
}

void K3bThread::emitFinished( bool success )
{
  if( d->eventHandler )
    QApplication::postEvent( d->eventHandler, new K3bProgressInfoEvent( K3bProgressInfoEvent::Finished, success ) );
  else
    kdWarning() << "(K3bThread) call to emitFinished() without eventHandler." << endl;
}

void K3bThread::emitProcessedSize( int p, int size )
{
  if( d->eventHandler )
    QApplication::postEvent( d->eventHandler, new K3bProgressInfoEvent( K3bProgressInfoEvent::ProcessedSize, p, size ) );
  else
    kdWarning() << "(K3bThread) call to emitProcessedSize() without eventHandler." << endl;
}

void K3bThread::emitProcessedSubSize( int p, int size )
{
  if( d->eventHandler )
    QApplication::postEvent( d->eventHandler, new K3bProgressInfoEvent( K3bProgressInfoEvent::ProcessedSubSize, p, size ) );
  else
    kdWarning() << "(K3bThread) call to emitProcessedSubSize() without eventHandler." << endl;
}

void K3bThread::emitNewTask( const QString& job )
{
  if( d->eventHandler )
    QApplication::postEvent( d->eventHandler, new K3bProgressInfoEvent( K3bProgressInfoEvent::NewTask, job ) );
  else
    kdWarning() << "(K3bThread) call to emitNewTask() without eventHandler." << endl;
}

void K3bThread::emitNewSubTask( const QString& job )
{
  if( d->eventHandler )
    QApplication::postEvent( d->eventHandler, new K3bProgressInfoEvent( K3bProgressInfoEvent::NewSubTask, job ) );
  else
    kdWarning() << "(K3bThread) call to emitNewSubTask() without eventHandler." << endl;
}

void K3bThread::emitDebuggingOutput(const QString& group, const QString& text)
{
  if( d->eventHandler )
    QApplication::postEvent( d->eventHandler, new K3bProgressInfoEvent( K3bProgressInfoEvent::DebuggingOutput, group, text ) );
  else
    kdWarning() << "(K3bThread) call to emitDebuggingOutput() without eventHandler." << endl;
}


void K3bThread::emitNextTrack( int t, int n )
{
  if( d->eventHandler )
    QApplication::postEvent( d->eventHandler, new K3bProgressInfoEvent( K3bProgressInfoEvent::NextTrack, t, n ) );
  else
    kdWarning() << "(K3bThread) call to emitNextTrack() without eventHandler." << endl;
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
        kdWarning() << "(K3bThread) call to waitForMedium() without eventHandler." << endl;
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
        kdWarning() << "(K3bThread) call to questionYesNo() without eventHandler." << endl;
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
        kdWarning() << "(K3bThread) call to blockingInformation() without eventHandler." << endl;
    }
}
