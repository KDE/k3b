/*
 *
 * Copyright (C) 2005-2007 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bdebuggingoutputfile.h"

#include <k3bdevicemanager.h>
#include <k3bdevice.h>
#include <k3bcore.h>
#include <k3bversion.h>
#include <k3bdeviceglobals.h>
#include <k3bglobals.h>

#include <kstandarddirs.h>
#include <kglobalsettings.h>
#include <kapplication.h>

#include <qtextstream.h>


K3b::DebuggingOutputFile::DebuggingOutputFile()
    : QFile( KStandardDirs::locateLocal( "appdata", "lastlog.log", true ) )
{
}


bool K3b::DebuggingOutputFile::open( OpenMode mode )
{
    if( !QFile::open( mode|WriteOnly ) )
        return false;

    addOutput( QLatin1String( "System" ), QLatin1String( "K3b Version: " ) + k3bcore->version() );
    addOutput( QLatin1String( "System" ), QLatin1String( "KDE Version: " ) + QString(KDE::versionString()) );
    addOutput( QLatin1String( "System" ), QLatin1String( "QT Version:  " ) + QString(qVersion()) );
    addOutput( QLatin1String( "System" ), QLatin1String( "Kernel:      " ) + K3b::kernelVersion() );

    // devices in the logfile
    Q_FOREACH( K3b::Device::Device* dev, k3bcore->deviceManager()->allDevices() ) {
        addOutput( "Devices",
                   QString( "%1 (%2, %3) [%5] [%6] [%7]" )
                   .arg( dev->vendor() + " " + dev->description() + " " + dev->version() )
                   .arg( dev->blockDeviceName() )
                   .arg( K3b::Device::deviceTypeString( dev->type() ) )
                   .arg( K3b::Device::mediaTypeString( dev->supportedProfiles() ) )
                   .arg( K3b::Device::writingModeString( dev->writingModes() ) ) );
    }

    return true;
}


void K3b::DebuggingOutputFile::addOutput( const QString& app, const QString& msg )
{
    if( !isOpen() )
        open();

    QTextStream s( this );
    s << "[" << app << "] " << msg << endl;
    flush();
}

#include "k3bdebuggingoutputfile.moc"
