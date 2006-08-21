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
#include "k3blsofwrapper.h"

#include <k3bdevice.h>
#include <k3bprocess.h>
#include <k3bjob.h>
#include <k3bglobals.h>

#include <klocale.h>
#include <kapplication.h>
#include <kstandarddirs.h>
#include <kstaticdeleter.h>
#include <dcopclient.h>

#include <qcstring.h>
#include <qvaluelist.h>
#include <qmap.h>
#include <qwaitcondition.h>
#include <qevent.h>
#include <qthread.h>

#include <sys/types.h>
#include <signal.h>
#include <unistd.h>


static Qt::HANDLE s_guiThreadHandle = QThread::currentThread();


class DisableEvent : public QCustomEvent
{
  public:
  DisableEvent( bool disable_, K3bDevice::Device* dev, QWaitCondition* wc_ )
    : QCustomEvent( QEvent::User + 33 ),
      disable(disable_),
      device(dev),
      wc(wc_) {
  }

  bool disable;
  K3bDevice::Device* device;
  QWaitCondition* wc;
};


K3bInterferingSystemsHandler* K3bInterferingSystemsHandler::s_instance = 0;

class K3bInterferingSystemsHandler::Private
{
public:
  Private()
    : /*disabledCnt( 0 ),*/
      disabledMediaManager(false),
      disabledMediaNotifier(false),
      disabledSuSEPlugger(false) {
  }

  int& operator[]( K3bDevice::Device* dev ) {
    QMap<K3bDevice::Device*,int>::iterator it = disabledCntMap.find( dev );
    if( it != disabledCntMap.end() )
      return it.data();
    else
      disabledCntMap[dev] = 0;
    return disabledCntMap[dev];
  }

  QMap<K3bDevice::Device*,int> disabledCntMap;

  //  int disabledCnt;

  bool disabledMediaManager;
  bool disabledMediaNotifier;
  bool disabledSuSEPlugger;
};



K3bInterferingSystemsHandler::K3bInterferingSystemsHandler()
  : QObject()
{
  d = new Private();
}


K3bInterferingSystemsHandler::~K3bInterferingSystemsHandler()
{
  // enable everything in case someone forgot to call enable
  for( QMap<K3bDevice::Device*,int>::const_iterator it = d->disabledCntMap.begin();
       it != d->disabledCntMap.end(); ++it ) {
    if( it.data() > 0 )
      enableInternal( it.key() );
  }

  delete d;
}


void K3bInterferingSystemsHandler::disable( K3bDevice::Device* dev )
{
  int& cnt = (*d)[dev];
  if( cnt == 0 )
    disableInternal( dev );
  ++cnt;
}


void K3bInterferingSystemsHandler::enable( K3bDevice::Device* dev )
{
  int& cnt = (*d)[dev];
  if( cnt > 0 ) {
    --cnt;
    if( cnt == 0 )
      enableInternal( dev );
  }
}


// TODO: maybe create a class K3bInterferingSystemModule which we simply keep in a list
void K3bInterferingSystemsHandler::disableInternal( K3bDevice::Device* dev )
{
  if( !d->disabledMediaManager ) {
    int r = startStopMediaManager( false );
    d->disabledMediaManager = ( r == 1 );
    if( r == 1 )
      emit infoMessage( i18n("Disabled KDED module %1.").arg( "mediamanager" ), K3bJob::INFO );
    else if( r == -1 )
      emit infoMessage( i18n("Failed to disable KDED module %1.").arg( "mediamanager" ), K3bJob::WARNING );
  }

  if( !d->disabledMediaNotifier ) {
    int r = startStopMediaNotifier( false );
    d->disabledMediaNotifier = ( r == 1 );
    if( r == 1 )
      emit infoMessage( i18n("Disabled KDED module %1.").arg( "medianotifier" ), K3bJob::INFO );
    else if( r == -1 )
      emit infoMessage( i18n("Failed to disable KDED module %1.").arg( "medianotifier" ), K3bJob::WARNING );
  }

  if( !d->disabledSuSEPlugger ) {
    int r = startStopSuSEPlugger( false );
    d->disabledSuSEPlugger = ( r == 1 );
    if( r == 1 )
      emit infoMessage( i18n("Shut down SuSEPlugger."), K3bJob::INFO );
    else if( r == -1 )
      emit infoMessage( i18n("Failed to shut down SuSEPlugger."), K3bJob::WARNING );
  }

  if( dev ) {
    blockUnblockPmount( true, dev );

//     int r = startStopAutomounting( false, dev );
//     if( r == 1 )
//       emit infoMessage( i18n("Disabled Automounting."), K3bJob::INFO );
//     else if( r == -1 )
//       emit infoMessage( i18n("Failed to disable Automounting."), K3bJob::WARNING );
    
    //
    // see if other applications are using the device
    //
    K3bLsofWrapper lsof;
    if( lsof.checkDevice( dev ) ) {
      if( !lsof.usingApplications().isEmpty() ) {
	// TODO: In a perfect workd we would block here until the user shut down the other
	//       applications but that is not easy:
	//       1. we are in libk3b -> no GUI if possible
	//       2. If we would for example use KMessageBox we would not have a parent widget and thus
	//          with some window managers the user would not see the dialog but only the stalled
	//          progress.
	emit infoMessage( i18n("The device '%1' is already in use by other applications "
			       "(%2) "
			       "It is highly recommended to quit those.")
			  .arg(dev->vendor() + " - " + dev->description())
			  .arg(lsof.usingApplications().join(", ")),
			  K3bJob::WARNING );
      }
    }
  }

  kdDebug() << "(K3bInterferingSystemsHandler) disabled for device " << dev->blockDeviceName() << endl;
}


void K3bInterferingSystemsHandler::enableInternal( K3bDevice::Device* dev )
{
  if( d->disabledMediaNotifier ) {
    int r = startStopMediaNotifier( true );
    if( r == -1 )
      emit infoMessage( i18n("Failed to enable KDED module %1.").arg( "medianotifier" ), K3bJob::WARNING );
    else {
      d->disabledMediaNotifier = false;
      if( r == 1 )
	emit infoMessage( i18n("Enabled KDED module %1.").arg( "medianotifier" ), K3bJob::INFO );
    }
  }

  if( d->disabledMediaManager ) {
    int r = startStopMediaManager( true );
    if( r == -1 )
      emit infoMessage( i18n("Failed to enable KDED module %1.").arg( "mediamanager" ), K3bJob::WARNING );
    else {
      d->disabledMediaManager = false;
      if( r == 1 )
	emit infoMessage( i18n("Enabled KDED module %1.").arg( "mediamanager" ), K3bJob::INFO );
    }
  }

  if( d->disabledSuSEPlugger ) {
    int r = startStopSuSEPlugger( true );
    if( r == -1 )
      emit infoMessage( i18n("Failed to start SuSEPlugger."), K3bJob::WARNING );
    else {
      d->disabledSuSEPlugger = false;
      if( r == 1 )
	emit infoMessage( i18n("Restarted SuSEPlugger."), K3bJob::INFO );
    }
  }

  if( dev ) {
    blockUnblockPmount( false, dev );

//     int r = startStopAutomounting( true, dev );
//     if( r == -1 )
//       emit infoMessage( i18n("Failed to enable Automounting."), K3bJob::WARNING );
//     else {
//       if( r == 1 )
// 	emit infoMessage( i18n("Enabled Automounting."), K3bJob::INFO );
//     }
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


int K3bInterferingSystemsHandler::startStopMediaNotifier( bool start )
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
    running = l.contains( QCString("medianotifier") );
  }
  else
    kdDebug() << "(K3bApplication) call to kded::loadedModules failed." << endl;


  if( start ) {
    if( running )
      return 0;

    // FIXME: kded crashes when we do this through dcopClient(), so for now we call the dcop
    //        command line tool
    KProcess p;
    p << "dcop" << "kded" << "kded" << "loadModule" << "medianotifier";
    if( p.start( KProcess::Block ) ) {
      //       QCString replyType;
      //       QByteArray replyData;
      //       if( dcopClient()->call( "kded", "kded", "loadModule(QCString)", QCString("medianotifier"), replyType, replyData, false, 10000 ) )
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
    p << "dcop" << "kded" << "kded" << "unloadModule" << "medianotifier";
    if( p.start( KProcess::Block ) ) {
      
      //       QByteArray data;
      //       QDataStream ds( data, IO_WriteOnly );
      //       ds << QCString("medianotifier");
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


// int K3bInterferingSystemsHandler::startStopAutomounting( bool start, K3bDevice::Device* dev )
// {
//   //
//   // here we simply call the script if we can find it and don't care if automounting is actually enabled
//   // or not since the script returns a proper error code telling us everything we need to know.
//   //
//   QString autoMountingScript = KStandardDirs::findExe( "k3b_automount" );
//   if( autoMountingScript.isEmpty() ) {
//     kdDebug() << "(K3bInterferingSystemsHandler) could not find the automounting script" << endl;
//     return -1;
//   }

//   KProcess p;
//   p << autoMountingScript;
//   if( start )
//     p << "enable";
//   else
//     p << "disable";
//   p << dev->blockDeviceName();

//   if( p.start( KProcess::Block ) ) {
//     if( p.normalExit() ) {
//       //
//       //  Exit codes:
//       //    0 - success
//       //    1 - wrong usage
//       //    2 - device not configured with subfs/supermount in /etc/fstab
//       //    X - failed to mount/umount
//       //
//       switch( p.exitStatus() ) {
//       case 0:
// 	return 1;
//       case 1:
// 	kdDebug() << "(K3bInterferingSystemsHandler) k3b_automount usage failure." << endl;
// 	return -1;
//       case 2:
// 	return 0;
//       default:
// 	kdDebug() << "(K3bInterferingSystemsHandler) some k3b_automount problem." << endl;
// 	return -1;
//       }
//     }
//     else {
//       kdDebug() << "(K3bInterferingSystemsHandler) the automounting script failed in some way." << endl;
//       return -1;
//     }
//   }
//   else {
//     kdDebug() << "(K3bInterferingSystemsHandler) could not start the automounting script." << endl;
//     return -1;
//   }
// }


int K3bInterferingSystemsHandler::blockUnblockPmount( bool block, K3bDevice::Device* dev )
{
  // pumount [-l] dev
  // pmount --lock dev pid
  // pmount --unlock dev pid
  QString pmountBin = K3b::findExe( "pmount" );
  QString pumountBin = K3b::findExe( "pumount" );

  //
  // If pmount is not installed blocking it would not make sense anyway ;)
  //
  if( pmountBin.isEmpty() || pumountBin.isEmpty() ) {
    kdDebug() << "(K3bInterferingSystemsHandler) could not find pmount/pumount." << endl;    
    return 0;
  }


  //
  // first unmount the device and ignore the outcome
  //
  if( block ) {
    KProcess p;
    p << pumountBin;
    // p << "-l"; // lazy unmount
    p << dev->blockDeviceName();
    p.start( KProcess::Block );
  }

  //
  // Now lock/unlock it
  //
  KProcess p;
  p << pmountBin;
  if( block )
    p << "--lock";
  else
    p << "--unlock";
  p << dev->blockDeviceName();
  p << QString::number( (int)::getpid() );
  if( p.start( KProcess::Block ) && p.normalExit() && p.exitStatus() == 0 ) {
    kdDebug() << "(K3bInterferingSystemsHandler) " << (block?"blocked":"unblocked") << " " << dev->blockDeviceName() << " with pid " << (int)::getpid() << endl;
    return 1;
  }
  else
    return -1;
}


// static
void K3bInterferingSystemsHandler::threadSafeDisable( K3bDevice::Device* dev )
{
  if( QThread::currentThread() == s_guiThreadHandle ) {
    instance()->disable( dev );
  }
  else {
    QWaitCondition w;
    
    QApplication::postEvent( K3bInterferingSystemsHandler::instance(),
			     new DisableEvent( true, dev, &w ) );
    
    w.wait();
  }
}


// static
void K3bInterferingSystemsHandler::threadSafeEnable( K3bDevice::Device* dev )
{
  if( QThread::currentThread() == s_guiThreadHandle ) {
    instance()->enable( dev );
  }
  else {
    QWaitCondition w;
    
    QApplication::postEvent( K3bInterferingSystemsHandler::instance(),
			     new DisableEvent( false, dev, &w ) );
    
    w.wait();
  }
}


void K3bInterferingSystemsHandler::customEvent( QCustomEvent* e )
{
  if( DisableEvent* de = dynamic_cast<DisableEvent*>(e) ) {
    if( de->disable )
      disable( de->device );
    else
      enable( de->device );
    de->wc->wakeAll();
  }
}


K3bInterferingSystemsHandler* K3bInterferingSystemsHandler::instance()
{
  static KStaticDeleter<K3bInterferingSystemsHandler> sd;

  if( !s_instance )
    sd.setObject( s_instance, new K3bInterferingSystemsHandler() );

  return s_instance;
}

#include "k3binterferingsystemshandler.moc"
