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

#include "k3bblankingjob.h"
#include "k3bcdrecordwriter.h"
#include "k3bcdrdaowriter.h"

#include <k3bglobals.h>
#include <device/k3bdevice.h>
#include <device/k3bdevicehandler.h>

#include <kconfig.h>
#include <klocale.h>
#include <kio/global.h>
#include <kio/job.h>
#include <kdebug.h>

#include <qstring.h>



K3bBlankingJob::K3bBlankingJob( QObject* parent )
  : K3bJob( parent ),
    m_writerJob(0),
    m_force(false),
    m_device(0),
    m_speed(0),
    m_mode(Fast),
    m_writingApp(K3b::DEFAULT),
    m_canceled(false),
    m_forceNoEject(false)
{
}


K3bBlankingJob::~K3bBlankingJob()
{
  delete m_writerJob;
}


void K3bBlankingJob::setDevice( K3bDevice* dev )
{
  m_device = dev;
}


void K3bBlankingJob::start()
{
  if( m_device == 0 )
    return;

  if( !KIO::findDeviceMountPoint( m_device->mountDevice() ).isEmpty() ) {
    // TODO: enable me after message freeze
    emit infoMessage( i18n("Unmounting disk"), INFO );
    // unmount the cd
    connect( K3bCdDevice::unmount(m_device),SIGNAL(finished(K3bCdDevice::DeviceHandler *)),
	     this, SLOT(slotStartErasing()) );
  }
  else {
    slotStartErasing();
  }
}

void K3bBlankingJob::slotStartErasing()
{
  m_canceled = false;

  if( m_writerJob )
    delete m_writerJob;

  if( m_writingApp == K3b::CDRDAO ) {
    K3bCdrdaoWriter* writer = new K3bCdrdaoWriter( m_device, this );
    m_writerJob = writer;
    
    writer->setCommand(K3bCdrdaoWriter::BLANK);
    writer->setBlankMode( m_mode == Fast ? K3bCdrdaoWriter::MINIMAL : K3bCdrdaoWriter::FULL );
    writer->setForce(m_force);
    writer->setBurnSpeed(m_speed);
    writer->setForceNoEject( m_forceNoEject );
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
    writer->setForceNoEject( m_forceNoEject );
  }

  connect(m_writerJob, SIGNAL(finished(bool)), this, SLOT(slotFinished(bool)));
  connect(m_writerJob, SIGNAL(infoMessage( const QString&, int)),
          this,SIGNAL(infoMessage( const QString&, int)));

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
  if( success ) {
    emit infoMessage( i18n("Process completed successfully"), K3bJob::STATUS );
    emit finished( true );
  }
  else {
    if( m_canceled ) {
      emit infoMessage( i18n("Canceled!"), ERROR );
      emit canceled();
    }
    else {
      emit infoMessage( i18n("Blanking error "), K3bJob::ERROR );
      emit infoMessage( i18n("Sorry, no error handling yet! :-(("), K3bJob::ERROR );
    }
    emit finished( false );
  }
}



#include "k3bblankingjob.moc"
