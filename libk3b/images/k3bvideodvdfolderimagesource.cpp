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

#include "k3bvideodvdfolderimagesource.h"
#include "k3bvideodvdfolderreader.h"

#include <k3bexternalbinmanager.h>
#include <k3bcore.h>
#include <k3bprocess.h>

#include <klocale.h>
#include <kdebug.h>

#include <qvaluelist.h>

#include <errno.h>
#include <string.h>


class K3bVideoDVDFolderImageSource::Private
{
public:
  Private()
    : mkisofsBin( 0 ),
      mkisofsProcess( 0 ) {
  }

  QString volumeId;
  QString pathToVideoDVD;

  const K3bExternalBin* mkisofsBin;
  K3bProcess* mkisofsProcess;

  bool canceled;
};


K3bVideoDVDFolderImageSource::K3bVideoDVDFolderImageSource( const K3bVideoDVDFolderReader* reader, K3bJobHandler* hdl, QObject* parent )
  : K3bImageSource( hdl, parent )
{
  d = new Private();

  setVolumeId( reader->volumeId() );
  d->pathToVideoDVD = reader->imageFileName();
}


K3bVideoDVDFolderImageSource::~K3bVideoDVDFolderImageSource()
{
  delete d->mkisofsProcess;
  delete d;
}


void K3bVideoDVDFolderImageSource::setVolumeId( const QString& id )
{
  d->volumeId = id;
}


void K3bVideoDVDFolderImageSource::start()
{
  jobStarted();

  d->canceled = false;

  // init the K3bMkisofsHandler (this will emit an error message in case it fails)
  d->mkisofsBin = initMkisofs();
  if( !d->mkisofsBin ) {
    jobFinished( false );
    return;
  }

  delete d->mkisofsProcess;
  d->mkisofsProcess = createMkisofsProcess( false );
  if( !d->mkisofsProcess ) {
    jobFinished( false );
    return;
  }

  // output some debugging information
  emit debuggingOutput( "Used versions", "mkisofs: " + d->mkisofsBin->version );

  // conect the process to our slots
  connect( d->mkisofsProcess, SIGNAL(processExited(KProcess*)),
	   this, SLOT(slotMkisofsExited(KProcess*)) );
  connect( d->mkisofsProcess, SIGNAL(stderrLine( const QString& )),
	   this, SLOT(slotMkisofsStderr( const QString& )) );

  // pipe mkisofs's stdout directly into our fd
  if( fdToWriteTo() != -1 )
    d->mkisofsProcess->writeToFd( fdToWriteTo() );
  else
    d->mkisofsProcess->setRawStdout( true );

  // debug parameters
  kdDebug() << "***** mkisofs parameters:\n";
  const QValueList<QCString>& args = d->mkisofsProcess->args();
  QString s;
  for( QValueList<QCString>::const_iterator it = args.begin(); it != args.end(); ++it ) {
    s += *it + " ";
  }
  kdDebug() << s << endl << flush;
  emit debuggingOutput("mkisofs command:", s);

  // start mkisofs
  if( !d->mkisofsProcess->start( KProcess::NotifyOnExit, KProcess::AllOutput) ) {
    // something went wrong when starting the program
    // it "should" be the executable
    kdDebug() << "(K3bIsoImager) could not start mkisofs" << endl;
    emit infoMessage( i18n("Could not start %1.").arg("mkisofs"), K3bJob::ERROR );
    jobFinished( false );
  }
}


void K3bVideoDVDFolderImageSource::cancel()
{
  if( d->mkisofsProcess && d->mkisofsProcess->isRunning() ) {
    d->canceled = true;
    d->mkisofsProcess->kill();
  }
}


void K3bVideoDVDFolderImageSource::slotMkisofsStderr( const QString& line )
{
  parseMkisofsOutput( line );
  emit debuggingOutput( "mkisofs", line );
}


void K3bVideoDVDFolderImageSource::slotMkisofsExited( KProcess* p )
{
  if( d->canceled ) {
    emit canceled();
    jobFinished( false );
  }
  else {
    if( p->normalExit() ) {
      if( p->exitStatus() == 0 ) {
	jobFinished( true );
      }
      else  {
	switch( p->exitStatus() ) {
	case 104:
	  // connection reset by peer
	  // This only happens if cdrecord does not finish successfully
	  // so we may leave the error handling to it meaning we handle this
	  // as a known error
	  break;

	default:
	  if( !mkisofsReadError() ) {
	    emit infoMessage( i18n("%1 returned an unknown error (code %2).").arg("mkisofs").arg(p->exitStatus()),
			      K3bJob::ERROR );
	    emit infoMessage( strerror(p->exitStatus()), K3bJob::ERROR );
	    emit infoMessage( i18n("Please send me an email with the last output."), K3bJob::ERROR );
	  }
	}

	jobFinished( false );
      }
    }
    else {
      emit infoMessage( i18n("%1 did not exit cleanly.").arg("mkisofs"), ERROR );
      jobFinished( false );
    }
  }
}


void K3bVideoDVDFolderImageSource::handleMkisofsProgress( int p )
{
  emit percent( p );
}


void K3bVideoDVDFolderImageSource::handleMkisofsInfoMessage( const QString& message, int type )
{
  emit infoMessage( message, type );
}


void K3bVideoDVDFolderImageSource::determineSize()
{
  //
  // The output parsing is the same as in K3bIsoImager
  // See k3bisoimage.cpp for more comments
  //

  int blocks = 0;
  bool success = false;

  if( K3bProcess* p = createMkisofsProcess( true ) ) {
    K3bProcessOutputCollector opc( p );

    // a VideoDVD contains a very small number of files. So -print-size
    // will be very fast and KProcess::Block should not be a problem    
    if( p->start( KProcess::Block, KProcess::AllOutput ) ) {
      if( !opc.stdout().isEmpty() ) {
	blocks = opc.stdout().toInt( &success );
      }
      else {
	// parse the stderr output
	// I hope parsing the last line is enough!
	int pos = opc.stderr().findRev( "extents scheduled to be written" );
	
	if( pos == -1 )
	  success = false;
	else
	  blocks = opc.stderr().mid( pos+33 ).toInt( &success );
      }
    }
    else {
      kdDebug() << "(K3bVideoDVDFolderImageSource) unable to start mkisofs." << endl;
    }
    
    delete p;
    
    if( success ) {
      K3bDevice::Toc toc;
      toc.append( K3bDevice::Track( 0, blocks-1, K3bDevice::Track::DATA, K3bDevice::Track::MODE1 ) );
      setToc( toc );
    }
    else
      kdDebug() << "(K3bVideoDVDFolderImageSource) failed to determine size with mkisofs." << endl;
  }

  emit tocReady( success );
}


K3bProcess* K3bVideoDVDFolderImageSource::createMkisofsProcess( bool printSize )
{
  K3bProcess* p = new K3bProcess();

  // add the path to the bin
  *p << d->mkisofsBin;

  // allow proper output parsing
  *p << "-gui";

  // the name of the DVD
  *p << "-volid" << d->volumeId;

  // a little advertising ;)
  *p << "-appid" << "K3B THE CD KREATOR (C) 1998-2005 SEBASTIAN TRUEG AND THE K3B TEAM";

  // we want to create a video dvd image with udf and stuff
  *p << "-dvd-video";

  if( printSize )
    *p << "-print-size" << "-quiet";

  // and the thing we want to write
  *p << d->pathToVideoDVD;

  return p;
}


long K3bVideoDVDFolderImageSource::read( char* data, long maxLen )
{
  // error handlig will be done in slotMkisofsExited
  return ::read( d->mkisofsProcess->stdoutFd(), data, maxLen );
}

#include "k3bvideodvdfolderimagesource.moc"
