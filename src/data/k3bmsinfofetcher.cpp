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

#include "k3bmsinfofetcher.h"

#include <tools/k3bexternalbinmanager.h>
#include <device/k3bdevicemanager.h>
#include <device/k3bdevice.h>
#include <k3bcore.h>

#include <klocale.h>
#include <kprocess.h>
#include <kdebug.h>

#include <qstringlist.h>


K3bMsInfoFetcher::K3bMsInfoFetcher( QObject* parent, const char* name )
  : K3bJob( parent, name ),
    m_process(0),
    m_device(0)
{
}


K3bMsInfoFetcher::~K3bMsInfoFetcher()
{
    delete m_process;
}


void K3bMsInfoFetcher::start()
{
  emit infoMessage( i18n("Searching previous session"), K3bJob::PROCESS );

  delete m_process;
  m_process = new KProcess();

  if( !k3bcore->externalBinManager()->foundBin( "cdrecord" ) ) {
    kdDebug() << "(K3bMsInfoFetcher) could not find cdrecord executable" << endl;
    emit infoMessage( i18n("cdrecord executable not found."), K3bJob::ERROR );
    emit finished(false);
    return;
  }

  if( m_device == 0 ) {
    kdDebug() << "(K3bMsInfoFetcher) internal error: No device set!" << endl;
    emit finished(false);
    return;
  }

  *m_process << k3bcore->externalBinManager()->binPath( "cdrecord" );
  *m_process << "-msinfo";

  // add the device (e.g. /dev/sg1)
  *m_process << QString("dev=%1").arg( m_device->busTargetLun() );

  connect( m_process, SIGNAL(receivedStderr(KProcess*, char*, int)),
	   this, SLOT(slotCollectOutput(KProcess*, char*, int)) );
  connect( m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
	   this, SLOT(slotCollectOutput(KProcess*, char*, int)) );
  connect( m_process, SIGNAL(processExited(KProcess*)),
	   this, SLOT(slotProcessExited()) );

  m_msInfo = QString::null;
  m_collectedOutput = QString::null;
  m_canceled = false;

  if( !m_process->start( KProcess::NotifyOnExit, KProcess::AllOutput ) ) {
    emit infoMessage( i18n("Could not start cdrecord!"), K3bJob::ERROR );
    emit finished(false);
  }
}


void K3bMsInfoFetcher::slotProcessExited()
{
  if( m_canceled )
    return;

  kdDebug() << "(K3bMsInfoFetcher) msinfo fetched" << endl;

  // now parse the output
  QString firstLine = m_collectedOutput.left( m_collectedOutput.find("\n") );
  QStringList list = QStringList::split( ",",  firstLine );
  if( list.count() == 2 ) {
    bool ok1, ok2;
    m_lastSessionStart = list.first().toInt( &ok1 );
    m_nextSessionStart = list[1].toInt( &ok2 );
    if( ok1 && ok2 )
      m_msInfo = firstLine.stripWhiteSpace();
    else
      m_msInfo = QString::null;
  }
  else {
    m_msInfo = QString::null;
  }

  kdDebug() << "(K3bMsInfoFetcher) msinfo parsed: " << m_msInfo << endl;

  if( m_msInfo.isEmpty() ) {
    emit infoMessage( i18n("Could not retrieve multisession information from disk."), K3bJob::ERROR );
    emit infoMessage( i18n("The disk is either empty or not appendable."), K3bJob::ERROR );
    emit finished(false);
  }
  else {
    emit finished(true);
  }
}


void K3bMsInfoFetcher::slotCollectOutput( KProcess*, char* output, int len )
{
  m_collectedOutput += QString::fromLocal8Bit( output, len );
}


void K3bMsInfoFetcher::cancel()
{
  if( m_process )
    if( m_process->isRunning() ) {
      m_canceled = true;
      m_process->kill();
      emit canceled();
      emit finished(false);
    }
}


#include "k3bmsinfofetcher.moc"
