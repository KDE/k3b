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


#include "k3babstractwriter.h"

#include <k3bcore.h>
#include <device/k3bdevicemanager.h>
#include <device/k3bdevicehandler.h>

#include <klocale.h>
#include <kglobal.h>
#include <kconfig.h>


K3bAbstractWriter::K3bAbstractWriter( K3bDevice* dev, QObject* parent, const char* name )
  : K3bJob( parent, name ),
    m_burnDevice(dev),
    m_burnSpeed(1),
    m_burnproof(false),
    m_simulate(false)
{
}


K3bAbstractWriter::~K3bAbstractWriter()
{
}


K3bDevice* K3bAbstractWriter::burnDevice() const
{
  if( m_burnDevice )
    return m_burnDevice; 
  else
    return k3bcore->deviceManager()->burningDevices().first();
}


void K3bAbstractWriter::cancel()
{
  if( burnDevice() ) {
    // we need to unlock the writer because cdrecord locked it while writing
    emit infoMessage( i18n("Unblocking drive..."), INFO );
    connect( K3bCdDevice::unblock( burnDevice() ), SIGNAL(finished(bool)),
	     this, SLOT(slotUnblockWhileCancellationFinished(bool)) );
  }
  else {
    emit canceled();
    emit finished(false);
  }
}


void K3bAbstractWriter::slotUnblockWhileCancellationFinished( bool success )
{
  k3bcore->config()->setGroup("General Options");

  if( success ) {
    if( !k3bcore->config()->readBoolEntry( "No cd eject", false ) ) {
      emit infoMessage( i18n("Ejecting CD..."), INFO );
      connect( K3bCdDevice::eject( burnDevice() ), SIGNAL(finished(bool)),
	       this, SLOT(slotEjectWhileCancellationFinished(bool)) );
      return;
    }
  }
  else {
    emit infoMessage( i18n("Could not unlock CD drive."), K3bJob::ERROR );
  }

  emit canceled();
  emit finished( false );
}


void K3bAbstractWriter::slotEjectWhileCancellationFinished( bool success )
{
  if( !success ) {
    emit infoMessage( i18n("Could not eject CD."), K3bJob::ERROR );
  }

  emit canceled();
  emit finished( false );
}


void K3bAbstractWriter::createEstimatedWriteSpeed( int made, bool firstCall )
{
  if (firstCall) {
    m_lastWriteSpeedCalcTime = QTime::currentTime();
    m_firstWriteSpeedCalcTime = QTime::currentTime();
    m_lastWrittenBytes = made;
  }
  else {
    int elapsed = m_lastWriteSpeedCalcTime.msecsTo( QTime::currentTime() );
    int written = made - m_lastWrittenBytes;
    if( elapsed > 0 && written > 0 ) {
      m_lastWriteSpeedCalcTime = QTime::currentTime();
      m_lastWrittenBytes = made;
      emit writeSpeed( (int)((double)written * 1024.0 * 1000.0 / (double)elapsed) );
    }
  }
}


void K3bAbstractWriter::createAverageWriteSpeedInfoMessage()
{
  double secs = (double)m_firstWriteSpeedCalcTime.secsTo( m_lastWriteSpeedCalcTime );
  double speed = (double)m_lastWrittenBytes * 1024.0 / ( secs > 0 ? secs : 1 );
  emit infoMessage( i18n("Average overall write speed: %1 kb/s (%2x)").arg((int)speed).arg(KGlobal::locale()->formatNumber(speed/150.0,2)), INFO );
}

#include "k3babstractwriter.moc"
