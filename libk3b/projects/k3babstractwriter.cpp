/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3babstractwriter.h"

#include <k3bcore.h>
#include <k3bdevicemanager.h>
#include <k3bdevicehandler.h>
#include <k3bglobalsettings.h>

#include <klocale.h>
#include <kglobal.h>



K3bAbstractWriter::K3bAbstractWriter( K3bDevice::Device* dev, K3bJobHandler* jh, QObject* parent, const char* name )
  : K3bJob( jh, parent, name ),
    m_burnDevice(dev),
    m_burnSpeed(1),
    m_simulate(false),
    m_sourceUnreadable(false)
{
}


K3bAbstractWriter::~K3bAbstractWriter()
{
}


K3bDevice::Device* K3bAbstractWriter::burnDevice()
{
  if( m_burnDevice )
    return m_burnDevice; 
  else
    return k3bcore->deviceManager()->burningDevices().getFirst();
}


void K3bAbstractWriter::cancel()
{
  if( burnDevice() ) {
    // we need to unlock the writer because cdrecord locked it while writing
    emit infoMessage( i18n("Unlocking drive..."), INFO );
    connect( K3bDevice::unblock( burnDevice() ), SIGNAL(finished(bool)),
	     this, SLOT(slotUnblockWhileCancellationFinished(bool)) );
  }
  else {
    emit canceled();
    jobFinished(false);
  }
}


void K3bAbstractWriter::slotUnblockWhileCancellationFinished( bool success )
{
  if( success ) {
    if( k3bcore->globalSettings()->ejectMedia() ) {
      emit newSubTask( i18n("Ejecting CD") );  // FIXME: "media" instead of "CD"
      connect( K3bDevice::eject( burnDevice() ), SIGNAL(finished(bool)),
	       this, SLOT(slotEjectWhileCancellationFinished(bool)) );
      return;
    }
  }
  else {
    emit infoMessage( i18n("Could not unlock CD drive."), K3bJob::ERROR ); // FIXME: simply "drive", not "CD drive"
  }

  emit canceled();
  jobFinished( false );
}


void K3bAbstractWriter::slotEjectWhileCancellationFinished( bool success )
{
  if( !success ) {
    emit infoMessage( i18n("Unable to eject media."), K3bJob::ERROR );
  }

  emit canceled();
  jobFinished( false );
}


#include "k3babstractwriter.moc"
