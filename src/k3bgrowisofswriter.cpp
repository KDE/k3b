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

#include "k3bgrowisofswriter.h"

#include <k3bcore.h>
#include <device/k3bdevice.h>
#include <k3bprocess.h>
#include <k3bexternalbinmanager.h>
#include <k3bversion.h>
#include <device/k3bdiskinfo.h>
#include <k3bglobals.h>

#include <klocale.h>
#include <kdebug.h>
#include <kconfig.h>

#include <qvaluelist.h>

#include <errno.h>
#include <string.h>


class K3bGrowisofsWriter::Private
{
public:
  Private() 
    : writingMode( 0 ),
      process( 0 ),
      growisofsBin( 0 ) {
  }

  int writingMode;
  K3bProcess* process;
  const K3bExternalBin* growisofsBin;
  QString image;
};


K3bGrowisofsWriter::K3bGrowisofsWriter( K3bCdDevice::CdDevice* dev, QObject* parent, const char* name )
  : K3bAbstractWriter( dev, parent, name )
{
  d = new Private;
}


K3bGrowisofsWriter::~K3bGrowisofsWriter()
{
  delete d->process;
  delete d;
}


bool K3bGrowisofsWriter::write( const char* data, int len )
{
  if( d->process )
    return d->process->writeStdin( data, len );
  else
    return -1;
}


int K3bGrowisofsWriter::fd() const
{
  if( d->process )
    return d->process->stdin();
  else
    return -1;
}


bool K3bGrowisofsWriter::prepareProcess()
{
  delete d->process;
  d->process = new K3bProcess();
  d->process->setRunPrivileged(true);
  d->process->setSplitStdout(true);
  connect( d->process, SIGNAL(stdoutLine(const QString&)), this, SLOT(slotStdLine(const QString&)) );
  connect( d->process, SIGNAL(stderrLine(const QString&)), this, SLOT(slotStdLine(const QString&)) );
  connect( d->process, SIGNAL(processExited(KProcess*)), this, SLOT(slotProcessExited(KProcess*)) );
  connect( d->process, SIGNAL(wroteStdin(KProcess*)), this, SIGNAL(dataWritten()) );

  d->growisofsBin = k3bcore->externalBinManager()->binObject( "growisofs" );
  if( !d->growisofsBin ) {
    emit infoMessage( i18n("Could not find %1 executable.").arg("growisofs"), ERROR );
    return false;
  }
  
  if( d->growisofsBin->version < K3bVersion( 5, 10 ) ) {
    emit infoMessage( i18n("Growisofs version %1 is too old. "
			   "K3b needs at least version 5.10.").arg(d->growisofsBin->version), 
		      ERROR );
    return false;
  }

  //
  // The growisofs bin is ready. Now we add the parameters
  //

  *d->process << d->growisofsBin->path;

  // for now we do not support multisession
  *d->process << "-Z";
  QString s = burnDevice()->blockDeviceName() + "=";
  if( d->image.isEmpty() )
    s += "/dev/fd/0";  // read from stdin
  else
    s += d->image;

  // now we use the force (luke ;)
  *d->process << "-use-the-force-luke=notray";

  // this only makes sense for DVD-R(W) media
  // ----------------------------------------
  if( simulate() )
    *d->process << "-use-the-force-luke=dummy";
  if( d->writingMode == K3b::DAO )
    *d->process << "-use-the-force-luke=dao";  // does DAO apply to DVD+R?
  if( burnSpeed() != 0 )
    *d->process << QString("-speed=%1").arg(burnSpeed());
  // -------------------------------- DVD-R(W)

  if( k3bcore->config()->readBoolEntry( "Allow overburning", false ) )
    *d->process << "-overburn";


  // additional user parameters from config
  const QStringList& params = d->growisofsBin->userParameters();
  for( QStringList::const_iterator it = params.begin(); it != params.end(); ++it )
    *d->process << *it;

  return true;
}


void K3bGrowisofsWriter::start()
{
  emit started();

  if( !prepareProcess() ) {
    emit finished( false );
  }
  else {

    kdDebug() << "***** " << d->growisofsBin->name() << " parameters:\n";
    const QValueList<QCString>& args = d->process->args();
    QString s;
    for( QValueList<QCString>::const_iterator it = args.begin(); it != args.end(); ++it ) {
      s += *it + " ";
    }
    kdDebug() << s << flush << endl;
    emit debuggingOutput( d->growisofsBin->name() + " comand:", s);


    emit newSubTask( i18n("Preparing write process...") );

    if( !d->process->start( KProcess::NotifyOnExit, KProcess::All ) ) {
      // something went wrong when starting the program
      // it "should" be the executable
      kdDebug() << "(K3bGrowisofsWriter) could not start " << d->growisofsBin->path << endl;
      emit infoMessage( i18n("Could not start %1.").arg(d->growisofsBin->name()), K3bJob::ERROR );
      emit finished(false);
    }
    else {
      if( simulate() ) {
	emit newTask( i18n("Simulating") );
	emit infoMessage( i18n("Starting simulation at %1x speed...").arg(burnSpeed()), 
			  K3bJob::PROCESS );
	//
	// TODO: info message that DVD+R(W) has no dummy mode and the speed setting is not used
	//       perhaps we could determine the media type in the writer?
	//
      }
      else {
	emit newTask( i18n("Writing") );
	emit infoMessage( i18n("Starting writing at %1x speed...").arg(burnSpeed()), K3bJob::PROCESS );
      }
    }
  }
}


void K3bGrowisofsWriter::cancel()
{
  if( d->process ) {
    if( d->process->isRunning() ) {
      d->process->disconnect();
      d->process->kill();

      // this will unblock and eject the drive and emit the finished/canceled signals
      K3bAbstractWriter::cancel();
    }
  }
}


void K3bGrowisofsWriter::setWritingMode( int m )
{
  d->writingMode = m;
}


void K3bGrowisofsWriter::setImageToWrite( const QString& filename )
{
  d->image = filename;
}


void K3bGrowisofsWriter::slotStdLine( const QString& line )
{
  emit debuggingOutput( d->growisofsBin->name(), line );

  if( line.contains( "remaining" ) ) {
    // parse progress
    int pos = line.find( "/" );
    int done = line.left( pos ).toInt();
    int size = line.mid( pos+1, line.find( "(", pos ) - pos - 1 ).toInt();

    emit percent( 100 * done / size );
    emit processedSize( done/1024/1024, size/1024/1024  );
  }
  else if( line.contains( "flushing cache" ) ) {
    emit infoMessage( i18n("Flushing Cache"), PROCESS );
  }
  else if( line.contains( "updating RMA" ) ) {
    emit infoMessage( i18n("Updating RMA"), PROCESS );
  }
  else if( line.contains( "closing session" ) ) {
    emit infoMessage( i18n("Closing Session"), PROCESS );
  }
  else {
    kdDebug() << "(growisofs) " << line << endl;
  }
}


void K3bGrowisofsWriter::slotProcessExited( KProcess* p )
{
  // TODO: eject the dvd if configured

  if( p->normalExit() ) {
    if( p->exitStatus() == 0 ) {
      if( simulate() )
	emit infoMessage( i18n("Simulation successfully finished"), K3bJob::STATUS );
      else
	emit infoMessage( i18n("Writing successfully finished"), K3bJob::STATUS );

      createAverageWriteSpeedInfoMessage();
      emit finished( true );
    }
    else {
      //
      // The growisofs error codes:
      //
      // 128 + errno: fatal error upon program startup
      // 
      //
      
      if( p->exitStatus() > 128 ) {
	// for now we just emit a message with the error
	// in the future when I know more about what kinds of errors may occure
	// we will enhance this
	emit infoMessage( i18n("Fatal error: %1 (%2)").arg(strerror(errno)).arg(errno), ERROR );
      }
      else if( p->exitStatus() == 1 ) {
	emit infoMessage( i18n("I/O Error"), ERROR );
      }
      else {
	emit infoMessage( i18n("%1 returned an unknown error (code %2).").arg(d->growisofsBin->name()).arg(p->exitStatus()), 
			  K3bJob::ERROR );
	emit infoMessage( strerror(p->exitStatus()), K3bJob::ERROR );
	emit infoMessage( i18n("Please send me an email with the last output."), K3bJob::ERROR );
      }

      emit finished( false );
    }
  }
  else {
    emit infoMessage( i18n("%1 did not exit cleanly.").arg(d->growisofsBin->name()), 
		      ERROR );
    emit finished( false );
  }
}


#include "k3bgrowisofswriter.moc"
