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

#include <k3bexternalbinmanager.h>
#include <k3bdevicemanager.h>
#include <k3bdevicehandler.h>
#include <k3bdevice.h>
#include <k3bcore.h>
#include <k3bglobals.h>

#include <klocale.h>
#include <kprocess.h>
#include <kdebug.h>

#include <qstringlist.h>


K3bMsInfoFetcher::K3bMsInfoFetcher( QObject* parent, const char* name )
  : K3bJob( parent, name ),
    m_process(0),
    m_device(0),
    m_dvd(false)
{
}


K3bMsInfoFetcher::~K3bMsInfoFetcher()
{
  delete m_process;
}


void K3bMsInfoFetcher::start()
{
  emit infoMessage( i18n("Searching previous session"), K3bJob::INFO );

  if( !k3bcore->externalBinManager()->foundBin( "cdrecord" ) ) {
    kdDebug() << "(K3bMsInfoFetcher) could not find cdrecord executable" << endl;
    emit infoMessage( i18n("Could not find %1 executable.").arg("cdrecord"), K3bJob::ERROR );
    emit finished(false);
    return;
  }

  if( m_device == 0 ) {
    kdDebug() << "(K3bMsInfoFetcher) internal error: No device set!" << endl;
    emit finished(false);
    return;
  }

  //
  // first we try to determine if it is a dvd. If so we need to
  // read the info on our own
  //

  connect( K3bCdDevice::sendCommand( K3bCdDevice::DeviceHandler::NG_DISKINFO, m_device ),
	   SIGNAL(finished(K3bCdDevice::DeviceHandler*)),
	   this,
	   SLOT(slotMediaDetectionFinished(K3bCdDevice::DeviceHandler*)) );
}


void K3bMsInfoFetcher::getMsInfo()
{
  delete m_process;
  m_process = new KProcess();

  const K3bExternalBin* bin = 0;
  if( m_dvd )
    bin = k3bcore->externalBinManager()->binObject( "dvdrecord" );
  else
    bin = k3bcore->externalBinManager()->binObject( "cdrecord" );
 
  if( !bin ) {
    emit infoMessage( i18n("Could not find %1 executable.").arg( m_dvd ? "dvdrecord" : "cdrecord" ), ERROR );
    emit finished(false);
    return;
  }

  *m_process << bin->path;

  // add the device (e.g. /dev/sg1)
  *m_process << QString("dev=%1").arg( K3b::externalBinDeviceParameter(m_device, bin) );

  *m_process << "-msinfo";

  kdDebug() << "***** " << bin->name() << " parameters:\n";
  const QValueList<QCString>& args = m_process->args();
  QString s;
  for( QValueList<QCString>::const_iterator it = args.begin(); it != args.end(); ++it ) {
    s += *it + " ";
  }
  kdDebug() << s << flush << endl;
  emit debuggingOutput( "msinfo comand:", s );


//   connect( m_process, SIGNAL(receivedStderr(KProcess*, char*, int)),
// 	   this, SLOT(slotCollectOutput(KProcess*, char*, int)) );
  connect( m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
	   this, SLOT(slotCollectOutput(KProcess*, char*, int)) );
  connect( m_process, SIGNAL(processExited(KProcess*)),
	   this, SLOT(slotProcessExited()) );

  m_msInfo = QString::null;
  m_collectedOutput = QString::null;
  m_canceled = false;

  if( !m_process->start( KProcess::NotifyOnExit, KProcess::AllOutput ) ) {
    emit infoMessage( i18n("Could not start %1.").arg(bin->name()), K3bJob::ERROR );
    emit finished(false);
  }
}


void K3bMsInfoFetcher::slotMediaDetectionFinished( K3bCdDevice::DeviceHandler* h )
{
  if( h->success() ) {
    m_dvd = h->ngDiskInfo().isDvdMedia();
  }
  else {
    // for now we just default to cd and go on with the detecting
    m_dvd = false;
  }

  getMsInfo();
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
  emit debuggingOutput( "msinfo", QString::fromLocal8Bit( output, len ) );

  m_collectedOutput += QString::fromLocal8Bit( output, len );
}


void K3bMsInfoFetcher::cancel()
{
  // FIXME: this does not work if the devicehandler is running

  if( m_process )
    if( m_process->isRunning() ) {
      m_canceled = true;
      m_process->kill();
      emit canceled();
      emit finished(false);
    }
}


#include "k3bmsinfofetcher.moc"
