/* 
 *
 * $Id$
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2005 Sebastian Trueg <trueg@k3b.org>
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


K3bDebuggingOutputFile::K3bDebuggingOutputFile()
  : QFile( locateLocal( "appdata", "lastlog.log", true ) )
{
}


bool K3bDebuggingOutputFile::open()
{
  if( !QFile::open( IO_WriteOnly ) )
    return false;

  addOutput( "System", "K3b Version: " + k3bcore->version() );
  addOutput( "System", "KDE Version: " + QString(KDE::versionString()) );
  addOutput( "System", "QT Version:  " + QString(qVersion()) );
  addOutput( "System", "Kernel:      " + K3b::kernelVersion() );
  
  // devices in the logfile
  for( QPtrListIterator<K3bDevice::Device> it( k3bcore->deviceManager()->allDevices() ); *it; ++it ) {
    K3bDevice::Device* dev = *it;
    addOutput( "Devices", 
	       QString( "%1 (%2, %3) at %4 [%5] [%6] [%7]" )
	       .arg( dev->vendor() + " " + dev->description() + " " + dev->version() )
	       .arg( dev->blockDeviceName() )
	       .arg( dev->genericDevice() )
	       .arg( dev->mountPoint() )
	       .arg( K3bDevice::deviceTypeString( dev->type() ) )
	       .arg( K3bDevice::mediaTypeString( dev->supportedProfiles() ) )
	       .arg( K3bDevice::writingModeString( dev->writingModes() ) ) );
  }

  return true;
}


void K3bDebuggingOutputFile::addOutput( const QString& app, const QString& msg )
{
  if( !isOpen() )
    open();

  QTextStream s( this );
  s << "[" << app << "] " << msg << endl;// << QFile::flush;
}
