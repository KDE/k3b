/* 
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 * Copyright (C) 2006 Sebastian Trueg <trueg@k3b.org>
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

#include <k3bjob.h>

#include <qcstring.h>
#include <qdatastream.h>


K3bJobInterface::K3bJobInterface( QObject* parent )
  : QObject( parent ),
    DCOPObject( "K3bJobInterface" ),
    m_job( 0 )
{
}


void K3bJobInterface::setJob( K3bJob* job )
{
  if( m_job )
    m_job->disconnect( this );

  m_job = job;
  m_lastProgress = m_lastSubProgress = 0;

  if( m_job ) {
    connect( m_job, SIGNAL(newTask(const QString&)), this, SLOT(slotNewTask(const QString&)) );
    connect( m_job, SIGNAL(newSubTask(const QString&)), this, SLOT(slotNewSubTask(const QString&)) );
    connect( m_job, SIGNAL(infoMessage(const QString&, int)), this, SLOT(slotInfoMessage(const QString&, int)) );
    connect( m_job, SIGNAL(finished(bool)), this, SLOT(slotFinished(bool)) );
    connect( m_job, SIGNAL(started()), this, SLOT(slotStarted()) );
    connect( m_job, SIGNAL(canceled()), this, SLOT(slotCanceled()) );
    connect( m_job, SIGNAL(percent(int)), this, SLOT(slotProgress(int)) );
    connect( m_job, SIGNAL(subPercent(int)), this, SLOT(slotSubProgress(int)) );
    connect( m_job, SIGNAL(nextTrack(int, int)), this, SLOT(slotNextTrack(int, int)) );

    if( m_job->inherits( "K3bBurnJob" ) ) {
      connect( m_job, SIGNAL(bufferStatus(int)), this, SLOT(slotBuffer(int)) );
      connect( m_job, SIGNAL(deviceBuffer(int)), this, SLOT(slotDeviceBuffer(int)) );
    }

    connect( m_job, SIGNAL(destroyed()), this, SLOT(slotDestroyed()) );
  }
}


bool K3bJobInterface::jobRunning() const
{
  return ( m_job && m_job->active() );
}


QString K3bJobInterface::jobDescription() const
{
  if( m_job )
    return m_job->jobDescription();
  else
    return QString::null;
}


QString K3bJobInterface::jobDetails() const
{
  if( m_job )
    return m_job->jobDetails();
  else
    return QString::null;
}


void K3bJobInterface::slotStarted()
{
  m_lastProgress = m_lastSubProgress = 0;
  emitDCOPSignal( "started()", QByteArray() );
}


void K3bJobInterface::slotCanceled()
{
  emitDCOPSignal( "canceled()", QByteArray() );
}


void K3bJobInterface::slotFinished( bool success )
{
  QByteArray params;
  QDataStream stream(params, IO_WriteOnly);
  stream << success;
  emitDCOPSignal( "finished(bool)", params );
}


void K3bJobInterface::slotInfoMessage( const QString& message, int type )
{
  QByteArray params;
  QDataStream stream(params, IO_WriteOnly);
  stream << message << type;
  emitDCOPSignal( "infoMessage(QString)", params );
}


void K3bJobInterface::slotProgress( int val )
{
  if( m_lastProgress != val ) {
    m_lastProgress = val;
    QByteArray params;
    QDataStream stream(params, IO_WriteOnly);
    stream << val;
    emitDCOPSignal( "progress(int)", params );
  }
}


void K3bJobInterface::slotSubProgress( int val )
{
  if( m_lastSubProgress != val ) {
    m_lastSubProgress = val;
    QByteArray params;
    QDataStream stream(params, IO_WriteOnly);
    stream << val;
    emitDCOPSignal( "subProgress(int)", params );
  }
}


void K3bJobInterface::slotNewTask( const QString& task )
{
  QByteArray params;
  QDataStream stream(params, IO_WriteOnly);
  stream << task;
  emitDCOPSignal( "newTask(QString)", params );
}


void K3bJobInterface::slotNewSubTask( const QString& task )
{
  QByteArray params;
  QDataStream stream(params, IO_WriteOnly);
  stream << task;
  emitDCOPSignal( "newSubTask(QString)", params );
}


void K3bJobInterface::slotBuffer( int val )
{
  QByteArray params;
  QDataStream stream(params, IO_WriteOnly);
  stream << val;
  emitDCOPSignal( "buffer(int)", params );
}


void K3bJobInterface::slotDeviceBuffer( int val )
{
  QByteArray params;
  QDataStream stream(params, IO_WriteOnly);
  stream << val;
  emitDCOPSignal( "deviceBuffer(int)", params );
}


void K3bJobInterface::slotNextTrack( int track, int numTracks )
{
  QByteArray params;
  QDataStream stream(params, IO_WriteOnly);
  stream << track << numTracks;
  emitDCOPSignal( "nextTrack(int,int)", params );
}


void K3bJobInterface::slotDestroyed()
{
  m_job = 0;
}

#include "k3bjobinterface.moc"
