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


#include "k3bcore.h"

#include <device/k3bdevicemanager.h>
#include <k3bexternalbinmanager.h>
#include <k3bdefaultexternalprograms.h>
#include <k3bglobals.h>
#include <k3bversion.h>

#include <kapplication.h>
#include <klocale.h>
#include <kconfig.h>
#include <kaboutdata.h>
#include <kstandarddirs.h>



class K3bCore::Private {
public:
  Private( const KAboutData* about )
    : version( about->version() ) {
  }

  K3bVersion version;
  K3bDeviceManager* deviceManager;
  K3bExternalBinManager* externalBinManager;
};



K3bCore* K3bCore::s_k3bCore = 0;



K3bCore::K3bCore( const KAboutData* about, QObject* parent, const char* name)
  : QObject( parent, name )
{
  d = new Private( about );
  s_k3bCore = this;

  d->externalBinManager = new K3bExternalBinManager( this );
  d->deviceManager = new K3bCdDevice::DeviceManager( d->externalBinManager, this );
  K3b::addDefaultPrograms( d->externalBinManager );
}


K3bCore::~K3bCore()
{
  delete d;
}


K3bCdDevice::DeviceManager* K3bCore::deviceManager() const
{
  return d->deviceManager;
}


K3bExternalBinManager* K3bCore::externalBinManager() const
{
  return d->externalBinManager;
}


const K3bVersion& K3bCore::version() const
{
  return d->version;
}


KConfig* K3bCore::config() const
{
  return kapp->config();
}


void K3bCore::init()
{
  emit initializationInfo( i18n("Reading Options...") );

  KConfig globalConfig( K3b::globalConfig() );
  config()->setGroup( "General Options" );
  K3bVersion globalConfigVersion( globalConfig.readEntry( "config version", "0.1" ) );
  K3bVersion configVersion( config()->readEntry( "config version", "0.1" ) );

  // external bin manager
  // ===============================================================================
  emit initializationInfo( i18n("Searching for external programs...") );

  d->externalBinManager->search();

  if( globalConfig.hasGroup("External Programs") ) {
    globalConfig.setGroup( "External Programs" );
    d->externalBinManager->readConfig( &globalConfig );
  }

  if( config()->hasGroup("External Programs") ) {
    config()->setGroup( "External Programs" );
    d->externalBinManager->readConfig( config() );
  }

  // ===============================================================================


  // device manager
  // ===============================================================================
  //

  emit initializationInfo( i18n("Scanning for CD devices...") );

  if( !d->deviceManager->scanbus() )
    kdDebug() << "No Devices found!" << endl;

  if( globalConfig.hasGroup("Devices") ) {
    globalConfig.setGroup( "Devices" );

    d->deviceManager->readConfig( &globalConfig );
  }

  if( config()->hasGroup("Devices") ) {
    config()->setGroup( "Devices" );
    d->deviceManager->readConfig( config() );
  }

  d->deviceManager->printDevices();
  // ===============================================================================

  //  emit initializationInfo( i18n("Initializing CD view...") );

  // ===============================================================================
}


void K3bCore::saveConfig()
{
  config()->setGroup( "General Options" );
  config()->writeEntry( "config version", version() );

  // FIXME: I think this should be done when saving the options.
  config()->setGroup( "Devices" );
  deviceManager()->saveConfig( config() );
  config()->setGroup( "External Programs" );
  externalBinManager()->saveConfig( config() );
}


void K3bCore::requestBusyInfo( const QString& text )
{
  emit busyInfoRequested( text );
}

void K3bCore::requestBusyFinish()
{
  emit busyFinishRequested();
}

#include "k3bcore.moc"
