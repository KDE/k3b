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


#include "k3bjob.h"
#include "k3bglobals.h"
#include "k3bcore.h"
#include "k3b_i18n.h"

#include <QtCore/QDebug>
#include <QtCore/QEventLoop>
#include <QtCore/QStringList>



class K3b::Job::Private
{
public:
    K3b::JobHandler* jobHandler;
    QList<K3b::Job*> runningSubJobs;

    bool canceled;
    bool active;

    QList<QEventLoop*> waitLoops;
};


const char K3b::Job::DEFAULT_SIGNAL_CONNECTION[] = "K3b::JobDefault";


K3b::Job::Job( K3b::JobHandler* handler, QObject* parent )
    : QObject( parent ),
      d( new Private() )
{
    d->jobHandler = handler;
    d->canceled = false;
    d->active = false;

    connect( this, SIGNAL(canceled()),
             this, SLOT(slotCanceled()) );
}

K3b::Job::~Job()
{
    //
    // Normally a job (or the user of a job should take care of this
    // but we do this here for security reasons.
    //
    if( d->active ) {
        qDebug() << "Finishing job in destuctor! This is NOT good. Fix the job.";
        jobFinished( false );
    }

    delete d;
}


K3b::JobHandler* K3b::Job::jobHandler() const
{
    return d->jobHandler;
}


bool K3b::Job::active() const
{
    return d->active;
}


bool K3b::Job::hasBeenCanceled() const
{
    return d->canceled;
}


QList<K3b::Job*> K3b::Job::runningSubJobs() const
{
    return d->runningSubJobs;
}


void K3b::Job::setJobHandler( K3b::JobHandler* jh )
{
    d->jobHandler = jh;
}


void K3b::Job::jobStarted()
{
    d->canceled = false;
    d->active = true;

    if( jobHandler() && jobHandler()->isJob() )
        static_cast<K3b::Job*>(jobHandler())->registerSubJob( this );
    else
        k3bcore->registerJob( this );

    emit started();
}


void K3b::Job::jobFinished( bool success )
{
    d->active = false;

    if( jobHandler() && jobHandler()->isJob() )
        static_cast<K3b::Job*>(jobHandler())->unregisterSubJob( this );
    else
        k3bcore->unregisterJob( this );

    foreach( QEventLoop* loop, d->waitLoops ) {
        loop->exit();
    }

    emit finished( success );
}


void K3b::Job::slotCanceled()
{
    d->canceled = true;
}


K3b::Device::MediaType K3b::Job::waitForMedium( K3b::Device::Device* device,
                                                Device::MediaStates mediaState,
                                                Device::MediaTypes mediaType,
                                                const K3b::Msf& minMediaSize,
                                                const QString& message )
{
    // TODO: What about:   emit newSubTask( i18n("Waiting for media") );
    return d->jobHandler->waitForMedium( device, mediaState, mediaType, minMediaSize, message );
}


bool K3b::Job::questionYesNo( const QString& text,
                              const QString& caption,
                              const KGuiItem& buttonYes,
                              const KGuiItem& buttonNo )
{
    return d->jobHandler->questionYesNo( text, caption, buttonYes, buttonNo );
}


void K3b::Job::blockingInformation( const QString& text,
                                    const QString& caption )
{
    return d->jobHandler->blockingInformation( text, caption );
}

void K3b::Job::connectJob( K3b::Job* subJob,
                           const char* finishedSlot,
                           const char* newTaskSlot,
                           const char* newSubTaskSlot,
                           const char* progressSlot,
                           const char* subProgressSlot,
                           const char* processedSizeSlot,
                           const char* processedSubSizeSlot )
{
    // standard connections
    connect( subJob, SIGNAL(debuggingOutput(QString,QString)),
             this, SIGNAL(debuggingOutput(QString,QString)) );
    connect( subJob, SIGNAL(infoMessage(QString,int)),
             this, SIGNAL(infoMessage(QString,int)) );

    // task connections
    if( newTaskSlot == DEFAULT_SIGNAL_CONNECTION )
        connect( subJob, SIGNAL(newTask(QString)), this, SIGNAL(newTask(QString)) );
    else if( newTaskSlot )
        connect( subJob, SIGNAL(newTask(QString)), this, newTaskSlot );

    if( newSubTaskSlot == DEFAULT_SIGNAL_CONNECTION )
        connect( subJob, SIGNAL(newSubTask(QString)), this, SIGNAL(newSubTask(QString)) );
    else if( newSubTaskSlot )
        connect( subJob, SIGNAL(newSubTask(QString)), this, newSubTaskSlot );

    if( finishedSlot == DEFAULT_SIGNAL_CONNECTION )
        connect( subJob, SIGNAL(finished(bool)), this, SIGNAL(finished(bool)) );
    else if( finishedSlot )
        connect( subJob, SIGNAL(finished(bool)), this, finishedSlot );

    // progress
    if( progressSlot == DEFAULT_SIGNAL_CONNECTION )
        connect( subJob, SIGNAL(percent(int)), this, SIGNAL(percent(int)) );
    else if( progressSlot )
        connect( subJob, SIGNAL(percent(int)), this, progressSlot );

    if( subProgressSlot == DEFAULT_SIGNAL_CONNECTION )
        connect( subJob, SIGNAL(subPercent(int)), this, SIGNAL(subPercent(int)) );
    else if( subProgressSlot )
        connect( subJob, SIGNAL(subPercent(int)), this, subProgressSlot );

    // processed size
    if( processedSizeSlot == DEFAULT_SIGNAL_CONNECTION )
        connect( subJob, SIGNAL(processedSize(int,int)), this, SIGNAL(processedSize(int,int)) );
    else if( processedSizeSlot )
        connect( subJob, SIGNAL(processedSize(int,int)), this, processedSizeSlot );

    if( processedSubSizeSlot == DEFAULT_SIGNAL_CONNECTION )
        connect( subJob, SIGNAL(processedSubSize(int,int)), this, SIGNAL(processedSubSize(int,int)) );
    else if( processedSubSizeSlot )
        connect( subJob, SIGNAL(processedSubSize(int,int)), this, processedSubSizeSlot );
}


void K3b::Job::connectSubJob( K3b::Job* subJob,
                              const char* finishedSlot,
                              const char* newTaskSlot,
                              const char* newSubTaskSlot,
                              const char* progressSlot,
                              const char* subProgressSlot,
                              const char* processedSizeSlot,
                              const char* processedSubSizeSlot )
{
    // standard connections
    connect( subJob, SIGNAL(debuggingOutput(QString,QString)),
             this, SIGNAL(debuggingOutput(QString,QString)) );
    connect( subJob, SIGNAL(infoMessage(QString,int)),
             this, SIGNAL(infoMessage(QString,int)) );

    // task connections
    if( newTaskSlot == DEFAULT_SIGNAL_CONNECTION )
        connect( subJob, SIGNAL(newTask(QString)), this, SIGNAL(newSubTask(QString)) );
    else if( newTaskSlot )
        connect( subJob, SIGNAL(newTask(QString)), this, newTaskSlot );

    if( newSubTaskSlot == DEFAULT_SIGNAL_CONNECTION )
        connect( subJob, SIGNAL(newSubTask(QString)), this, SLOT(slotNewSubTask(QString)) );
    else if( newSubTaskSlot )
        connect( subJob, SIGNAL(newSubTask(QString)), this, newSubTaskSlot );

    if( finishedSlot == DEFAULT_SIGNAL_CONNECTION )
        connect( subJob, SIGNAL(finished(bool)), this, SIGNAL(finished(bool)) );
    else if( finishedSlot )
        connect( subJob, SIGNAL(finished(bool)), this, finishedSlot );

    // progress
    if( progressSlot == DEFAULT_SIGNAL_CONNECTION )
        connect( subJob, SIGNAL(percent(int)), this, SIGNAL(subPercent(int)) );
    else if( progressSlot )
        connect( subJob, SIGNAL(percent(int)), this, progressSlot );

    if( subProgressSlot != DEFAULT_SIGNAL_CONNECTION )
        connect( subJob, SIGNAL(subPercent(int)), this, subProgressSlot );

    // processed size
    if( processedSizeSlot == DEFAULT_SIGNAL_CONNECTION )
        connect( subJob, SIGNAL(processedSize(int,int)), this, SIGNAL(processedSubSize(int,int)) );
    else if( processedSizeSlot )
        connect( subJob, SIGNAL(processedSize(int,int)), this, processedSizeSlot );

    if( processedSubSizeSlot != DEFAULT_SIGNAL_CONNECTION )
        connect( subJob, SIGNAL(processedSubSize(int,int)), this, processedSubSizeSlot );
}


int K3b::Job::numRunningSubJobs() const
{
    return d->runningSubJobs.count();
}


void K3b::Job::slotNewSubTask( const QString& str )
{
    emit infoMessage( str, MessageInfo );
}


void K3b::Job::registerSubJob( K3b::Job* job )
{
    d->runningSubJobs.append( job );
}


void K3b::Job::unregisterSubJob( K3b::Job* job )
{
    d->runningSubJobs.removeOne( job );
}


void K3b::Job::wait()
{
    if ( active() ) {
        QEventLoop loop;
        d->waitLoops.append( &loop );
        loop.exec();
        d->waitLoops.removeOne( &loop );
    }
}




class K3b::BurnJob::Private
{
public:
    K3b::WritingApp writeMethod;
};



K3b::BurnJob::BurnJob( K3b::JobHandler* handler, QObject* parent )
    : K3b::Job( handler, parent ),
      d( new Private() )
{
    d->writeMethod = K3b::WritingAppAuto;
}


K3b::BurnJob::~BurnJob()
{
    delete d;
}


K3b::WritingApp K3b::BurnJob::writingApp() const
{
    return d->writeMethod;
}


void K3b::BurnJob::setWritingApp( K3b::WritingApp w )
{
    d->writeMethod = w;
}


K3b::WritingApps K3b::BurnJob::supportedWritingApps() const
{
    return K3b::WritingAppAuto | K3b::WritingAppCdrdao | K3b::WritingAppCdrecord;
}


