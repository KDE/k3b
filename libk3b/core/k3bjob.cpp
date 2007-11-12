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


#include "k3bjob.h"
#include <k3bglobals.h>
#include <k3bcore.h>

#include <klocale.h>
#include <k3process.h>

#include <qstringlist.h>
#include <kdebug.h>


class K3bJob::Private
{
public:
};


const char* K3bJob::DEFAULT_SIGNAL_CONNECTION = "K3bJobDefault";


K3bJob::K3bJob( K3bJobHandler* handler, QObject* parent )
  : QObject( parent ),
    m_jobHandler( handler ),
    m_canceled(false),
    m_active(false)
{
  connect( this, SIGNAL(canceled()),
	   this, SLOT(slotCanceled()) );
}

K3bJob::~K3bJob()
{
  //
  // Normally a job (or the user of a job should take care of this
  // but we do this here for security reasons.
  //
  if( m_active )
    jobFinished( false );
}


void K3bJob::setJobHandler( K3bJobHandler* jh )
{
  m_jobHandler = jh;
}


void K3bJob::jobStarted()
{
  m_canceled = false;
  m_active = true;

  if( jobHandler() && jobHandler()->isJob() )
    static_cast<K3bJob*>(jobHandler())->registerSubJob( this );
  else
    k3bcore->registerJob( this );

  emit started();
}


void K3bJob::jobFinished( bool success )
{
  m_active = false;

  if( jobHandler() && jobHandler()->isJob() )
    static_cast<K3bJob*>(jobHandler())->unregisterSubJob( this );
  else
    k3bcore->unregisterJob( this );

  emit finished( success );
}


void K3bJob::slotCanceled()
{
  m_canceled = true;
}


int K3bJob::waitForMedia( K3bDevice::Device* device,
			  int mediaState,
			  int mediaType,
			  const QString& message )
{
  // TODO: What about:   emit newSubTask( i18n("Waiting for media") );
  return m_jobHandler->waitForMedia( device, mediaState, mediaType, message );
}


bool K3bJob::questionYesNo( const QString& text,
			    const QString& caption,
			    const QString& yesText,
			    const QString& noText )
{
  return m_jobHandler->questionYesNo( text, caption, yesText, noText );
}


void K3bJob::blockingInformation( const QString& text,
				  const QString& caption )
{
  return m_jobHandler->blockingInformation( text, caption );
}


void K3bJob::connectSubJob( K3bJob* subJob,
			    const char* finishedSlot,
			    bool connectProgress,
			    const char* progressSlot,
			    const char* subProgressSlot,
			    const char* processedSizeSlot,
			    const char* processedSubSizeSlot )
{
  connect( subJob, SIGNAL(newTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
  connect( subJob, SIGNAL(newSubTask(const QString&)), this, SLOT(slotNewSubTask(const QString&)) );
  connect( subJob, SIGNAL(debuggingOutput(const QString&, const QString&)),
	   this, SIGNAL(debuggingOutput(const QString&, const QString&)) );
  connect( subJob, SIGNAL(infoMessage(const QString&, int)),
	   this, SIGNAL(infoMessage(const QString&, int)) );
  connect( subJob, SIGNAL(finished(bool)), this, finishedSlot );

  if( connectProgress ) {
    connect( subJob, SIGNAL(percent(int)),
	       this, progressSlot != 0 ? progressSlot : SIGNAL(subPercent(int)) );
    if( subProgressSlot )
      connect( subJob, SIGNAL(subPercent(int)), this, subProgressSlot );
    connect( subJob, SIGNAL(processedSize(int, int)),
	     this, processedSizeSlot != 0 ? processedSizeSlot : SIGNAL(processedSubSize(int, int)) );
    if( processedSubSizeSlot )
      connect( subJob, SIGNAL(processedSubSize(int, int)), this, processedSubSizeSlot );
  }
}


void K3bJob::connectSubJob( K3bJob* subJob,
			    const char* finishedSlot,
			    const char* newTaskSlot,
			    const char* newSubTaskSlot,
			    const char* progressSlot,
			    const char* subProgressSlot,
			    const char* processedSizeSlot,
			    const char* processedSubSizeSlot )
{
  // standard connections
  connect( subJob, SIGNAL(debuggingOutput(const QString&, const QString&)),
	   this, SIGNAL(debuggingOutput(const QString&, const QString&)) );
  connect( subJob, SIGNAL(infoMessage(const QString&, int)),
	   this, SIGNAL(infoMessage(const QString&, int)) );

  // task connections
  if( newTaskSlot == DEFAULT_SIGNAL_CONNECTION )
    connect( subJob, SIGNAL(newTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
  else if( newTaskSlot )
    connect( subJob, SIGNAL(newTask(const QString&)), this, newTaskSlot );

  if( newSubTaskSlot == DEFAULT_SIGNAL_CONNECTION )
    connect( subJob, SIGNAL(newSubTask(const QString&)), this, SLOT(slotNewSubTask(const QString&)) );
  else if( newSubTaskSlot )
    connect( subJob, SIGNAL(newSubTask(const QString&)), this, newSubTaskSlot );

  if( finishedSlot && finishedSlot != DEFAULT_SIGNAL_CONNECTION )
    connect( subJob, SIGNAL(finished(bool)), this, finishedSlot );

  // progress
  if( progressSlot == DEFAULT_SIGNAL_CONNECTION )
    connect( subJob, SIGNAL(percent(int)), this, SIGNAL(subPercent(int)) );
  else if( progressSlot )
    connect( subJob, SIGNAL(percent(int)), this, progressSlot );

  if( subProgressSlot && subProgressSlot != DEFAULT_SIGNAL_CONNECTION )
    connect( subJob, SIGNAL(subPercent(int)), this, subProgressSlot );

  // processed size
  if( processedSizeSlot == DEFAULT_SIGNAL_CONNECTION )
    connect( subJob, SIGNAL(processedSize(int, int)), this, SIGNAL(processedSubSize(int, int)) );
  else if( processedSizeSlot )
    connect( subJob, SIGNAL(processedSize(int, int)), this, processedSizeSlot );

  if( processedSubSizeSlot && processedSubSizeSlot != DEFAULT_SIGNAL_CONNECTION )
    connect( subJob, SIGNAL(processedSubSize(int, int)), this, processedSubSizeSlot );
}


unsigned int K3bJob::numRunningSubJobs() const
{
  return m_runningSubJobs.count();
}


void K3bJob::slotNewSubTask( const QString& str )
{
  emit infoMessage( str, INFO );
}


void K3bJob::registerSubJob( K3bJob* job )
{
  m_runningSubJobs.append( job );
}


void K3bJob::unregisterSubJob( K3bJob* job )
{
  m_runningSubJobs.removeRef( job );
}




class K3bBurnJob::Private
{
public:
};



K3bBurnJob::K3bBurnJob( K3bJobHandler* handler, QObject* parent )
  : K3bJob( handler, parent ),
    m_writeMethod( K3b::DEFAULT )
{
  d = new Private;
}


K3bBurnJob::~K3bBurnJob()
{
  delete d;
}


int K3bBurnJob::supportedWritingApps() const
{
  return K3b::DEFAULT | K3b::CDRDAO | K3b::CDRECORD;
}

#include "k3bjob.moc"
