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


#include "k3bgrowisofsimager.h"
#include "k3bdvddoc.h"

#include <k3bcore.h>
#include <k3bexternalbinmanager.h>
#include <k3bdevice.h>
#include <k3bdevicemanager.h>
#include <k3bdevicehandler.h>
#include <k3bprocess.h>
#include <k3bglobals.h>
#include <k3bthroughputestimator.h>
#include <k3bgrowisofshandler.h>

#include <kdebug.h>
#include <klocale.h>
#include <kconfig.h>
#include <kglobal.h>

#include <qvaluelist.h>



class K3bGrowisofsImager::Private
{
public:
  bool success;

  // speed calculation
  K3bThroughputEstimator* speedEst;

  bool writingStarted;

  int lastPercent;
  int lastProcessedSize;

  K3bGrowisofsHandler* gh;
};



K3bGrowisofsImager::K3bGrowisofsImager( K3bDataDoc* doc, K3bJobHandler* jh, QObject* parent, const char* name )
  : K3bIsoImager( doc, jh, parent, name ),
    m_doc(doc)
{
  d = new Private;
  d->speedEst = new K3bThroughputEstimator( this );
  connect( d->speedEst, SIGNAL(throughput(int)),
	   this, SLOT(slotThroughput(int)) );

  d->gh = new K3bGrowisofsHandler( this );
  connect( d->gh, SIGNAL(infoMessage(const QString&, int)),
	   this,SIGNAL(infoMessage(const QString&, int)) );
  connect( d->gh, SIGNAL(newSubTask(const QString&)),
	   this, SIGNAL(newSubTask(const QString&)) );
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

  d->speedEst->reset();
  d->writingStarted = false;
  d->lastProcessedSize = d->lastPercent = 0;

  m_process = new K3bProcess();
  m_process->setRunPrivileged(true);
  //  m_process->setPriority( KProcess::PrioHighest );
  m_process->setSuppressEmptyLines(true);

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

  *m_process << m_growisofsBin;

  //
  // add the growisofs options
  //
  if( m_doc->multiSessionMode() == K3bDataDoc::NONE ||
      m_doc->multiSessionMode() == K3bDataDoc::START )
    *m_process << "-Z";
  else
    *m_process << "-M";
  *m_process << m_doc->burner()->blockDeviceName();

  // now we use the force (luke ;)
  *m_process << "-use-the-force-luke=notray";

  // we check for existing filesystems ourselves, so we always force the overwrite...
  *m_process << "-use-the-force-luke=tty";

  // this only makes sense for DVD-R(W) media
  if( m_doc->dummy() )
    *m_process << "-use-the-force-luke=dummy";
  if( ( m_doc->writingMode() == K3b::DAO || m_doc->writingMode() == K3b::WRITING_MODE_AUTO )
      && m_doc->multiSessionMode() == K3bDataDoc::NONE ) {
    *m_process << "-use-the-force-luke=dao";  // does DAO apply to DVD+R? no.
    d->gh->reset( true );
  }
  else
    d->gh->reset( false );

  //
  // Some DVD writers do not allow changing the writing speed so we allow
  // the user to ignore the speed setting
  //
  int speed = m_doc->speed();
  if( speed >= 0 ) {
    if( speed == 0 ) {
      // try to determine the writeSpeed
      // if it fails determineMaxWriteSpeed() will return 0 and
      // the choice is left to growisofs which means that the choice is
      // really left to the drive since growisofs does not change the speed
      // if no option is given
      speed = m_doc->burner()->determineMaximalWriteSpeed();
    }
    
    if( speed != 0 )
      *m_process << QString("-speed=%1").arg( speed%1385 > 0
					      ? QString::number( (float)speed/1385.0, 'f', 1 )  // example: DVD+R(W): 2.4x
					      : QString::number( speed/1385 ) );
  }

  if( k3bcore->config()->readBoolEntry( "Allow overburning", false ) )
    *m_process << "-overburn";


  // additional user parameters from config
  const QStringList& params = m_growisofsBin->userParameters();
  for( QStringList::const_iterator it = params.begin(); it != params.end(); ++it )
    *m_process << *it;


  // prepare the filenames as written to the image
  m_doc->prepareFilenames();

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


  if( m_doc->needToCutFilenames() )
    emit infoMessage( i18n("Some filenames need to be shortened due to the %1 char restriction "
			   "of the Joliet extensions.")
		      .arg( m_doc->isoOptions().jolietLong() ? 103 : 64 ), 
		      WARNING );


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
      emit infoMessage( i18n("Starting simulation..."), 
			K3bJob::INFO );
      //
      // TODO: info message that DVD+R(W) has no dummy mode and the speed setting is not used
      //       perhaps we could determine the media type in the writer?
      //
    }
    else {
      emit newTask( i18n("Writing") );
      emit infoMessage( i18n("Starting writing..."), K3bJob::INFO );
    }
  }
}


void K3bGrowisofsImager::slotReceivedStderr( const QString& line )
{
  // all we need to do is parsing any status messges from growisofs
  // progress parsing can be done by the K3bIsoImager


  emit debuggingOutput( "growisofs", line );

  if( line.contains( "done, estimate" ) ) {

    if( !d->writingStarted ) {
      d->writingStarted = true;
      emit newSubTask( i18n("Writing data") );
    }

    //
    // Here we use m_doc->burningSize() to prevent wrong values when the user
    // imported a session.
    //

    int p = K3bIsoImager::parseProgress( line );
    if( p != -1 ) {
      d->speedEst->dataWritten( p*m_doc->burningSize()/1024/100 );
      if( p > d->lastPercent ) {
	emit percent( p );
	d->lastPercent = p;
      }
      int ps = p*m_doc->size()/1024/1024/100;
      if( ps > d->lastProcessedSize ) {
	emit processedSize( ps, m_doc->burningSize()/1024/1024 );
	d->lastProcessedSize = ps;
      }
    }
  }

  else
    d->gh->handleLine( line );
}


void K3bGrowisofsImager::slotProcessExited( KProcess* p )
{
  m_processExited = true;

  cleanup();

  if( m_canceled ) {
    emit canceled();
    d->success = false;
  }

  else if( p->normalExit() ) {
    if( p->exitStatus() == 0 ) {

      //
      // growisofs/mkisofs normally end the output at 99.98 or something like that.
      // So here we make sure we get to 100%
      //
      emit processedSize( m_doc->burningSize()/1024/1024, m_doc->burningSize()/1024/1024 );
      emit percent( 100 );

      int av = d->speedEst->average();
      emit infoMessage( i18n("Average overall write speed: %1 KB/s (%2x)")
			.arg(av).arg(KGlobal::locale()->formatNumber((double)av/1385.0, 2)), INFO );

      if( m_doc->dummy() )
	emit infoMessage( i18n("Simulation successfully finished"), K3bJob::SUCCESS );
      else
	emit infoMessage( i18n("Writing successfully finished"), K3bJob::SUCCESS );

      d->success = true;
    }
    else {
      d->gh->handleExit( p->exitStatus() );
      d->success = false;
    }
  }
  else {
    emit infoMessage( i18n("%1 did not exit cleanly.").arg(m_growisofsBin->name()), ERROR );
    d->success = false;
  }

  k3bcore->config()->setGroup("General Options");
  if( k3bcore->config()->readBoolEntry( "No cd eject", false ) )
    emit finished(d->success);
  else {
    emit newSubTask( i18n("Ejecting DVD") );
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


void K3bGrowisofsImager::slotThroughput( int t )
{
  emit writeSpeed( t, 1385 );
}

#include "k3bgrowisofsimager.moc"
