/* 
 *
 * $Id: sourceheader 380067 2005-01-19 13:03:46Z trueg $
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

#include "k3binterferingsystemshandler.h"

#include <k3bdevice.h>
#include <k3bprocess.h>

#include <klocale.h>
#include <kapplication.h>
#include <kstandarddirs.h>
#include <dcopclient.h>

#include <qcstring.h>
#include <qvaluelist.h>

#include <sys/types.h>
#include <signal.h>


class K3bInterferingSystemsHandler::Private
{
public:
  Private()
    : device( 0 ),
      disabled(false),
      disabledMediaManager(false),
      disabledSuSEPlugger(false),
      disabledAutofs(false) {
  }

  K3bDevice::Device* device;

  bool disabled;

  bool disabledMediaManager;
  bool disabledSuSEPlugger;
  bool disabledAutofs;
};



K3bInterferingSystemsHandler::K3bInterferingSystemsHandler( QObject* parent, const char* name )
  : K3bJob( 0, parent, name )
{
  d = new Private();
}


K3bInterferingSystemsHandler::~K3bInterferingSystemsHandler()
{
  delete d;
}


void K3bInterferingSystemsHandler::setDevice( K3bDevice::Device* dev )
{
  d->device = dev;
}


void K3bInterferingSystemsHandler::disable( K3bDevice::Device* dev )
{
  setDevice( dev );
  disable();
}


// TODO: maybe create a class K3bInterferingSystemModule which we simply keep in a list
void K3bInterferingSystemsHandler::disable()
{
  if( !d->disabled ) {
    d->disabled = true;

    int r = startStopMediaManager( false );
    d->disabledMediaManager = ( r == 1 );
    if( r == 1 )
      emit infoMessage( i18n("Disabled KDED module mediamanager."), INFO );
    else if( r == -1 )
      emit infoMessage( i18n("Failed to disable KDED module mediamanager."), WARNING );


    r = startStopSuSEPlugger( false );
    d->disabledSuSEPlugger = ( r == 1 );
    if( r == 1 )
      emit infoMessage( i18n("Shut down SuSEPlugger."), INFO );
    else if( r == -1 )
      emit infoMessage( i18n("Failed to shut down SuSEPlugger."), WARNING );

    if( d->device ) {
      r = startStopAutomounting( false, d->device );
      d->disabledAutofs = ( r == 1 );
      if( r == 1 )
	emit infoMessage( i18n("Disabled Automounting."), INFO );
      else if( r == -1 )
	emit infoMessage( i18n("Failed to disable Automounting."), WARNING );
    }
  }
}


void K3bInterferingSystemsHandler::enable()
{
  if( d->disabled ) {
    if( d->disabledMediaManager ) {
      int r = startStopMediaManager( true );
      if( r == -1 )
	emit infoMessage( i18n("Failed to enable KDED module mediamanager."), WARNING );
      else {
	d->disabledMediaManager = false;
	if( r == 1 )
	  emit infoMessage( i18n("Enabled KDED module mediamanager."), INFO );
      }
    }

    if( d->disabledSuSEPlugger ) {
      int r = startStopSuSEPlugger( true );
      if( r == -1 )
	emit infoMessage( i18n("Failed to start SuSEPlugger."), WARNING );
      else {
	d->disabledSuSEPlugger = false;
	if( r == 1 )
	  emit infoMessage( i18n("Restarted SuSEPlugger."), INFO );
      }
    }

    if( d->disabledAutofs ) {
      int r = startStopAutomounting( true, d->device );
      if( r == -1 )
	emit infoMessage( i18n("Failed to enabled Automounting."), WARNING );
      else {
	d->disabledAutofs = false;
	if( r == 1 )
	  emit infoMessage( i18n("Enabled Automounting."), INFO );
      }
    }

    d->disabled = false;
  }
}


int K3bInterferingSystemsHandler::startStopMediaManager( bool start )
{
    // check if the mediamanager is running
  bool running = false;
  QByteArray replyData;
  QCString replyType;
  if( KApplication::dcopClient()->call( "kded",
					"kded",
					"loadedModules()",
					QByteArray(),
					replyType,
					replyData,
					false, /* no eventloop */
					10000 /* 10 second timeout */ ) ) {
    QCStringList l;    
    QDataStream s( replyData, IO_ReadOnly );
    s >> l;
    kdDebug() << l << endl;
    running = l.contains( QCString("mediamanager") );
  }
  else
    kdDebug() << "(K3bApplication) call to kded::loadedModules failed." << endl;


  if( start ) {
    if( running )
      return 0;

    // FIXME: kded crashes when we do this through dcopClient(), so for now we call the dcop
    //        command line tool
    KProcess p;
    p << "dcop" << "kded" << "kded" << "loadModule" << "mediamanager";
    if( p.start( KProcess::Block ) ) {
      //       QCString replyType;
      //       QByteArray replyData;
      //       if( dcopClient()->call( "kded", "kded", "loadModule(QCString)", QCString("mediamanager"), replyType, replyData, false, 10000 ) )
      return 1;
    }
    else
      return -1;
  }

  else {
    if( !running )
      return 0;
    
    //
    // mediamanager is running
    //
    
    // FIXME: kded crashes when we do this through dcopClient(), so for now we call the dcop
    //        command line tool
    KProcess p;
    p << "dcop" << "kded" << "kded" << "unloadModule" << "mediamanager";
    if( p.start( KProcess::Block ) ) {
      
      //       QByteArray data;
      //       QDataStream ds( data, IO_WriteOnly );
      //       ds << QCString("mediamanager");
      //       if( dcopClient()->call( "kded", 
      // 			      "kded", 
      // 			      "unloadModule(QCString)", 
      // 			      data, 
      // 			      replyType, 
      // 			      replyData, 
      // 			      false, 
      // 			      10000 ) ) {
      // TODO: check the return value
      return 1;
    }
    else
      return -1;
  }
}


int K3bInterferingSystemsHandler::startStopSuSEPlugger( bool start )
{
  // check if it's running
  bool running;
  QCStringList objects = KApplication::dcopClient()->remoteObjects( "suseplugger", &running );

  if( start ) {
    if( running )
      return 0;

    return ( KApplication::startServiceByDesktopName( "suseplugger" ) == 0 );
  }
  else {
    if( !running )
      return 0;
 
    // newer versions of SuSEPlugger have a MainApplication-Interface which has a quit function
    if( objects.contains( "MainApplication-Interface" ) ) {
      if( KApplication::dcopClient()->send( "suseplugger", "MainApplication-Interface", "quit()", QByteArray() ) )
	return 1;
    }

    // simply kill suseplugger
    QString susePluggerPath = KStandardDirs::findExe( "suseplugger" );
    KProcess p;
    p << "pidof" << susePluggerPath;
    K3bProcessOutputCollector out( &p );
    p.start( KProcess::Block, KProcess::Stdout );
    int susePluggerPid = out.stdout().toInt();
    if( susePluggerPid > 0 ) {
      return ( ::kill( susePluggerPid, SIGTERM ) == 0 ? 1 : -1 );
    }
    else {
      kdDebug() << "(K3bInterferingSystemsHandler) unable to determine suseplugger pid." << endl;
      return -1;
    }
  }
}


int K3bInterferingSystemsHandler::startStopAutomounting( bool start, K3bDevice::Device* dev )
{
  //
  // here we simply call the script if we can find it and don't care if automounting is actually enabled
  // or not since the script returns a proper error code telling us everything we need to know.
  //
  QString autoMountingScript = KStandardDirs::findExe( "k3b_automount" );
  if( autoMountingScript.isEmpty() ) {
    kdDebug() << "(K3bInterferingSystemsHandler) could not find the automounting script" << endl;
    return -1;
  }

  KProcess p;
  p << autoMountingScript;
  if( start )
    p << "enable";
  else
    p << "disable";
  p << dev->blockDeviceName();

  if( p.start( KProcess::Block ) ) {
    if( p.normalExit() ) {
      //
      //  Exit codes:
      //    0 - success
      //    1 - wrong usage
      //    2 - device not configured with subfs/supermount in /etc/fstab
      //    X - failed to mount/umount
      //
      switch( p.exitStatus() ) {
      case 0:
	return 1;
      case 1:
	kdDebug() << "(K3bInterferingSystemsHandler) k3b_automount usage failure." << endl;
	return -1;
      case 2:
	return 0;
      default:
	kdDebug() << "(K3bInterferingSystemsHandler) some k3b_automount problem." << endl;
	return -1;
      }
    }
    else {
      kdDebug() << "(K3bInterferingSystemsHandler) the automounting script failed in some way." << endl;
      return -1;
    }
  }
  else {
    kdDebug() << "(K3bInterferingSystemsHandler) could not start the automounting script." << endl;
    return -1;
  }
}

#include "k3binterferingsystemshandler.moc"
