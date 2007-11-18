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


#include "k3bthread.h"
#include "k3bprogressinfoevent.h"
#include "k3bthreadjobcommunicationevent.h"

#include <kdebug.h>

#include <qapplication.h>
//Added by qt3to4:
#include <Q3PtrList>


static Q3PtrList<K3bThread> s_threads;


void K3bThread::waitUntilFinished()
{
  Q3PtrListIterator<K3bThread> it( s_threads );
  while( it.current() ) {
    kDebug() << "Waiting for thread " << it.current();
    it.current()->wait();
    ++it;
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


K3bThread::K3bThread( QObject* eventHandler )
  : QThread()
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
  if( d->eventHandler )
    QApplication::postEvent( d->eventHandler,
			     new K3bProgressInfoEvent( K3bProgressInfoEvent::InfoMessage, msg, QString::null, type ) );
  else
    kWarning() << "(K3bThread) call to emitInfoMessage() without eventHandler.";
}

void K3bThread::emitPercent( int p )
{
  if( d->eventHandler )
    QApplication::postEvent( d->eventHandler,
			     new K3bProgressInfoEvent( K3bProgressInfoEvent::Progress, p ) );
  else
    kWarning() << "(K3bThread) call to emitPercent() without eventHandler.";
}

void K3bThread::emitSubPercent( int p )
{
  if( d->eventHandler )
    QApplication::postEvent( d->eventHandler,
			     new K3bProgressInfoEvent( K3bProgressInfoEvent::SubProgress, p ) );
  else
    kWarning() << "(K3bThread) call to emitSubPercent() without eventHandler.";
}

void K3bThread::emitStarted()
{
  if( d->eventHandler )
    QApplication::postEvent( d->eventHandler, new K3bProgressInfoEvent( K3bProgressInfoEvent::Started ) );
  else
    kWarning() << "(K3bThread) call to emitStarted() without eventHandler.";
}

void K3bThread::emitCanceled()
{
  if( d->eventHandler )
    QApplication::postEvent( d->eventHandler, new K3bProgressInfoEvent( K3bProgressInfoEvent::Canceled ) );
  else
    kWarning() << "(K3bThread) call to emitCanceled() without eventHandler.";
}

void K3bThread::emitFinished( bool success )
{
  if( d->eventHandler )
    QApplication::postEvent( d->eventHandler, new K3bProgressInfoEvent( K3bProgressInfoEvent::Finished, success ) );
  else
    kWarning() << "(K3bThread) call to emitFinished() without eventHandler.";
}

void K3bThread::emitProcessedSize( int p, int size )
{
  if( d->eventHandler )
    QApplication::postEvent( d->eventHandler, new K3bProgressInfoEvent( K3bProgressInfoEvent::ProcessedSize, p, size ) );
  else
    kWarning() << "(K3bThread) call to emitProcessedSize() without eventHandler.";
}

void K3bThread::emitProcessedSubSize( int p, int size )
{
  if( d->eventHandler )
    QApplication::postEvent( d->eventHandler, new K3bProgressInfoEvent( K3bProgressInfoEvent::ProcessedSubSize, p, size ) );
  else
    kWarning() << "(K3bThread) call to emitProcessedSubSize() without eventHandler.";
}

void K3bThread::emitNewTask( const QString& job )
{
  if( d->eventHandler )
    QApplication::postEvent( d->eventHandler, new K3bProgressInfoEvent( K3bProgressInfoEvent::NewTask, job ) );
  else
    kWarning() << "(K3bThread) call to emitNewTask() without eventHandler.";
}

void K3bThread::emitNewSubTask( const QString& job )
{
  if( d->eventHandler )
    QApplication::postEvent( d->eventHandler, new K3bProgressInfoEvent( K3bProgressInfoEvent::NewSubTask, job ) );
  else
    kWarning() << "(K3bThread) call to emitNewSubTask() without eventHandler.";
}

void K3bThread::emitDebuggingOutput(const QString& group, const QString& text)
{
  if( d->eventHandler )
    QApplication::postEvent( d->eventHandler, new K3bProgressInfoEvent( K3bProgressInfoEvent::DebuggingOutput, group, text ) );
  else
    kWarning() << "(K3bThread) call to emitDebuggingOutput() without eventHandler.";
}


void K3bThread::emitNextTrack( int t, int n )
{
  if( d->eventHandler )
    QApplication::postEvent( d->eventHandler, new K3bProgressInfoEvent( K3bProgressInfoEvent::NextTrack, t, n ) );
  else
    kWarning() << "(K3bThread) call to emitNextTrack() without eventHandler.";
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
