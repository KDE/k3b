/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3bthread.h"
#include "k3bprogressinfoevent.h"

#include <kdebug.h>

#include <qapplication.h>


K3bThread::K3bThread( QObject* eventHandler )
  : QThread(),
    m_eventHandler( eventHandler )
{
}


K3bThread::K3bThread( unsigned int stackSize, QObject* eventHandler )
  : QThread( stackSize ),
    m_eventHandler( eventHandler )
{
}


K3bThread::~K3bThread()
{
}


void K3bThread::cancel()
{
  terminate();
  if( m_eventHandler ) {
    emitCanceled();
    emitFinished(false);
  }
}


void K3bThread::emitInfoMessage( const QString& msg, int type )
{
  if( m_eventHandler ) 
    qApp->postEvent( m_eventHandler,
		     new K3bProgressInfoEvent( K3bProgressInfoEvent::InfoMessage, msg, QString::null, type ) );
  else
    kdWarning() << "(K3bThread) call to emitInfoMessage() without eventHandler." << endl;
}

void K3bThread::emitPercent( int p )
{
  if( m_eventHandler ) 
    qApp->postEvent( m_eventHandler,
		     new K3bProgressInfoEvent( K3bProgressInfoEvent::Progress, p ) );
  else
    kdWarning() << "(K3bThread) call to emitPercent() without eventHandler." << endl;
}

void K3bThread::emitSubPercent( int p )
{
  if( m_eventHandler ) 
    qApp->postEvent( m_eventHandler,
		     new K3bProgressInfoEvent( K3bProgressInfoEvent::SubProgress, p ) );
  else
    kdWarning() << "(K3bThread) call to emitSubPercent() without eventHandler." << endl;
}

void K3bThread::emitStarted()
{
  if( m_eventHandler )
    qApp->postEvent( m_eventHandler, new K3bProgressInfoEvent( K3bProgressInfoEvent::Started ) );
  else
    kdWarning() << "(K3bThread) call to emitStarted() without eventHandler." << endl;
}

void K3bThread::emitCanceled()
{
  if( m_eventHandler )
    qApp->postEvent( m_eventHandler, new K3bProgressInfoEvent( K3bProgressInfoEvent::Canceled ) );
  else
    kdWarning() << "(K3bThread) call to emitCanceled() without eventHandler." << endl;
}

void K3bThread::emitFinished( bool success )
{
  if( m_eventHandler ) 
    qApp->postEvent( m_eventHandler, new K3bProgressInfoEvent( K3bProgressInfoEvent::Finished, success ) );
  else
    kdWarning() << "(K3bThread) call to emitFinished() without eventHandler." << endl;
}

void K3bThread::emitProcessedSize( int p, int size )
{
  if( m_eventHandler ) 
    qApp->postEvent( m_eventHandler, new K3bProgressInfoEvent( K3bProgressInfoEvent::ProcessedSize, p, size ) );
  else
    kdWarning() << "(K3bThread) call to emitProcessedSize() without eventHandler." << endl;
}

void K3bThread::emitProcessedSubSize( int p, int size )
{
  if( m_eventHandler ) 
    qApp->postEvent( m_eventHandler, new K3bProgressInfoEvent( K3bProgressInfoEvent::ProcessedSubSize, p, size ) );
  else
    kdWarning() << "(K3bThread) call to emitProcessedSubSize() without eventHandler." << endl;
}

void K3bThread::emitNewTask( const QString& job )
{
  if( m_eventHandler ) 
    qApp->postEvent( m_eventHandler, new K3bProgressInfoEvent( K3bProgressInfoEvent::NewTask, job ) );
  else
    kdWarning() << "(K3bThread) call to emitNewTask() without eventHandler." << endl;
}

void K3bThread::emitNewSubTask( const QString& job )
{
  if( m_eventHandler ) 
    qApp->postEvent( m_eventHandler, new K3bProgressInfoEvent( K3bProgressInfoEvent::NewSubTask, job ) );
  else
    kdWarning() << "(K3bThread) call to emitNewSubTask() without eventHandler." << endl;
}

void K3bThread::emitDebuggingOutput(const QString& group, const QString& text)
{
  if( m_eventHandler )
    qApp->postEvent( m_eventHandler, new K3bProgressInfoEvent( K3bProgressInfoEvent::DebuggingOutput, group, text ) );
  else
    kdWarning() << "(K3bThread) call to emitDebuggingOutput() without eventHandler." << endl;
}

void K3bThread::emitWriteSpeed(int s)
{
  if( m_eventHandler )
    qApp->postEvent( m_eventHandler, new K3bProgressInfoEvent( K3bProgressInfoEvent::WriteSpeed, s ) );
  else
    kdWarning() << "(K3bThread) call to emitWriteSpeed() without eventHandler." << endl;
}

void K3bThread::emitBufferStatus(int s)
{
  if( m_eventHandler )
    qApp->postEvent( m_eventHandler, new K3bProgressInfoEvent( K3bProgressInfoEvent::BufferStatus, s ) );
  else
    kdWarning() << "(K3bThread) call to emitBufferStatus() without eventHandler." << endl;
}

