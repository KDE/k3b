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


#include "k3bgrowisofsimager.h"
#include "k3bdvddoc.h"

#include <k3bcore.h>
#include <k3bexternalbinmanager.h>
#include <device/k3bdevice.h>
#include <device/k3bdevicemanager.h>
#include <device/k3bdevicehandler.h>
#include <k3bprocess.h>
#include <k3bglobals.h>

#include <kdebug.h>
#include <klocale.h>
#include <kconfig.h>

#include <qvaluelist.h>

#include <errno.h>
#include <string.h>


class K3bGrowisofsImager::Private
{
public:
  bool success;
};



K3bGrowisofsImager::K3bGrowisofsImager( K3bDvdDoc* doc, QObject* parent, const char* name )
  : K3bIsoImager( doc, parent, name ),
    m_doc(doc)
{
  d = new Private;
}


K3bGrowisofsImager::~K3bGrowisofsImager()
{
  delete d;
}


void K3bGrowisofsImager::start()
{
  emit started();

  cleanup();
  init();

  m_process = new K3bProcess();
  m_growisofsBin = k3bcore->externalBinManager()->binObject( "growisofs" );
  m_mkisofsBin = k3bcore->externalBinManager()->binObject( "mkisofs" );
  if( !m_growisofsBin ) {
    emit infoMessage( i18n("Could not find %1 executable.").arg("growisofs"), ERROR );
    emit finished( false );
    return;
  }
  if( !m_mkisofsBin ) {
    emit infoMessage( i18n("Could not find %1 executable.").arg("mkisofs"), ERROR );
    emit finished( false );
    return;
  }
  
  if( m_growisofsBin->version < K3bVersion( 5, 10 ) ) {
    emit infoMessage( i18n("Growisofs version %1 is too old. "
			   "K3b needs at least version 5.10.").arg(m_growisofsBin->version), 
		      ERROR );
    emit finished( false );
    return;
  }
  if( m_doc->multiSessionMode() != K3bDataDoc::NONE ) {
    if( m_mkisofsBin->version < K3bVersion( 2, 0 ) ) {
      emit infoMessage( i18n("Mkisofs version %1 is too old. "
			     "For writing multisession DVDs "
			     "K3b needs at least version 2.0.").arg(m_mkisofsBin->version), 
			ERROR );
      emit finished( false );
      return;
    }
  }

  if( !m_growisofsBin->copyright.isEmpty() )
    emit infoMessage( i18n("Using %1 %2 - Copyright (C) %3").arg("growisofs").arg(m_growisofsBin->version).arg(m_growisofsBin->copyright), INFO );
  if( !m_mkisofsBin->copyright.isEmpty() )
    emit infoMessage( i18n("Using %1 %2 - Copyright (C) %3").arg("mkisofs").arg(m_mkisofsBin->version).arg(m_mkisofsBin->copyright), INFO );

  //
  // As growisofs calls mkisofs we need to make sure it calls "our" version.
  // This can be done through a environment variable MKISOFS
  //
  m_process->setEnvironment( "MKISOFS", m_mkisofsBin->path );

  *m_process << m_growisofsBin->path;

  //
  // add the growisofs options
  //
  if( m_doc->multiSessionMode() == K3bDataDoc::NONE )
    *m_process << "-Z";
  else
    *m_process << "-M";
  *m_process << m_doc->burner()->blockDeviceName();

  // now we use the force (luke ;)
  *m_process << "-use-the-force-luke=notray";

  // this only makes sense for DVD-R(W) media
  // ----------------------------------------
  if( m_doc->dummy() )
    *m_process << "-use-the-force-luke=dummy";
  if( m_doc->writingMode() == K3b::DAO && m_doc->multiSessionMode() == K3bDataDoc::NONE )
    *m_process << "-use-the-force-luke=dao";  // does DAO apply to DVD+R?
  if( m_doc->speed() != 0 )
    *m_process << QString("-speed=%1").arg(m_doc->speed());
  // -------------------------------- DVD-R(W)

  if( k3bcore->config()->readBoolEntry( "Allow overburning", false ) )
    *m_process << "-overburn";


  // additional user parameters from config
  const QStringList& params = m_growisofsBin->userParameters();
  for( QStringList::const_iterator it = params.begin(); it != params.end(); ++it )
    *m_process << *it;


  //
  // now add the mkisofs options
  //

  if( !prepareMkisofsFiles() || 
      !addMkisofsParameters() ) {
    cleanup();
    emit finished( false );
    return;
  }

  connect( m_process, SIGNAL(processExited(KProcess*)),
	   this, SLOT(slotProcessExited(KProcess*)) );

  connect( m_process, SIGNAL(stderrLine( const QString& )),
	   this, SLOT(slotReceivedStderr( const QString& )) );


  kdDebug() << "***** growisofs parameters:\n";
  const QValueList<QCString>& args = m_process->args();
  QString s;
  for( QValueList<QCString>::const_iterator it = args.begin(); it != args.end(); ++it ) {
    s += *it + " ";
  }
  kdDebug() << s << endl << flush;
  emit debuggingOutput("growisofs comand:", s);


  emit newSubTask( i18n("Preparing write process...") );

  if( !m_process->start( KProcess::NotifyOnExit, KProcess::AllOutput) ) {
    // something went wrong when starting the program
    // it "should" be the executable
    cleanup();
    kdDebug() << "(K3bIsoImager) could not start growisofs" << endl;
    emit infoMessage( i18n("Could not start %1.").arg("growisofs"), K3bJob::ERROR );
    emit finished( false );
  }
  else {
    if( m_doc->dummy() ) {
      emit newTask( i18n("Simulating") );
      emit infoMessage( i18n("Starting simulation at %1x speed...").arg(m_doc->speed()), 
			K3bJob::PROCESS );
      //
      // TODO: info message that DVD+R(W) has no dummy mode and the speed setting is not used
      //       perhaps we could determine the media type in the writer?
      //
    }
    else {
      emit newTask( i18n("Writing") );
      emit infoMessage( i18n("Starting writing at %1x speed...").arg(m_doc->speed()), K3bJob::PROCESS );
    }
  }
}


void K3bGrowisofsImager::slotReceivedStderr( const QString& line )
{
  // all we need to do is parsing any status messges from growisofs
  // progress parsing can be done by the K3bIsoImager

  emit debuggingOutput( "growisofs", line );

  if( line.contains( "done, estimate" ) ) {
    K3bIsoImager::parseProgress( line );
  }
  else if( line.contains( "flushing cache" ) ) {
    emit newSubTask( i18n("Flushing Cache") );
  }
  else if( line.contains( "updating RMA" ) ) {
    emit newSubTask( i18n("Updating RMA") );
  }
  else if( line.contains( "closing session" ) ) {
    emit newSubTask( i18n("Closing Session") );
  }
  else {
    kdDebug() << "(growisofs) " << line << endl;
  }
}


void K3bGrowisofsImager::slotProcessExited( KProcess* p )
{
  //
  // This is more or less the same as in K3bGrowisofsWriter. :(
  //

  cleanup();

  if( m_canceled ) {
    emit canceled();
    d->success = false;
  }
  else if( p->normalExit() ) {
    if( p->exitStatus() == 0 ) {
      if( m_doc->dummy() )
	emit infoMessage( i18n("Simulation successfully finished"), K3bJob::STATUS );
      else
	emit infoMessage( i18n("Writing successfully finished"), K3bJob::STATUS );

      d->success = true;
    }
    else {
      //
      // The growisofs error codes:
      //
      // 128 + errno: fatal error upon program startup
      // errno      : fatal error during recording
      //
      
      if( p->exitStatus() > 128 ) {
	// for now we just emit a message with the error
	// in the future when I know more about what kinds of errors may occure
	// we will enhance this
	emit infoMessage( i18n("Fatal error at startup: %1").arg(strerror(p->exitStatus()-128)), 
			  ERROR );
      }
      else if( p->exitStatus() == 1 ) {
	// Doku says: warning at exit
	// Example: mkisofs error
	//          unable to reload
	// So basicly this is just for mkisofs failure since we do not let growisofs reload the media
	emit infoMessage( i18n("Warning at exit: (1)"), ERROR );
	emit infoMessage( i18n("Most likely mkisofs failed in some way."), ERROR );
      }
      else {
	emit infoMessage( i18n("Fatal error during recording: %1").arg(strerror(p->exitStatus())), 
			  ERROR );
      }

      d->success = false;
    }
  }
  else {
    emit infoMessage( i18n("%1 did not exit cleanly.").arg(m_growisofsBin->name()), 
		      ERROR );
    d->success = false;
  }

  k3bcore->config()->setGroup("General Options");
  if( k3bcore->config()->readBoolEntry( "No cd eject", false ) )
    emit finished(d->success);
  else {
    emit infoMessage( i18n("Ejecting CD..."), INFO );
    connect( K3bCdDevice::eject( m_doc->burner() ), 
	     SIGNAL(finished(K3bCdDevice::DeviceHandler*)),
	     this, 
	     SLOT(slotEjectingFinished(K3bCdDevice::DeviceHandler*)) );
  }
}


void K3bGrowisofsImager::slotEjectingFinished( K3bCdDevice::DeviceHandler* dh )
{
  if( !dh->success() )
    emit infoMessage( "Unable to eject media.", ERROR );

  emit finished(d->success);
}


void K3bGrowisofsImager::cancel()
{
  m_canceled = true;

  if( m_process ) {
    if( !m_processExited ) {
      m_process->kill();
    }
  }
}

#include "k3bgrowisofsimager.moc"
