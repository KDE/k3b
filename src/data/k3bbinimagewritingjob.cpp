/*
 *
 * $Id$
 * Copyright (C) 2003 Klaus-Dieter Krannich <kd@k3b.org>
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


#include "k3bbinimagewritingjob.h"

#include <k3bemptydiscwaiter.h>
#include <device/k3bdevice.h>

#include <klocale.h>
#include <kdebug.h>



K3bBinImageWritingJob::K3bBinImageWritingJob( QObject* parent )
        : K3bBurnJob( parent ),
        m_copies(1)
{
    m_cdrdaowriter = new K3bCdrdaoWriter(0, this);
    connect(m_cdrdaowriter,SIGNAL(percent(int)),
            this,SLOT(copyPercent(int)));
    connect(m_cdrdaowriter,SIGNAL(subPercent(int)),
            this,SLOT(copySubPercent(int)));
    connect(m_cdrdaowriter,SIGNAL(buffer(int)),
            this,SIGNAL(bufferStatus(int)));
    connect(m_cdrdaowriter,SIGNAL(newSubTask(const QString&)),
            this, SIGNAL(newSubTask(const QString&)) );
    connect(m_cdrdaowriter,SIGNAL(infoMessage(const QString&, int)),
            this, SIGNAL(infoMessage(const QString&, int)) );
    connect(m_cdrdaowriter,SIGNAL(debuggingOutput(const QString&, const QString&)),
            this,SIGNAL(debuggingOutput(const QString&, const QString&)));
    connect(m_cdrdaowriter,SIGNAL(finished(bool)),
            this,SLOT(cdrdaoFinished(bool)));
    connect(m_cdrdaowriter, SIGNAL(nextTrack(int, int)),
            this, SLOT(slotNextTrack(int, int)) );
    connect( m_cdrdaowriter, SIGNAL(writeSpeed(int)), 
	     this, SIGNAL(writeSpeed(int)) );
}


K3bBinImageWritingJob::~K3bBinImageWritingJob()
{
}


void K3bBinImageWritingJob::start() 
{
  m_canceled =  false;

  if( m_copies < 1 )
    m_copies = 1;
  m_finishedCopies = 0;

  emit started();
  emit newTask( i18n("Write Binary Image") );
  
  cdrdaoWrite();
}

void K3bBinImageWritingJob::cancel() 
{
  m_canceled = true;
  m_cdrdaowriter->cancel();
  emit canceled();
  emit finished( false );
}


void K3bBinImageWritingJob::cdrdaoWrite() 
{
  m_cdrdaowriter->setCommand(K3bCdrdaoWriter::WRITE);
  if( K3bEmptyDiscWaiter::wait( m_cdrdaowriter->burnDevice() ) == K3bEmptyDiscWaiter::CANCELED ) {
    cancel();
  }
  // just to be sure we did not get canceled during the async discWaiting
  else if( !m_canceled ) {
    m_cdrdaowriter->start();
  }
}

void K3bBinImageWritingJob::copyPercent(int p)
{
  emit percent( (100*m_finishedCopies + p)/m_copies );
}

void K3bBinImageWritingJob::copySubPercent(int p)
{
  emit subPercent(p);
}

void K3bBinImageWritingJob::cdrdaoFinished(bool ok) 
{
  if( m_canceled )
    return;

  if (ok) {
    m_finishedCopies++;
    if ( m_finishedCopies == m_copies ) {
      emit infoMessage( i18n("%1 copies succsessfully created").arg(m_copies),K3bJob::INFO );
      emit finished( true );
    } 
    else 
      cdrdaoWrite();
  }
  else {
    emit finished(false);
  }
}


void K3bBinImageWritingJob::slotNextTrack( int t, int tt )
{
  emit newSubTask( i18n("Writing track %1 of %2").arg(t).arg(tt) );
}


QString K3bBinImageWritingJob::jobDescription() const
{
  return i18n("Writing cue/bin image");
}


QString K3bBinImageWritingJob::jobDetails() const
{
  return m_tocFile.section("/", -1);
}


void K3bBinImageWritingJob::setTocFile(const QString& s)
{ 
  m_cdrdaowriter->setTocFile(s); 
  m_tocFile = s; 
}
		
#include "k3bbinimagewritingjob.moc"
