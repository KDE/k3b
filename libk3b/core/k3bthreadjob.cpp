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



K3bThreadJob::K3bThreadJob( K3bJobHandler* jh, QObject* parent )
  : K3bJob( jh, parent ),
    m_running(false)
{
}


K3bThreadJob::K3bThreadJob( K3bThread* thread, K3bJobHandler* jh, QObject* parent )
  : K3bJob( jh, parent ),
    m_running(false)
{
  setThread(thread);
}


K3bThreadJob::~K3bThreadJob()
{
}


QString K3bThreadJob::jobDescription() const
{
  if( m_thread )
    return m_thread->jobDescription();
  else
    return QString::null;
}


QString K3bThreadJob::jobDetails() const
{
  if( m_thread )
    return m_thread->jobDetails();
  else
    return QString::null;
}


void K3bThreadJob::setThread( K3bThread* t )
{
  m_thread = t;
  m_thread->setProgressInfoEventHandler(this);
}


void K3bThreadJob::start()
{
  if( m_thread ) {
    if( !m_running ) {
      m_thread->setProgressInfoEventHandler(this);
      m_running = true;
      m_thread->init();
      m_thread->start();
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
  m_thread->cancel();
  // wait for the thread to finish
  //  m_thread->wait();
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
            jobStarted();
            break;
        case K3bProgressInfoEvent::Canceled:
            emit canceled();
            break;
        case K3bProgressInfoEvent::Finished:
            // we wait until the thred really finished
            // although this may be dangerous if some thread
            // emits the finished signal although it has not finished yet
            // but makes a lot stuff easier.
            kDebug() << "(K3bThreadJob) waiting for the thread to finish.";
            m_thread->wait();
            kDebug() << "(K3bThreadJob) thread finished.";
            cleanupJob( be->firstValue() );
            m_running = false;
            jobFinished( be->firstValue() );
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
