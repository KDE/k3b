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
#include "k3bsystemproblemdialog.h"

#include <device/k3bdevicemanager.h>
#include <tools/k3bexternalbinmanager.h>
#include <tools/k3bdefaultexternalprograms.h>
#include <tools/k3bglobals.h>
#include <tools/k3bversion.h>
#include <rip/songdb/k3bsongmanager.h>

#include <kapplication.h>
#include <klocale.h>
#include <kconfig.h>
#include <kaboutdata.h>
#include <kstandarddirs.h>


class K3bCore::Private {
public:
  Private( const KAboutData* about )
    : version( about->version() ) {
    songManager = 0;
  }

  K3bVersion version;
  K3bSongManager* songManager;
  K3bDeviceManager* deviceManager;
  K3bExternalBinManager* externalBinManager;
};



K3bCore* K3bCore::s_k3bCore = 0;



K3bCore::K3bCore( const KAboutData* about, QObject* parent, const char* name)
  : QObject( parent, name )
{
  d = new Private( about );
  s_k3bCore = this;

  d->externalBinManager = K3bExternalBinManager::self();
  d->deviceManager = K3bCdDevice::DeviceManager::self();
  K3b::addDefaultPrograms( d->externalBinManager );
  d->songManager = new K3bSongManager( this );
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


K3bSongManager* K3bCore::songManager() const
{
  return d->songManager;
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
  emit initializationInfo( i18n("Reading local Song database...") );
  config()->setGroup( "General Options" );

  QString filename = config()->readEntry( "songlistPath", locateLocal("data", "k3b") + "/songlist.xml" );
  d->songManager->load( filename );

  emit initializationInfo( i18n("Ready.") );
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
  songManager()->save();
}


#include "k3bcore.moc"
