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

#include "k3bthreadjob.h"
#include "k3bthread.h"
#include <k3bprogressinfoevent.h>
#include <k3bdataevent.h>

#include <kdebug.h>
#include <kapplication.h>



K3bThreadJob::K3bThreadJob( QObject* parent, const char* name )
  : K3bJob( parent, name )
{
}


K3bThreadJob::K3bThreadJob( K3bThread* thread, QObject* parent, const char* name )
  : K3bJob( parent, name ),
    m_thread(thread)
{
}


K3bThreadJob::~K3bThreadJob()
{
}


void K3bThreadJob::setThread( K3bThread* t )
{
  m_thread = t;
  m_thread->setProgressInfoEventHandler(this);
}


void K3bThreadJob::start()
{
  if( m_thread ) {
    m_thread->setProgressInfoEventHandler(this);
    m_thread->start();
  }
  else {
    kdError() << "(K3bThreadJob) no job set." << endl;
    emit finished(false);
  }
}


void K3bThreadJob::cancel()
{
  m_thread->cancel();
}


void K3bThreadJob::customEvent( QCustomEvent* e )
{
  if( e->type() == K3bDataEvent::EVENT_TYPE ) {
    K3bDataEvent* de = static_cast<K3bDataEvent*>(e);
    emit data( de->data(), de->length() );
  }
  else {
    K3bProgressInfoEvent* be = static_cast<K3bProgressInfoEvent*>(e);
    switch( be->type() ) {
    case K3bProgressInfoEvent::Progress:
      emit percent( be->firstValue() );
      break;
    case K3bProgressInfoEvent::SubProgress:
      emit subPercent( be->firstValue() );
      break;
    case K3bProgressInfoEvent::ProcessedSize:
      emit processedSize( be->firstValue(), be->secondValue() );
      break;
    case K3bProgressInfoEvent::ProcessedSubSize:
      emit processedSubSize( be->firstValue(), be->secondValue() );
      break;
    case K3bProgressInfoEvent::InfoMessage:
      emit infoMessage( be->firstString(), be->firstValue() ); 
      break;
    case K3bProgressInfoEvent::Started:
      emit started();
      break;
    case K3bProgressInfoEvent::Canceled:
      emit canceled();
      break;
    case K3bProgressInfoEvent::Finished:
      emit finished( be->firstValue() );
      break;
    case K3bProgressInfoEvent::NewTask:
      emit newTask( be->firstString() );
      break;
    case K3bProgressInfoEvent::NewSubTask:
      emit newSubTask( be->firstString() );
      break;
    case K3bProgressInfoEvent::DebuggingOutput:
      emit debuggingOutput( be->firstString(), be->secondString() );
      break;
    case K3bProgressInfoEvent::NextTrack:
      emit nextTrack( be->firstValue(), be->secondValue() );
      break;
    }
  }
}

#include "k3bthreadjob.moc"
