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

#include <device/k3bdevicemanager.h>
#include <tools/k3bexternalbinmanager.h>
#include <tools/k3bdefaultexternalprograms.h>
#include <tools/k3bglobals.h>
#include <tools/k3bversion.h>
#include <rip/songdb/k3bsongmanager.h>
#include <k3bdoc.h>

#include <klocale.h>
#include <kconfig.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>


K3bApplication* K3bApplication::s_k3bApp = 0;



K3bApplication::K3bApplication()
  : KApplication()
{
  m_core = new K3bCore( aboutData(), this );
  connect( m_core, SIGNAL(initializationInfo(const QString&)), 
	   SIGNAL(initializationInfo(const QString&)) );
  s_k3bApp = this;

  connect( this, SIGNAL(shutDown()), SLOT(slotShutDown()) );
}


K3bApplication::~K3bApplication()
{
}


K3bMainWindow* K3bApplication::k3bMainWindow() const
{
  return static_cast<K3bMainWindow*>( mainWidget() );
}


void K3bApplication::init()
{
  m_core->init();

  emit initializationInfo( i18n("Creating Gui...") );

  K3bMainWindow *k3bMainWidget = new K3bMainWindow();
  setMainWidget( k3bMainWidget );

  k3bMainWidget->show();

  emit initializationInfo( i18n("Ready.") );

  KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

  if( args->isSet( "data" ) ) {
    // create new data project and add all arguments
    k3bMainWidget->slotNewDataDoc();
    K3bDoc* doc = k3bMainWidget->activeDoc();
    for( int i = 0; i < args->count(); i++ ) {
      doc->addUrl( args->url(i) );
    }
  } else if( args->isSet( "audio" ) ) {
    // create new audio project and add all arguments
    k3bMainWidget->slotNewAudioDoc();
    K3bDoc* doc = k3bMainWidget->activeDoc();
    for( int i = 0; i < args->count(); i++ ) {
      doc->addUrl( args->url(i) );
    }
  } else if( args->isSet( "mixed" ) ) {
    // create new audio project and add all arguments
    k3bMainWidget->slotNewMixedDoc();
    K3bDoc* doc = k3bMainWidget->activeDoc();
    for( int i = 0; i < args->count(); i++ ) {
      doc->addUrl( args->url(i) );
    }
  } else if( args->isSet( "vcd" ) ) {
    // create new audio project and add all arguments
    k3bMainWidget->slotNewVcdDoc();
    K3bDoc* doc = k3bMainWidget->activeDoc();
    for( int i = 0; i < args->count(); i++ ) {
      doc->addUrl( args->url(i) );
    }
  } else if( args->isSet( "emovix" ) ) {
    // create new audio project and add all arguments
    k3bMainWidget->slotNewMovixDoc();
    K3bDoc* doc = k3bMainWidget->activeDoc();
    for( int i = 0; i < args->count(); i++ ) {
      doc->addUrl( args->url(i) );
    }
  } else if( args->isSet( "isoimage" ) ) {
    if ( args->count() == 1 )
      k3bMainWidget->slotWriteIsoImage( args->url(0) );
    else
      k3bMainWidget->slotWriteIsoImage();
  } else if( args->isSet( "binimage" ) ) {
    if ( args->count() == 1 )
      k3bMainWidget->slotWriteBinImage( args->url(0) );
    else
      k3bMainWidget->slotWriteBinImage();
  } else if(args->count()) {
    for( int i = 0; i < args->count(); i++ ) {
      k3bMainWidget->openDocumentFile( args->url(i) );
    }
  }

  if( args->isSet("copy") )
    k3bMainWidget->slotCdCopy();

  args->clear();
}


void K3bApplication::slotShutDown()
{
  m_core->externalBinManager()->saveConfig( config() );
  m_core->deviceManager()->saveConfig( config() );
}

#include "k3bapplication.moc"
