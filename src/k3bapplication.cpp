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


#include "k3bapplication.h"
#include "k3b.h"
#include "k3bcore.h"
#include "k3binterface.h"

#include <device/k3bdevicemanager.h>
#include <tools/k3bexternalbinmanager.h>
#include <tools/k3bdefaultexternalprograms.h>
#include <tools/k3bglobals.h>
#include <tools/k3bversion.h>
#include <rip/songdb/k3bsongmanager.h>
#include <k3bdoc.h>
#include <k3bsystemproblemdialog.h>

#include <klocale.h>
#include <kconfig.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <dcopclient.h>


K3bApplication* K3bApplication::s_k3bApp = 0;



K3bApplication::K3bApplication()
  : KApplication(),
    m_interface(0),
    m_mainWindow(0)
{
  m_core = new K3bCore( aboutData(), this );
  connect( m_core, SIGNAL(initializationInfo(const QString&)),
	   SIGNAL(initializationInfo(const QString&)) );
  s_k3bApp = this;

  connect( this, SIGNAL(shutDown()), SLOT(slotShutDown()) );
}


K3bApplication::~K3bApplication()
{
  // we must not delete m_mainWindow here, QApplication takes care of it
  delete m_interface;
}


K3bMainWindow* K3bApplication::k3bMainWindow() const
{
  return m_mainWindow;
}


void K3bApplication::init()
{
  m_core->init();

  emit initializationInfo( i18n("Creating GUI...") );

  m_mainWindow = new K3bMainWindow();
  setMainWidget( m_mainWindow );

  if( dcopClient()->attach() ) {
    dcopClient()->registerAs( "k3b" );
    m_interface = new K3bInterface( m_mainWindow );
    dcopClient()->setDefaultObject( m_interface->objId() );
  }
  else {
    kdDebug() << "(K3bApplication) unable to attach to dcopserver!" << endl;
  }

  m_mainWindow->show();

  emit initializationInfo( i18n("Ready.") );


  if( config()->readBoolEntry( "check system config", true ) ) {
    emit initializationInfo( i18n("Checking System") );
    K3bSystemProblemDialog::checkSystem();
  }

  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  if( args->isSet( "data" ) ) {
    // create new data project and add all arguments
    m_mainWindow->slotNewDataDoc();
    K3bDoc* doc = m_mainWindow->activeDoc();
    for( int i = 0; i < args->count(); i++ ) {
      doc->addUrl( args->url(i) );
    }
  }
  else if( args->isSet( "audio" ) ) {
    // create new audio project and add all arguments
    m_mainWindow->slotNewAudioDoc();
    K3bDoc* doc = m_mainWindow->activeDoc();
    for( int i = 0; i < args->count(); i++ ) {
      doc->addUrl( args->url(i) );
    }
  }
  else if( args->isSet( "mixed" ) ) {
    // create new audio project and add all arguments
    m_mainWindow->slotNewMixedDoc();
    K3bDoc* doc = m_mainWindow->activeDoc();
    for( int i = 0; i < args->count(); i++ ) {
      doc->addUrl( args->url(i) );
    }
  }
  else if( args->isSet( "vcd" ) ) {
    // create new audio project and add all arguments
    m_mainWindow->slotNewVcdDoc();
    K3bDoc* doc = m_mainWindow->activeDoc();
    for( int i = 0; i < args->count(); i++ ) {
      doc->addUrl( args->url(i) );
    }
  }
  else if( args->isSet( "emovix" ) ) {
    // create new audio project and add all arguments
    m_mainWindow->slotNewMovixDoc();
    K3bDoc* doc = m_mainWindow->activeDoc();
    for( int i = 0; i < args->count(); i++ ) {
      doc->addUrl( args->url(i) );
    }
  }
  else if( args->isSet( "isoimage" ) ) {
    if ( args->count() == 1 )
      m_mainWindow->slotWriteIsoImage( args->url(0) );
    else
      m_mainWindow->slotWriteIsoImage();
  }
  else if( args->isSet( "binimage" ) ) {
    if ( args->count() == 1 )
      m_mainWindow->slotWriteBinImage( args->url(0) );
    else
      m_mainWindow->slotWriteBinImage();
  }
  else if(args->count()) {
    for( int i = 0; i < args->count(); i++ ) {
      m_mainWindow->openDocumentFile( args->url(i) );
    }
  }

  if( args->isSet("copy") )
    m_mainWindow->slotCdCopy();
  else if( args->isSet("erase") )
    m_mainWindow->slotBlankCdrw();

  args->clear();
}


void K3bApplication::slotShutDown()
{
  m_core->externalBinManager()->saveConfig( config() );
  m_core->deviceManager()->saveConfig( config() );
}

#include "k3bapplication.moc"
