/***************************************************************************
 *   Copyright (C) 2002 by Sebastian Trueg                                 *
 *   trueg@k3b.org                                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "k3bmovixjob.h"
#include "k3bmovixdoc.h"
#include <data/k3bdatajob.h>

#include <klocale.h>
#include <kdebug.h>


K3bMovixJob::K3bMovixJob( K3bMovixDoc* doc, QObject* parent )
  : K3bBurnJob( parent ),
    m_doc(doc)
{
  m_dataJob = new K3bDataJob( doc, this );
  m_playlistFile = 0;

  // pipe signals
  connect( m_dataJob, SIGNAL(percent(int)), this, SIGNAL(percent(int)) );
  connect( m_dataJob, SIGNAL(subPercent(int)), this, SIGNAL(subPercent(int)) );
  connect( m_dataJob, SIGNAL(processedSubSize(int, int)), this, SIGNAL(processedSubSize(int, int)) );
  connect( m_dataJob, SIGNAL(processedSize(int, int)), this, SIGNAL(processedSize(int, int)) );
  connect( m_dataJob, SIGNAL(bufferStatus(int)), this, SIGNAL(bufferStatus(int)) );
  connect( m_dataJob, SIGNAL(writeSpeed(int)), this, SIGNAL(writeSpeed(int)) );
  connect( m_dataJob, SIGNAL(newTask(const QString&)), this, SIGNAL(newTask(const QString&)) );
  connect( m_dataJob, SIGNAL(newSubTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
  connect( m_dataJob, SIGNAL(debuggingOutput(const QString&, const QString&)),
	   this, SIGNAL(debuggingOutput(const QString&, const QString&)) );
  connect( m_dataJob, SIGNAL(infoMessage(const QString&, int)),
	   this, SIGNAL(infoMessage(const QString&, int)) );

  // we need to clean up here
  connect( m_dataJob, SIGNAL(finished(bool)), this, SLOT(slotDataJobFinished(bool)) );
}


K3bMovixJob::~K3bMovixJob()
{
}


K3bDevice* K3bMovixJob::writer() const
{
  return m_dataJob->writer();
}


K3bDoc* K3bMovixJob::doc() const
{
  return m_doc; 
}


void K3bMovixJob::start()
{
  m_canceled = false;
  m_dataJob->setWritingApp( writingApp() );

  // TODO: create playlist file and add it to the doc

  m_dataJob->start();
}


void K3bMovixJob::cancel()
{
  m_canceled = true;
  m_dataJob->cancel();
}


void K3bMovixJob::slotDataJobFinished( bool success )
{
  // TODO: remove playlist file from doc and delete it

  if( m_canceled )
    emit canceled();

  emit finished( success );
}


bool K3bMovixJob::writePlaylistFile()
{
  m_playlistFile = new KTempFile();

  return true;
}

#include "k3bmovixjob.moc"
