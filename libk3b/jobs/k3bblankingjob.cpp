/*
 *
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

#include "k3bblankingjob.h"
#include "k3bcdrecordwriter.h"
#include "k3bcdrdaowriter.h"

#include <k3bglobals.h>
#include <k3bdevice.h>
#include <k3bdevicehandler.h>
#include <k3bcore.h>
#include <k3bglobalsettings.h>

#include <kconfig.h>
#include <klocale.h>
#include <kio/global.h>
#include <kio/job.h>
#include <kdebug.h>

#include <qstring.h>



K3bBlankingJob::K3bBlankingJob( K3bJobHandler* hdl, QObject* parent )
  : K3bBurnJob( hdl, parent ),
    m_writerJob(0),
    m_force(true),
    m_device(0),
    m_speed(0),
    m_mode(Fast),
    m_writingApp(K3b::WRITING_APP_DEFAULT),
    m_canceled(false),
    m_forceNoEject(false)
{
}


K3bBlankingJob::~K3bBlankingJob()
{
  delete m_writerJob;
}


K3bDevice::Device* K3bBlankingJob::writer() const
{
  return m_device;
}


void K3bBlankingJob::setDevice( K3bDevice::Device* dev )
{
  m_device = dev;
}


void K3bBlankingJob::start()
{
  if( m_device == 0 )
    return;

  jobStarted();

  emit newTask( i18n( "Erasing CD-RW" ) );
  emit infoMessage( i18n( "When erasing a CD-RW no progress information is available." ), WARNING );

  slotStartErasing();
}

void K3bBlankingJob::slotStartErasing()
{
  m_canceled = false;

  if( m_writerJob )
    delete m_writerJob;

  if( m_writingApp == K3b::WRITING_APP_CDRDAO ) {
    K3bCdrdaoWriter* writer = new K3bCdrdaoWriter( m_device, this );
    m_writerJob = writer;

    writer->setCommand(K3bCdrdaoWriter::BLANK);
    writer->setBlankMode( m_mode == Fast ? K3bCdrdaoWriter::MINIMAL : K3bCdrdaoWriter::FULL );
    writer->setForce(m_force);
    writer->setBurnSpeed(m_speed);
  }
  else {
    K3bCdrecordWriter* writer = new K3bCdrecordWriter( m_device, this );
    m_writerJob = writer;

    QString mode;
    switch( m_mode ) {
    case Fast:
      mode = "fast";
      break;
    case Complete:
      mode = "all";
      break;
    case Track:
      mode = "track";
      break;
    case Unclose:
      mode = "unclose";
      break;
    case Session:
      mode = "session";
      break;
    }

    writer->addArgument("blank="+ mode);

    if (m_force)
      writer->addArgument("-force");
    writer->setBurnSpeed(m_speed);
  }

  connect(m_writerJob, SIGNAL(finished(bool)), this, SLOT(slotFinished(bool)));
  connect(m_writerJob, SIGNAL(infoMessage( const QString&, int)),
          this,SIGNAL(infoMessage( const QString&, int)));
  connect( m_writerJob, SIGNAL(debuggingOutput(const QString&, const QString&)),
	   this, SIGNAL(debuggingOutput(const QString&, const QString&)) );

  if( waitForMedia( m_device,
		    K3bDevice::STATE_COMPLETE|K3bDevice::STATE_INCOMPLETE,
		    K3bDevice::MEDIA_CD_RW,
		    i18n("Please insert a rewritable CD medium into drive<p><b>%1 %2 (%3)</b>.",
		    m_device->vendor(),
		    m_device->description(),
		    m_device->blockDeviceName()) ) < 0 ) {
    emit canceled();
    jobFinished(false);
    return;
  }

  m_writerJob->start();
}


void K3bBlankingJob::cancel()
{
  m_canceled = true;

  if( m_writerJob )
    m_writerJob->cancel();
}


void K3bBlankingJob::slotFinished(bool success)
{
    if ( !m_forceNoEject && k3bcore->globalSettings()->ejectMedia() ) {
        K3bDevice::eject( m_device );
    }

    if( success ) {
        emit percent( 100 );
        jobFinished( true );
    }
    else {
        if( m_canceled ) {
            emit canceled();
        }
        else {
            emit infoMessage( i18n("Blanking error "), K3bJob::ERROR );
            emit infoMessage( i18n("Sorry, no error handling yet."), K3bJob::ERROR );
        }
        jobFinished( false );
    }
}


QString K3bBlankingJob::jobDescription() const
{
  return i18n("Erasing CD-RW");
}


QString K3bBlankingJob::jobDetails() const
{
  if( m_mode == Fast )
    return i18n("Quick Format");
  else
    return QString();
}

#include "k3bblankingjob.moc"
