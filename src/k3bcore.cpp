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
#include <tools/k3bexternalbinmanager.h>
#include <tools/k3bdefaultexternalprograms.h>
#include <tools/k3bglobals.h>
#include <tools/k3bversion.h>
#include <rip/songdb/k3bsongmanager.h>

#include <kapplication.h>
#include <klocale.h>
#include <kconfig.h>
#include <kaboutdata.h>



class K3bCore::Private {
public:
  Private( const KAboutData* about )
    : version( about->version() ) {
    songManager = 0;
  }

  K3bVersion version;
  K3bSongManager* songManager;
};



K3bCore* K3bCore::s_k3bCore = 0;



K3bCore::K3bCore( const KAboutData* about, QObject* parent, const char* name)
  : QObject( parent, name )
{
  d = new Private( about );
  s_k3bCore = this;
}


K3bCore::~K3bCore()
{
  delete d;
}


K3bCdDevice::DeviceManager* K3bCore::deviceManager() const
{
  return K3bCdDevice::DeviceManager::self();
}


K3bExternalBinManager* K3bCore::externalBinManager() const
{
  return K3bExternalBinManager::self();
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

  K3b::addDefaultPrograms( K3bExternalBinManager::self() );
  K3bExternalBinManager::self()->search();

  if( globalConfig.hasGroup("External Programs") ) {
    globalConfig.setGroup( "External Programs" );
    K3bExternalBinManager::self()->readConfig( &globalConfig );
  }

  if( config()->hasGroup("External Programs") ) {
    config()->setGroup( "External Programs" );
    K3bExternalBinManager::self()->readConfig( config() );
  }

  // ===============================================================================


  // device manager
  // ===============================================================================
  emit initializationInfo( i18n("Scanning for CD devices...") );

  if( !K3bDeviceManager::self()->scanbus() )
    kdDebug() << "No Devices found!" << endl;

  if( globalConfig.hasGroup("Devices") ) {
    globalConfig.setGroup( "Devices" );
    K3bDeviceManager::self()->readConfig( &globalConfig );
  }

  if( config()->hasGroup("Devices") ) {
    config()->setGroup( "Devices" );
    K3bDeviceManager::self()->readConfig( config() );
  }

  K3bDeviceManager::self()->printDevices();
  // ===============================================================================

  //  emit initializationInfo( i18n("Initializing CD view...") );

  // ===============================================================================
//   emit initializationInfo( i18n("Reading local CDDB database...") );
//   config()->setGroup("Cddb");
//   QString filename = config()->readEntry("songlistPath", locateLocal("data", "k3b") + "/songlist.xml");
//   d->songManager = new K3bSongManager();
//   d->songManager->load( filename );

  emit initializationInfo( i18n("Ready.") );
}


void K3bCore::saveConfig()
{
  config()->setGroup( "Devices" );
  deviceManager()->saveConfig( config() );
  config()->setGroup( "External Programs" );
  externalBinManager()->saveConfig( config() );
}

#include "k3bcore.moc"
