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

#include "k3bgrowisofswriter.h"

#include <k3bcore.h>
#include <k3bdevice.h>
#include <k3bdevicehandler.h>
#include <k3bprocess.h>
#include <k3bexternalbinmanager.h>
#include <k3bversion.h>
#include <k3bdiskinfo.h>
#include <k3bglobals.h>
#include <k3bthroughputestimator.h>
#include <k3bgrowisofshandler.h>

#include <klocale.h>
#include <kdebug.h>
#include <kconfig.h>
#include <kglobal.h>

#include <qvaluelist.h>
//#include <qdatetime.h>

#include <unistd.h>


class K3bGrowisofsWriter::Private
{
public:
  Private() 
    : writingMode( 0 ),
      process( 0 ),
      growisofsBin( 0 ),
      trackSize(-1),
      trackSizePadding(0) {
  }

  int writingMode;
  K3bProcess* process;
  const K3bExternalBin* growisofsBin;
  QString image;

  bool success;
  bool canceled;

  QTime lastSpeedCalculationTime;
  int lastSpeedCalculationBytes;
  int lastProgress;
  unsigned int lastProgressed;
  double lastWritingSpeed;

  bool writingStarted;

  K3bThroughputEstimator* speedEst;
  K3bGrowisofsHandler* gh;

  // used in DAO with growisofs >= 5.15
  long trackSize;
  long trackSizePadding;

  unsigned long long overallSizeFromOutput;
};


K3bGrowisofsWriter::K3bGrowisofsWriter( K3bCdDevice::CdDevice* dev, K3bJobHandler* hdl,
					QObject* parent, const char* name )
  : K3bAbstractWriter( dev, hdl, parent, name )
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
  connect( d->gh, SIGNAL(deviceBuffer(int)),
	   this, SIGNAL(deviceBuffer(int)) );
}


K3bGrowisofsWriter::~K3bGrowisofsWriter()
{
  delete d->process;
  delete d;
}


bool K3bGrowisofsWriter::active() const
{
  return (d->process ? d->process->isRunning() : false);
}


int K3bGrowisofsWriter::fd() const
{
  if( d->process )
    return d->process->stdinFd();
  else
    return -1;
}


bool K3bGrowisofsWriter::closeFd()
{
  // do the padding
  if( d->trackSizePadding > 0 ) {
    kdDebug() << "(K3bGrowisofsWriter) padding with " << d->trackSizePadding << " blocks." << endl;
    char buf[d->trackSizePadding*2048];
    ::memset( buf, 0, d->trackSizePadding*2048 );
    if( ::write( fd(), buf, d->trackSizePadding*2048 ) < d->trackSizePadding*2048 )
      kdDebug() << "(K3bGrowisofsWriter) FAILED to pad." << endl;
  }

  return ( !::close( fd() ) );
}


bool K3bGrowisofsWriter::prepareProcess()
{
  delete d->process;
  d->process = new K3bProcess();
  d->process->setRunPrivileged(true);
  //  d->process->setPriority( KProcess::PrioHighest );
  d->process->setSplitStdout(true);
  d->process->setRawStdin(true);
  connect( d->process, SIGNAL(stderrLine(const QString&)), this, SLOT(slotReceivedStderr(const QString&)) );
  connect( d->process, SIGNAL(stdoutLine(const QString&)), this, SLOT(slotReceivedStderr(const QString&)) );
  connect( d->process, SIGNAL(processExited(KProcess*)), this, SLOT(slotProcessExited(KProcess*)) );

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

  if( !d->growisofsBin->copyright.isEmpty() )
    emit infoMessage( i18n("Using %1 %2 - Copyright (C) %3").arg("growisofs").arg(d->growisofsBin->version).arg(d->growisofsBin->copyright), INFO );

  //
  // The growisofs bin is ready. Now we add the parameters
  //

  *d->process << d->growisofsBin;

  QString s = burnDevice()->blockDeviceName() + "=";
  if( d->image.isEmpty() )
    s += "/dev/fd/0";
  else
    s += d->image;

  // for now we do not support multisession
  *d->process << "-Z" << s;

  // now we use the force (luke ;) do not reload the dvd, K3b does that.
  *d->process << "-use-the-force-luke=notray";

  // we check for existing filesystems ourselves, so we always force the overwrite...
  *d->process << "-use-the-force-luke=tty";

  // if reading from an image let growisofs use the size of the image
  if( d->growisofsBin->version > K3bVersion( 5, 17, -1 ) && d->trackSize > 0 && d->image.isEmpty() )
    *d->process << "-use-the-force-luke=tracksize:" + QString::number(d->trackSize);

  // this only makes sense for DVD-R(W) media
  if( simulate() )
    *d->process << "-use-the-force-luke=dummy";
  if( d->writingMode == K3b::DAO ) {
    // if reading from an image let growisofs use the size of the image
    if( d->growisofsBin->version >= K3bVersion( 5, 15, -1 ) && d->trackSize > 0 && d->image.isEmpty() )
      *d->process << "-use-the-force-luke=dao:" + QString::number(d->trackSize);
    else
      *d->process << "-use-the-force-luke=dao";
    d->gh->reset(true);
  }
  else
    d->gh->reset(false);

  //
  // Some DVD writers do not allow changing the writing speed so we allow
  // the user to ignore the speed setting
  //
  int speed = burnSpeed();
  if( speed >= 0 ) {
    if( speed == 0 ) {
      // try to determine the writeSpeed
      // if it fails determineOptimalWriteSpeed() will return 0 and
      // the choice is left to growisofs which means that the choice is
      // really left to the drive since growisofs does not change the speed
      // if no option is given
      speed = burnDevice()->determineMaximalWriteSpeed();
    }
    
    if( speed != 0 )
      *d->process << QString("-speed=%1").arg( speed%1385 > 0
					      ? QString::number( (float)speed/1385.0, 'f', 1 )  // example: DVD+R(W): 2.4x
					      : QString::number( speed/1385 ) );
  }

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

  d->lastWritingSpeed = 0;
  d->lastProgressed = 0;
  d->lastProgress = 0;
  d->lastSpeedCalculationTime = QTime::currentTime();
  d->lastSpeedCalculationBytes = 0;
  d->writingStarted = false;
  d->canceled = false;
  d->speedEst->reset();

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
	emit infoMessage( i18n("Starting simulation..."), 
			  K3bJob::INFO );
      }
      else {
	emit newTask( i18n("Writing") );
	emit infoMessage( i18n("Starting writing..."), K3bJob::INFO );
      }
    }
  }
}


void K3bGrowisofsWriter::cancel()
{
  if( active() ) {
    d->canceled = true;
    closeFd();
    d->process->kill();
  }
}


void K3bGrowisofsWriter::setWritingMode( int m )
{
  d->writingMode = m;
}


void K3bGrowisofsWriter::setTrackSize( long size )
{
  // for now growisofs want it to be a multiple of 16 (1 ECC block: 16*2048 bytes)
  // although I don't think it's necessary we simply pad
  if( size % 16 ) {
    d->trackSizePadding = (16 - size%16);
    d->trackSize = size + d->trackSizePadding;
  }
  else {
    d->trackSizePadding = 0;
    d->trackSize = size;
  }
  kdDebug() << "(K3bGrowisofsWriter) need to pad " << d->trackSizePadding << " blocks." << endl;
}


void K3bGrowisofsWriter::setImageToWrite( const QString& filename )
{
  d->image = filename;
}


void K3bGrowisofsWriter::slotReceivedStderr( const QString& line )
{
  emit debuggingOutput( d->growisofsBin->name(), line );

  if( line.contains( "remaining" ) ) {

    if( !d->writingStarted ) {
      d->writingStarted = true;
      emit newSubTask( i18n("Writing data") );
    }
    
    // parse progress
    int pos = line.find( "/" );
    unsigned long long done = K3b::toULongLong( line.left( pos ) );  // TODO: for QT 3.2: toULongLong
    bool ok = true;
    d->overallSizeFromOutput = K3b::toULongLong( line.mid( pos+1, line.find( "(", pos ) - pos - 1 ), &ok ); // TODO: for QT 3.2: toULongLong
    if( ok ) {
      int p = (int)(100 * done / d->overallSizeFromOutput);
      if( p > d->lastProgress ) {
	emit percent( p );
	d->lastProgress = p;
      }
      if( (unsigned int)(done/1024/1024) > d->lastProgressed ) {
	d->lastProgressed = (unsigned int)(done/1024/1024);
	emit processedSize( d->lastProgressed, (int)(d->overallSizeFromOutput/1024/1024)  );
      }

      // try parsing write speed (since growisofs 5.11)
      pos = line.find( '@' );
      if( pos != -1 ) {
	pos += 1;
	double speed = line.mid( pos, line.find( 'x', pos ) - pos ).toDouble(&ok);
	if( ok ) {
	  if( d->lastWritingSpeed != speed )
	    emit writeSpeed( (int)(speed*1385.0), 1385 );
	  d->lastWritingSpeed = speed;
	}
	else
	  kdDebug() << "(K3bGrowisofsWriter) speed parsing failed: '" 
		    << line.mid( pos, line.find( 'x', pos ) - pos ) << "'" << endl;
      }
      else {
	d->speedEst->dataWritten( done/1024 );
      }
    }
    else
      kdDebug() << "(K3bGrowisofsWriter) progress parsing failed: '" 
		<< line.mid( pos+1, line.find( "(", pos ) - pos - 1 ).stripWhiteSpace() << "'" << endl;
  }

  else
    d->gh->handleLine( line );
}


void K3bGrowisofsWriter::slotProcessExited( KProcess* p )
{
  if( d->canceled ) {
    // this will unblock and eject the drive and emit the finished/canceled signals
    K3bAbstractWriter::cancel();
    return;
  }

  if( p->normalExit() ) {
    if( p->exitStatus() == 0 ) {

      // the output stops before 100%, so we do it manually
      emit percent( 100 );
      emit processedSize( d->overallSizeFromOutput/1024/1024,
			  d->overallSizeFromOutput/1024/2024 );

      int s = d->speedEst->average();
      if( s > 0 )
	emit infoMessage( i18n("Average overall write speed: %1 KB/s (%2x)").arg(s).arg(KGlobal::locale()->formatNumber((double)s/1385.0), 2), INFO );

      if( simulate() )
	emit infoMessage( i18n("Simulation successfully finished"), K3bJob::SUCCESS );
      else
	emit infoMessage( i18n("Writing successfully finished"), K3bJob::SUCCESS );

      d->success = true;
    }
    else {
      if( !wasSourceUnreadable() )
	d->gh->handleExit( p->exitStatus() );
      d->success = false;
    }
  }
  else {
    emit infoMessage( i18n("%1 did not exit cleanly.").arg(d->growisofsBin->name()), ERROR );
    d->success = false;
  }

  k3bcore->config()->setGroup("General Options");
  if( k3bcore->config()->readBoolEntry( "No cd eject", false ) )
    emit finished(d->success);
  else {
    emit newSubTask( i18n("Ejecting DVD") );
    connect( K3bCdDevice::eject( burnDevice() ), 
	     SIGNAL(finished(K3bCdDevice::DeviceHandler*)),
	     this, 
	     SLOT(slotEjectingFinished(K3bCdDevice::DeviceHandler*)) );
  }
}


void K3bGrowisofsWriter::slotEjectingFinished( K3bCdDevice::DeviceHandler* dh )
{
  if( !dh->success() )
    emit infoMessage( i18n("Unable to eject media."), ERROR );

  emit finished(d->success);
}


void K3bGrowisofsWriter::slotThroughput( int t )
{
  emit writeSpeed( t, 1385 );
}

#include "k3bgrowisofswriter.moc"
