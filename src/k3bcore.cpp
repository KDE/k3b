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
  emit initializationInfo( i18n("Reading local Song database...") );
  config()->setGroup( "General" );

  QString filename = config()->readEntry( "songlistPath", locateLocal("data", "k3b") + "/songlist.xml" );
  d->songManager = new K3bSongManager( this );
  d->songManager->load( filename );


  emit initializationInfo( i18n("Checking System") );
  checkSystem();

  emit initializationInfo( i18n("Ready.") );
}


void K3bCore::saveConfig()
{
  config()->setGroup( "Devices" );
  deviceManager()->saveConfig( config() );
  config()->setGroup( "External Programs" );
  externalBinManager()->saveConfig( config() );
  songManager()->save();
}


void K3bCore::checkSystem() const
{
  QValueList<K3bSystemProblem> problems;

  // 1. cdrecord, cdrdao
  if( !externalBinManager()->foundBin( "cdrecord" ) ) {
    problems.append( K3bSystemProblem( K3bSystemProblem::CRITICAL,
				       i18n("Unable to find %1 executable").arg("cdrecord"),
				       i18n("K3b uses cdrecord to actually write cds. "
					    "Without cdrecord K3b won't be able to properly "
					    "initialize the writing devices."),
				       i18n("Install the cdrtools package which contains "
					    "cdrecord."),
				       false ) );
  }
  else if( !externalBinManager()->binObject( "cdrecord" )->hasFeature( "suidroot" ) ) {
    problems.append( K3bSystemProblem( K3bSystemProblem::CRITICAL,
				       i18n("%1 does not run with root privileges").arg("cdrecord"),
				       i18n("cdrecord needs to run with root privileges "
					    "to be able to access the cd devices, "
					    "use real time scheduling, and "
					    "set a non-standard fifo buffer. This is also "
					    "true when using SuSE's resmgr."),
				       i18n("Use K3bSetup to solve this problem."),
				       true ) );
  }

  if( !externalBinManager()->foundBin( "cdrdao" ) ) {
    problems.append( K3bSystemProblem( K3bSystemProblem::CRITICAL,
				       i18n("Unable to find %1 executable").arg("cdrdao"),
				       i18n("K3b uses cdrdao to actually write cds. "
					    "Without cdrdao you won't be able to copy cds, "
					    "write cue/bin images, write CD-TEXT, and write "
					    "audio cds on-the-fly."),
				       i18n("Install the cdrdao package."),
				       false ) );
  }
  else if( !externalBinManager()->binObject( "cdrdao" )->hasFeature( "suidroot" ) ) {
    problems.append( K3bSystemProblem( K3bSystemProblem::CRITICAL,
				       i18n("%1 does not run with root privileges").arg("cdrdao"),
				       i18n("cdrdao needs to run with root privileges "
					    "to be able to access the cd devices and "
					    "use real time scheduling."
					    "This is also true when using SuSE's resmgr."),
				       i18n("Use K3bSetup to solve this problem."),
				       true ) );
  }


  // 2. ATAPI devices
  bool atapiReader = false;
  bool atapiWriter = false;
  for( QPtrListIterator<K3bCdDevice::CdDevice> it( deviceManager()->readingDevices() );
       it.current(); ++it ) {
    if( it.current()->interfaceType() == K3bCdDevice::CdDevice::IDE ) {
      atapiReader = true;
      break;
    }
  }
  for( QPtrListIterator<K3bCdDevice::CdDevice> it( deviceManager()->burningDevices() );
       it.current(); ++it ) {
    if( it.current()->interfaceType() == K3bCdDevice::CdDevice::IDE ) {
      atapiWriter = true;
      break;
    }
  }

  if( atapiWriter ) {
    if( !K3bCdDevice::plainAtapiSupport() &&
	!K3bCdDevice::hackedAtapiSupport() ) {
      problems.append( K3bSystemProblem( K3bSystemProblem::CRITICAL,
					 i18n("No ATAPI writing support in kernel"),
					 i18n("Your kernel does not support writing without "
					      "SCSI emulation but there is at least one "
					      "writer in your system not configured to use "
					      "SCSI emulation."),
					 i18n("The best and recommended solution is to enable "
					      "ide-scsi (SCSI emulation) for all writer devices. "
					      "This way you won't have any problems."),
					 false ) );
    }
    else {
      // we have atapi support in some way in the kernel

      if( externalBinManager()->foundBin( "cdrecord" ) ) {

	if( !( externalBinManager()->binObject( "cdrecord" )->hasFeature( "hacked-atapi" ) &&
	       K3bCdDevice::hackedAtapiSupport() ) &&
	    !( externalBinManager()->binObject( "cdrecord" )->hasFeature( "plain-atapi" ) &&
	       K3bCdDevice::plainAtapiSupport() ) ) {
	  problems.append( K3bSystemProblem( K3bSystemProblem::CRITICAL,
					     i18n("%1 %2 does not support ATAPI").arg("cdrecord").arg(externalBinManager()->binObject("cdrecord")->version),
					     i18n("The configured version of %1 does not "
						  "support writing to ATAPI devices without "
						  "SCSI emulation and there is at least one writer "
						  "in your system not configured to use "
						  "SCSI emulation.").arg("cdrecord"),
					     i18n("The best and recommended solution is to enable "
						  "ide-scsi (SCSI emulation) for all writer devices. "
						  "This way you won't have any problems. Or you install "
						  "(or select as the default) a more recent version of %1.").arg("cdrtools"),
					     false ) );
	}
      }

      if( externalBinManager()->foundBin( "cdrdao" ) ) {

	if( !( externalBinManager()->binObject( "cdrdao" )->hasFeature( "hacked-atapi" ) &&
	       K3bCdDevice::hackedAtapiSupport() ) &&
	    !( externalBinManager()->binObject( "cdrdao" )->hasFeature( "plain-atapi" ) &&
	       K3bCdDevice::plainAtapiSupport() ) ) {
	  problems.append( K3bSystemProblem( K3bSystemProblem::CRITICAL,
					     i18n("%1 %2 does not support ATAPI").arg("cdrdao").arg(externalBinManager()->binObject("cdrdao")->version),
					     i18n("The configured version of %1 does not "
						  "support writing to ATAPI devices without "
						  "SCSI emulation and there is at least one writer "
						  "in your system not configured to use "
						  "SCSI emulation.").arg("cdrdao"),
					     i18n("The best and recommended solution is to enable "
						  "ide-scsi (SCSI emulation) for all writer devices. "
						  "This way you won't have any problems. Or you install "
						  "(or select as the default) a more recent version of %1.").arg("cdrdao"),
					     false ) );
	}
	else {
	  problems.append( K3bSystemProblem( K3bSystemProblem::NON_CRITICAL,
					     i18n("cdrdao has problems with ATAPI writers"),
					     i18n("When K3b %1 was released no version of cdrdao "
						  "was able to write without SCSI emulation. "
						  "Although it is possible that there actually "
						  "is a version with ATAPI support it is unlikely."),
					     i18n("The best and recommended solution is to enable "
						  "ide-scsi (SCSI emulation) for all writer devices. "
						  "This way you won't have any problems."),
					     false ) );
	}
      }
    }
  }

  if( atapiReader ) { 

    if( externalBinManager()->foundBin( "cdrdao" ) ) {
      
      if( !( externalBinManager()->binObject( "cdrdao" )->hasFeature( "hacked-atapi" ) &&
	     K3bCdDevice::hackedAtapiSupport() ) &&
	  !( externalBinManager()->binObject( "cdrdao" )->hasFeature( "plain-atapi" ) &&
	     K3bCdDevice::plainAtapiSupport() ) ) {
	problems.append( K3bSystemProblem( K3bSystemProblem::WARNING,
					   i18n("No support for ATAPI with cdrdao"),
					   i18n("You will not be able to use all your reading devices "
						"as copy sources since there is at least one not "
						"configured to use SCSI emulation and your system does "
						"not support ATAPI with cdrdao."),
					   i18n("The best and recommended solution is to enable "
						"ide-scsi (SCSI emulation) for all writer devices. "
						"This way you won't have any problems."),
					   false ) );
      }
    }
  }

  kdDebug() << "(K3bCore) System problems:" << endl;
  for( QValueList<K3bSystemProblem>::const_iterator it = problems.begin();
       it != problems.end(); ++it ) {
    const K3bSystemProblem& p = *it;

    switch( p.type ) {
    case K3bSystemProblem::CRITICAL:
      kdDebug() << " CRITICAL" << endl;
      break;
    case K3bSystemProblem::NON_CRITICAL:
      kdDebug() << " NON_CRITICAL" << endl;
      break;
    case K3bSystemProblem::WARNING:
      kdDebug() << " WARNING" << endl;
      break;
    }
    kdDebug() << " PROBLEM:  " << p.problem << endl
	      << " DETAILS:  " << p.details << endl
	      << " SOLUTION: " << p.solution << endl << endl;

  }
  if( problems.isEmpty() )
    kdDebug() << "          - none - " << endl;
  else {
    K3bSystemProblemDialog( problems ).exec();
  }
}

#include "k3bcore.moc"
