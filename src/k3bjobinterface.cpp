/*
 *
 * Copyright (C) 2006 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2010 Michal Malek <michalm@jabster.pl>
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

#include "k3bjobinterface.h"
#include "k3bjobinterfaceadaptor.h"
#include "k3bjob.h"

#include <QDBusConnection>
#include <QDataStream>

namespace K3b {


JobInterface::JobInterface( Job* job )
:
    QObject( job ),
    m_job( job ),
    m_lastProgress( 0 ),
    m_lastSubProgress( 0 )
{
    if( m_job ) {
        connect( m_job, SIGNAL(newTask(QString)), this, SIGNAL(newTask(QString)) );
        connect( m_job, SIGNAL(newSubTask(QString)), this, SIGNAL(newSubTask(QString)) );
        connect( m_job, SIGNAL(infoMessage(QString,int)), this, SIGNAL(infoMessage(QString,int)) );
        connect( m_job, SIGNAL(finished(bool)), this, SIGNAL(finished(bool)) );
        connect( m_job, SIGNAL(started()), this, SIGNAL(started()) );
        connect( m_job, SIGNAL(canceled()), this, SIGNAL(canceled()) );
        connect( m_job, SIGNAL(percent(int)), this, SLOT(slotProgress(int)) );
        connect( m_job, SIGNAL(subPercent(int)), this, SLOT(slotSubProgress(int)) );
        connect( m_job, SIGNAL(nextTrack(int,int)), this, SIGNAL(nextTrack(int,int)) );

        if( m_job->inherits( "K3b::BurnJob" ) ) {
            connect( m_job, SIGNAL(bufferStatus(int)), this, SIGNAL(buffer(int)) );
            connect( m_job, SIGNAL(deviceBuffer(int)), this, SIGNAL(deviceBuffer(int)) );
        }
    }

    new K3bJobInterfaceAdaptor( this );
    QDBusConnection::sessionBus().registerObject( "/job", this );
}


JobInterface::~JobInterface()
{
    QDBusConnection::sessionBus().unregisterObject( "/job" );
}


bool JobInterface::jobRunning() const
{
    return ( m_job && m_job->active() );
}


QString JobInterface::jobDescription() const
{
    if( m_job )
        return m_job->jobDescription();
    else
        return QString();
}


QString JobInterface::jobDetails() const
{
    if( m_job )
        return m_job->jobDetails();
    else
        return QString();
}


void JobInterface::slotProgress( int val )
{
    if( m_lastProgress != val )
        Q_EMIT progress( val );
}


void JobInterface::slotSubProgress( int val )
{
    if( m_lastSubProgress != val )
        Q_EMIT subProgress( val );
}

} // namespace K3b


