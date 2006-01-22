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
#include "k3bgrowisofshandler.h"
#include <k3bpipebuffer.h>
#include <k3bglobalsettings.h>

#include <klocale.h>
#include <kdebug.h>
#include <kglobal.h>

#include <qvaluelist.h>
#include <qfile.h>

#include <unistd.h>


class K3bGrowisofsWriter::Private
{
public:
  Private() 
    : writingMode( 0 ),
      closeDvd(false),
      process( 0 ),
      growisofsBin( 0 ),
      trackSize(-1),
      layerBreak(0),
      usingRingBuffer(false),
      ringBuffer(0) {
  }

  int writingMode;
  bool closeDvd;
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

  long layerBreak;

  unsigned long long overallSizeFromOutput;

  QFile inputFile;

  bool usingRingBuffer;
  K3bPipeBuffer* ringBuffer;
};


K3bGrowisofsWriter::K3bGrowisofsWriter( K3bDevice::Device* dev, K3bJobHandler* hdl,
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
  connect( d->gh, SIGNAL(buffer(int)),
	   this, SIGNAL(buffer(int)) );
  connect( d->gh, SIGNAL(deviceBuffer(int)),
	   this, SIGNAL(deviceBuffer(int)) );
  connect( d->gh, SIGNAL(flushingCache()),
	   this, SLOT(slotFlushingCache()) );
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
  if( d->process ) {
    if( d->usingRingBuffer )
      return d->ringBuffer->inFd();
    else
      return d->process->stdinFd();
  }
  else
    return -1;
}


bool K3bGrowisofsWriter::closeFd()
{
  return ( !::close( fd() ) );
}


bool K3bGrowisofsWriter::prepareProcess()
{
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

  emit debuggingOutput( "Used versions", "growisofs: " + d->growisofsBin->version );

  if( !d->growisofsBin->copyright.isEmpty() )
    emit infoMessage( i18n("Using %1 %2 - Copyright (C) %3").arg("growisofs")
		      .arg(d->growisofsBin->version).arg(d->growisofsBin->copyright), INFO );


  //
  // The growisofs bin is ready. Now we add the parameters
  //
  delete d->process;
  d->process = new K3bProcess();
  d->process->setRunPrivileged(true);
  //  d->process->setPriority( KProcess::PrioHighest );
  d->process->setSplitStdout(true);
  d->process->setRawStdin(true);
  connect( d->process, SIGNAL(stderrLine(const QString&)), this, SLOT(slotReceivedStderr(const QString&)) );
  connect( d->process, SIGNAL(stdoutLine(const QString&)), this, SLOT(slotReceivedStderr(const QString&)) );
  connect( d->process, SIGNAL(processExited(KProcess*)), this, SLOT(slotProcessExited(KProcess*)) );


  //
  // growisofs < 5.20 wants the tracksize to be a multiple of 16 (1 ECC block: 16*2048 bytes)
  // we simply pad ourselves.
  //
  // But since the writer itself properly pads or writes a longer lead-out we don't really need
  // to write zeros. We just tell growisofs to reserve a multiple of 16 blocks.
  // This is only releveant in DAO mode anyway.
  //
  int trackSizePadding = 0;
  if( d->trackSize > 0 && d->growisofsBin->version < K3bVersion( 5, 20 ) ) {
    if( d->trackSize % 16 ) {
      trackSizePadding = (16 - d->trackSize%16);
      kdDebug() << "(K3bGrowisofsWriter) need to pad " << trackSizePadding << " blocks." << endl;
    }
  }


  *d->process << d->growisofsBin;

  // set this var to true to enable the ringbuffer
  d->usingRingBuffer = ( d->growisofsBin->version < K3bVersion( 6, 0 ) );

  QString s = burnDevice()->blockDeviceName() + "=";
  if( d->usingRingBuffer || d->image.isEmpty() ) {
    // we always read from stdin since the ringbuffer does the actual reading from the source
    s += "/dev/fd/0";
  }
  else
    s += d->image;

  // for now we do not support multisession
  *d->process << "-Z" << s;


  if( !d->image.isEmpty() && d->usingRingBuffer ) {
    d->inputFile.setName( d->image );
    d->trackSize = (K3b::filesize( d->image ) + 1024) / 2048;
    if( !d->inputFile.open( IO_ReadOnly ) ) {
      emit infoMessage( i18n("Could not open file %1.").arg(d->image), ERROR );
      return false;
    }
  }

  // now we use the force (luke ;) do not reload the dvd, K3b does that.
  *d->process << "-use-the-force-luke=notray";

  // we check for existing filesystems ourselves, so we always force the overwrite...
  *d->process << "-use-the-force-luke=tty";

  bool dvdCompat = d->closeDvd;

  // DL writing with forced layer break
  if( d->layerBreak > 0 ) {
    *d->process << "-use-the-force-luke=break:" + QString::number(d->layerBreak);
    dvdCompat = true;
  }

  // the tracksize parameter takes priority over the dao:tracksize parameter since growisofs 5.18
  else if( d->growisofsBin->version > K3bVersion( 5, 17 ) && d->trackSize > 0 )
    *d->process << "-use-the-force-luke=tracksize:" + QString::number(d->trackSize + trackSizePadding);
  
  if( simulate() )
    *d->process << "-use-the-force-luke=dummy";

  if( d->writingMode == K3b::DAO ) {
    dvdCompat = true;
    if( d->growisofsBin->version >= K3bVersion( 5, 15 ) && d->trackSize > 0 )
      *d->process << "-use-the-force-luke=dao:" + QString::number(d->trackSize + trackSizePadding);
    else
      *d->process << "-use-the-force-luke=dao";
    d->gh->reset( burnDevice(), true );
  }
  else
    d->gh->reset( burnDevice(), false );

  if( dvdCompat )
    *d->process << "-dvd-compat";

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

    // speed may be a float number. example: DVD+R(W): 2.4x    
    if( speed != 0 )
      *d->process << QString("-speed=%1").arg( speed%1385 > 0
					      ? QString::number( (float)speed/1385.0, 'f', 1 )
					      : QString::number( speed/1385 ) );
  }

  if( k3bcore->globalSettings()->overburn() )
    *d->process << "-overburn";

  if( !d->usingRingBuffer && d->growisofsBin->version >= K3bVersion( 6, 0 ) ) {
    bool manualBufferSize = k3bcore->globalSettings()->useManualBufferSize();
    int bufSize = ( manualBufferSize ? k3bcore->globalSettings()->bufferSize() : 40 );
    *d->process << QString("-use-the-force-luke=bufsize:%1m").arg(bufSize);
  }

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
    emit debuggingOutput( d->growisofsBin->name() + " command:", s);


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

      d->gh->handleStart();

      // create the ring buffer
      if( d->usingRingBuffer ) {
	if( !d->ringBuffer ) {
	  d->ringBuffer = new K3bPipeBuffer( this, this );
	  connect( d->ringBuffer, SIGNAL(percent(int)), this, SIGNAL(buffer(int)) );
	  connect( d->ringBuffer, SIGNAL(finished(bool)), this, SLOT(slotRingBufferFinished(bool)) );
	}

	d->ringBuffer->writeToFd( d->process->stdinFd() );
	bool manualBufferSize = k3bcore->globalSettings()->useManualBufferSize();
	int bufSize = ( manualBufferSize ? k3bcore->globalSettings()->bufferSize() : 4 );
	d->ringBuffer->setBufferSize( bufSize );

	if( !d->image.isEmpty() )
	  d->ringBuffer->readFromFd( d->inputFile.handle() );

	d->ringBuffer->start();
      }
    }
  }
}


void K3bGrowisofsWriter::cancel()
{
  if( active() ) {
    d->canceled = true;
    closeFd();
    if( d->usingRingBuffer && d->ringBuffer )
      d->ringBuffer->cancel();
    d->process->kill();
  }
}


void K3bGrowisofsWriter::setWritingMode( int m )
{
  d->writingMode = m;
}


void K3bGrowisofsWriter::setTrackSize( long size )
{
  d->trackSize = size;
}


void K3bGrowisofsWriter::setLayerBreak( long lb )
{
  d->layerBreak = lb;
}


void K3bGrowisofsWriter::setCloseDvd( bool b )
{
  d->closeDvd = b;
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
    unsigned long long done = line.left( pos ).toULongLong();
    bool ok = true;
    d->overallSizeFromOutput = line.mid( pos+1, line.find( "(", pos ) - pos - 1 ).toULongLong( &ok );
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

  d->gh->handleLine( line );
}


void K3bGrowisofsWriter::slotProcessExited( KProcess* p )
{
  d->inputFile.close();

  if( d->canceled ) {
    if( !d->usingRingBuffer || !d->ringBuffer->active() ) {
      // this will unblock and eject the drive and emit the finished/canceled signals
      K3bAbstractWriter::cancel();
    }
    return;
  }

  if( p->exitStatus() == 0 ) {

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

  if( !k3bcore->globalSettings()->ejectMedia() )
    emit finished(d->success);
  else {
    emit newSubTask( i18n("Ejecting DVD") );
    connect( K3bDevice::eject( burnDevice() ), 
	     SIGNAL(finished(K3bDevice::DeviceHandler*)),
	     this, 
	     SLOT(slotEjectingFinished(K3bDevice::DeviceHandler*)) );
  }
}


void K3bGrowisofsWriter::slotRingBufferFinished( bool )
{
  if( d->canceled && !d->process->isRunning() ) {
    // this will unblock and eject the drive and emit the finished/canceled signals
    K3bAbstractWriter::cancel();
  }
}


void K3bGrowisofsWriter::slotEjectingFinished( K3bDevice::DeviceHandler* dh )
{
  if( !dh->success() )
    emit infoMessage( i18n("Unable to eject media."), ERROR );

  emit finished(d->success);
}


void K3bGrowisofsWriter::slotThroughput( int t )
{
  emit writeSpeed( t, 1385 );
}


void K3bGrowisofsWriter::slotFlushingCache()
{
  if( !d->canceled ) {
    //
    // growisofs's progress output stops before 100%, so we do it manually
    //
    emit percent( 100 );
    emit processedSize( d->overallSizeFromOutput/1024/1024,
			d->overallSizeFromOutput/1024/1024 );
  }
}

#include "k3bgrowisofswriter.moc"
