/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3bthread.h"
#include "k3bprogressinfoevent.h"
#include "k3bdataevent.h"

#include <kdebug.h>

#include <qapplication.h>


static QPtrList<K3bThread> s_threads;


void K3bThread::waitUntilFinished()
{
  QPtrListIterator<K3bThread> it( s_threads );
  while( it.current() ) {
    it.current()->wait();
    ++it;
  }
}


class K3bThread::Private
{
public:
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

void K3bThread::emitData( const char* data, int len )
{
  if( d->eventHandler )
    QApplication::postEvent( d->eventHandler, new K3bDataEvent( data, len ) );
  else
    kdWarning() << "(K3bThread) call to emitData() without eventHandler." << endl;
}

void K3bThread::emitNextTrack( int t, int n )
{
  if( d->eventHandler ) 
    QApplication::postEvent( d->eventHandler, new K3bProgressInfoEvent( K3bProgressInfoEvent::NextTrack, t, n ) );
  else
    kdWarning() << "(K3bThread) call to emitNextTrack() without eventHandler." << endl;
}

