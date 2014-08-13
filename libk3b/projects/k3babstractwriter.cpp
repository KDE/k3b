/*
 *
 * Copyright (C) 2003-2010 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2010 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3babstractwriter.h"

#include "k3bcore.h"
#include "k3bdevicemanager.h"
#include "k3bdevicehandler.h"
#include "k3bglobalsettings.h"

#include <KI18n/KLocalizedString>
#include <KDELibs4Support/KDE/KGlobal>



K3b::AbstractWriter::AbstractWriter( K3b::Device::Device* dev, K3b::JobHandler* jh, QObject* parent )
    : K3b::Job( jh, parent ),
      m_burnDevice(dev),
      m_burnSpeed(0),
      m_simulate(false),
      m_sourceUnreadable(false)
{
}


K3b::AbstractWriter::~AbstractWriter()
{
}


K3b::Device::Device* K3b::AbstractWriter::burnDevice()
{
    if( m_burnDevice )
        return m_burnDevice;
    else
        return k3bcore->deviceManager()->burningDevices()[0];
}


void K3b::AbstractWriter::cancel()
{
    if( burnDevice() ) {
        // we need to unlock the writer because cdrecord locked it while writing
        emit infoMessage( i18n("Unlocking drive..."), MessageInfo );
        connect( K3b::Device::unblock( burnDevice() ), SIGNAL(finished(bool)),
                 this, SLOT(slotUnblockWhileCancellationFinished(bool)) );
    }
    else {
        emit canceled();
        jobFinished(false);
    }
}


void K3b::AbstractWriter::slotUnblockWhileCancellationFinished( bool success )
{
    if( !success )
        emit infoMessage( i18n("Could not unlock drive."), K3b::Job::MessageError );

    if( k3bcore->globalSettings()->ejectMedia() ) {
        emit newSubTask( i18n("Ejecting Medium") );
        connect( K3b::Device::eject( burnDevice() ), SIGNAL(finished(bool)),
                 this, SLOT(slotEjectWhileCancellationFinished(bool)) );
    }
    else {
        emit canceled();
        jobFinished( false );
    }
}


void K3b::AbstractWriter::slotEjectWhileCancellationFinished( bool success )
{
    if( !success ) {
        emit infoMessage( i18n("Unable to eject medium."), K3b::Job::MessageError );
    }

    emit canceled();
    jobFinished( false );
}



