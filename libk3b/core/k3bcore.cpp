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

#include <config.h>

#include "k3bcore.h"

#include <k3bdevicemanager.h>
#include <k3bexternalbinmanager.h>
#include <k3bdefaultexternalprograms.h>
#include <k3bglobals.h>
#include <k3bversion.h>
#include <k3bjob.h>
#include <k3bthreadwidget.h>

#include <klocale.h>
#include <kconfig.h>
#include <kaboutdata.h>
#include <kstandarddirs.h>
#include <kapplication.h>

#include <qptrlist.h>


class K3bCore::Private {
public:
  Private( const K3bVersion& about )
    : version( about ) {
  }

  KConfig* config;
  K3bVersion version;
  K3bDevice::DeviceManager* deviceManager;
  K3bExternalBinManager* externalBinManager;

  QPtrList<K3bJob> runningJobs;
};



K3bCore* K3bCore::s_k3bCore = 0;



K3bCore::K3bCore( const K3bVersion& version, KConfig* c, QObject* parent, const char* name )
  : QObject( parent, name )
{
  d = new Private( version );
  d->config = c;
  if( !c )
    d->config = kapp->config();

  if( s_k3bCore ) {
    qFatal("ONLY ONE INSTANCE OF K3BCORE ALLOWED!");
  }

  s_k3bCore = this;

  d->externalBinManager = new K3bExternalBinManager( this );
  d->deviceManager = new K3bDevice::DeviceManager( this );
  K3b::addDefaultPrograms( d->externalBinManager );

  // create the thread widget instance in the GUI thread
  K3bThreadWidget::instance();
}


K3bCore::~K3bCore()
{
  delete d;
}


K3bDevice::DeviceManager* K3bCore::deviceManager() const
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
  return d->config;
}


void K3bCore::init()
{
  emit initializationInfo( i18n("Reading Options...") );

  config()->setGroup( "General Options" );
  K3bVersion configVersion( config()->readEntry( "config version", "0.1" ) );

  // external bin manager
  // ===============================================================================
  emit initializationInfo( i18n("Searching for external programs...") );

  d->externalBinManager->search();

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


void K3bCore::registerJob( K3bJob* job )
{
  d->runningJobs.append( job );
}


void K3bCore::unregisterJob( K3bJob* job )
{
  d->runningJobs.removeRef( job );
}


bool K3bCore::jobsRunning() const
{
  return !d->runningJobs.isEmpty();
}

#include "k3bcore.moc"
