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

#include "k3b.h"
#include "tools/k3bglobals.h"
#include "device/k3bdevice.h"
#include "tools/k3bexternalbinmanager.h"

#include <kprocess.h>
#include <kconfig.h>
#include <klocale.h>
#include <kio/global.h>
#include <kio/job.h>

#include <qstring.h>
#include <kdebug.h>



K3bBlankingJob::K3bBlankingJob( QObject* parent )
  : K3bJob( parent )
{
  m_device = 0;
  m_mode = Fast;
  m_speed = 1;
  m_writingApp = K3b::DEFAULT;

}


K3bBlankingJob::~K3bBlankingJob()
{
  delete m_blankingJob;
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
    // emit infoMessage( i18n("Unmounting disk"), INFO );
    // unmount the cd
    connect( KIO::unmount( m_device->mountPoint(), false ), SIGNAL(result(KIO::Job*)),
	     this, SLOT(slotStartErasing()) );
  }
  else {
    slotStartErasing();
  }
}

void K3bBlankingJob::slotStartErasing()
{
  switch ( m_writingApp )
  {
     case K3b::CDRDAO:  slotUseCdrdao(); break;
     case K3b::DEFAULT:
     case K3b::CDRECORD:
     default:           slotUseCdrecord(); break;
  }
}

void K3bBlankingJob::slotUseCdrecord()
{
  m_blankingJob = new K3bCdrecordWriter(m_device,this);
  connect(m_blankingJob, SIGNAL(finished(bool)), this, SLOT(slotFinished(bool)));
  connect(m_blankingJob, SIGNAL(infoMessage( const QString&, int)),
          this,SIGNAL(infoMessage( const QString&, int)));

  ((K3bCdrecordWriter *)m_blankingJob)->prepareArgumentList();

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

  ((K3bCdrecordWriter *)m_blankingJob)->addArgument("blank="+ mode);

  if (m_force)
    ((K3bCdrecordWriter *)m_blankingJob)->addArgument("-force");
  ((K3bCdrecordWriter *)m_blankingJob)->setBurnSpeed(m_speed);

  ((K3bCdrecordWriter *)m_blankingJob)->start();

}

void K3bBlankingJob::slotUseCdrdao()
{
  m_blankingJob = new K3bCdrdaoWriter(m_device,this);
  connect(m_blankingJob, SIGNAL(finished(bool)), this, SLOT(slotFinished(bool)));
  connect(m_blankingJob, SIGNAL(infoMessage( const QString&, int)),
          this,SIGNAL(infoMessage( const QString&, int)));

  ((K3bCdrdaoWriter *)m_blankingJob)->setCommand(K3bCdrdaoWriter::BLANK);

  int mode = K3bCdrdaoWriter::MINIMAL;
  switch( m_mode ) {
  case Fast:
    mode = K3bCdrdaoWriter::MINIMAL;
    break;
  case Complete:
    mode = K3bCdrdaoWriter::FULL;
    break;
  }

  ((K3bCdrdaoWriter *)m_blankingJob)->setBlankMode(mode);
  ((K3bCdrdaoWriter *)m_blankingJob)->setForce(m_force);
  ((K3bCdrdaoWriter *)m_blankingJob)->setBurnSpeed(m_speed);

  ((K3bCdrdaoWriter *)m_blankingJob)->start();
}

void K3bBlankingJob::cancel()
{
   switch ( m_writingApp )
  {
     case K3b::CDRDAO:  ((K3bCdrdaoWriter *)m_blankingJob)->cancel(); break;
     case K3b::DEFAULT:
     case K3b::CDRECORD:
     default:           ((K3bCdrecordWriter *)m_blankingJob)->cancel(); break;
  }
  emit canceled();
  emit finished( false );
}


void K3bBlankingJob::slotFinished(bool success)
{
  if( success )
  {
	  emit infoMessage( i18n("Process completed successfully"), K3bJob::STATUS );
	  emit finished( true );
	} else {
	  emit infoMessage( i18n("Blanking error "), K3bJob::ERROR );
	  emit infoMessage( i18n("Sorry, no error handling yet! :-(("), K3bJob::ERROR );
	  emit finished( false );
	}
}



#include "k3bblankingjob.moc"
