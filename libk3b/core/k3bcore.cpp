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
#include <k3bglobalsettings.h>
#include <k3bpluginmanager.h>

#include <klocale.h>
#include <kconfig.h>
#include <kaboutdata.h>
#include <kstandarddirs.h>
#include <kapplication.h>

#include <qptrlist.h>


#define LIBK3B_VERSION "0.11.97"


class K3bCore::Private {
public:
  Private()
    : version( LIBK3B_VERSION ),
      config(0),
      deleteConfig(false) {
  }

  K3bVersion version;
  KConfig* config;
  bool deleteConfig;
  K3bDevice::DeviceManager* deviceManager;
  K3bExternalBinManager* externalBinManager;
  K3bPluginManager* pluginManager;
  K3bGlobalSettings* globalSettings;

  QPtrList<K3bJob> runningJobs;
};



K3bCore* K3bCore::s_k3bCore = 0;



K3bCore::K3bCore( QObject* parent, const char* name )
  : QObject( parent, name )
{
  d = new Private();

  if( s_k3bCore ) {
    qFatal("ONLY ONE INSTANCE OF K3BCORE ALLOWED!");
  }

  s_k3bCore = this;

  d->globalSettings = new K3bGlobalSettings();
  d->externalBinManager = new K3bExternalBinManager( this );
  d->deviceManager = new K3bDevice::DeviceManager( this );
  d->pluginManager = new K3bPluginManager( this );

  K3b::addDefaultPrograms( d->externalBinManager );

  // create the thread widget instance in the GUI thread
  K3bThreadWidget::instance();
}


K3bCore::~K3bCore()
{
  delete d->globalSettings;
  delete d->pluginManager;
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


K3bPluginManager* K3bCore::pluginManager() const
{
  return d->pluginManager;
}


K3bGlobalSettings* K3bCore::globalSettings() const
{
  return d->globalSettings;
}


const K3bVersion& K3bCore::version() const
{
  return d->version;
}


KConfig* K3bCore::config() const
{
  if( !d->config ) {
    d->deleteConfig = true;
    d->config = new KConfig( "k3brc" );
  }

  return d->config;
}


void K3bCore::init()
{
  // load the plugins before doing anything else
  // they might add external bins
  d->pluginManager->loadAll();

  d->externalBinManager->search();
  if( !d->deviceManager->scanbus() )
    kdDebug() << "No Devices found!" << endl;
}


void K3bCore::readSettings( KConfig* cnf )
{
  KConfig* c = cnf;
  if( !c )
    c = config();

  QString oldGrp = c->group();

  d->globalSettings->readSettings( c );
  d->deviceManager->readConfig( c );
  d->externalBinManager->readConfig( c );

  c->setGroup( oldGrp );
}


void K3bCore::saveSettings( KConfig* cnf )
{
  KConfig* c = cnf;
  if( !c )
    c = config();

  QString oldGrp = c->group();

  c->setGroup( "General Options" );
  c->writeEntry( "config version", version() );

  deviceManager()->saveConfig( c );
  externalBinManager()->saveConfig( c );
  d->globalSettings->saveSettings( c );

  c->setGroup( oldGrp );
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
