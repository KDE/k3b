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


#include "k3breadcdreader.h"

#include <k3bcore.h>
#include <k3bexternalbinmanager.h>
#include <k3bdevice.h>
#include <k3bdevicemanager.h>
#include <k3bprocess.h>
#include <k3bmsf.h>
#include <k3bglobals.h>

#include <kdebug.h>
#include <klocale.h>
#include <kconfig.h>

#include <qregexp.h>
#include <qvaluelist.h>
#include <qstringlist.h>



class K3bReadcdReader::Private
{
public:
  Private() 
    : process(0),
      fdToWriteTo(-1),
      canceled(false) {
  }

  K3b::Msf firstSector, lastSector;

  K3bProcess* process;
  const K3bExternalBin* readcdBinObject;

  int fdToWriteTo;
  bool canceled;

  long blocksToRead;
  int unreadableBlocks;

  int lastProgress;
  int lastProcessedSize;
};



K3bReadcdReader::K3bReadcdReader( K3bJobHandler* jh, QObject* parent, const char* name )
  : K3bJob( jh, parent, name ),
    m_noCorr(false),
    m_clone(false),
    m_noError(false),
    m_c2Scan(false),
    m_speed(0),
    m_retries(128)
{
  d = new Private();
}


K3bReadcdReader::~K3bReadcdReader()
{
  delete d->process;
  delete d;
}


bool K3bReadcdReader::active() const
{
  return (d->process ? d->process->isRunning() : false);
}


void K3bReadcdReader::writeToFd( int fd )
{
  d->fdToWriteTo = fd;
}


void K3bReadcdReader::start()
{
  d->blocksToRead = 1;
  d->unreadableBlocks = 0;
  d->lastProgress = 0;
  d->lastProcessedSize = 0;

  // the first thing to do is to check for readcd
  d->readcdBinObject = k3bcore->externalBinManager()->binObject( "readcd" );
  if( !d->readcdBinObject ) {
    emit infoMessage( i18n("Could not find %1 executable.").arg("readcd"), ERROR );
    emit finished(false);
    return;
  }

  // check if we have clone support if we need it
  if( m_clone ) {
    bool foundCloneSupport = false;

    if( !d->readcdBinObject->hasFeature( "clone" ) ) {
      // search all readcd installations
      K3bExternalProgram* readcdProgram = k3bcore->externalBinManager()->program( "readcd" );
      const QPtrList<K3bExternalBin>& readcdBins = readcdProgram->bins();
      for( QPtrListIterator<K3bExternalBin> it( readcdBins ); it.current(); ++it ) {
	if( it.current()->hasFeature( "clone" ) ) {
	  d->readcdBinObject = it.current();
	  emit infoMessage( i18n("Using readcd %1 instead of default version for clone support.").arg(d->readcdBinObject->version), INFO );
	  foundCloneSupport = true;
	  break;
	}
      }

      if( !foundCloneSupport ) {
	emit infoMessage( i18n("Could not find readcd executable with cloning support."), ERROR );
	emit finished(false);
	return;
      }
    }
  }


  // create the commandline
  delete d->process;
  d->process = new K3bProcess();
  connect( d->process, SIGNAL(stderrLine(const QString&)), this, SLOT(slotStdLine(const QString&)) );
  connect( d->process, SIGNAL(processExited(KProcess*)), this, SLOT(slotProcessExited(KProcess*)) );


  *d->process << d->readcdBinObject;

  // display progress
  *d->process << "-v";

  // Again we assume the device to be set!
  *d->process << QString("dev=%1").arg(K3b::externalBinDeviceParameter(m_readDevice, 
									      d->readcdBinObject));
  if( m_speed > 0 )
    *d->process << QString("speed=%1").arg(m_speed);


  // output
  if( d->fdToWriteTo != -1 ) {
    *d->process << "f=-";
    d->process->dupStdout( d->fdToWriteTo );
  }
  else {
    emit newTask( i18n("Writing image to %1.").arg(m_imagePath) );
    emit infoMessage( i18n("Writing image to %1.").arg(m_imagePath), INFO );
    *d->process << "f=" + m_imagePath;
  }


  if( m_noError )
    *d->process << "-noerror";
  if( m_clone ) {
    *d->process << "-clone";
    // noCorr can only be used with cloning
    if( m_noCorr )
      *d->process << "-nocorr";
  }
  if( m_c2Scan )
    *d->process << "-c2scan";

  *d->process << QString("retries=%1").arg(m_retries);

  // readcd does not read the last sector specified
  if( d->firstSector < d->lastSector )
    *d->process << QString("sectors=%1-%2").arg(d->firstSector.lba()).arg(d->lastSector.lba()+1);

  // additional user parameters from config
  const QStringList& params = d->readcdBinObject->userParameters();
  for( QStringList::const_iterator it = params.begin(); it != params.end(); ++it )
    *d->process << *it;


  kdDebug() << "***** readcd parameters:\n";
  const QValueList<QCString>& args = d->process->args();
  QString s;
  for( QValueList<QCString>::const_iterator it = args.begin(); it != args.end(); ++it ) {
    s += *it + " ";
  }
  kdDebug() << s << endl << flush;

  emit debuggingOutput("readcd comand:", s);

  d->canceled = false;

  if( !d->process->start( KProcess::NotifyOnExit, KProcess::AllOutput) ) {
    // something went wrong when starting the program
    // it "should" be the executable
    kdError() << "(K3bReadcdReader) could not start readcd" << endl;
    emit infoMessage( i18n("Could not start readcd."), K3bJob::ERROR );
    emit finished( false );
  }
}


void K3bReadcdReader::cancel()
{
  if( d->process ) {
    if( d->process->isRunning() ) {  
      d->canceled = true;
      d->process->kill();
    }
  }
}


void K3bReadcdReader::slotStdLine( const QString& line )
{
  emit debuggingOutput( "readcd", line );

  int pos = -1;

  if( line.startsWith( "end:" ) ) {
    bool ok;
    d->blocksToRead = line.mid(4).toInt(&ok);
    if( d->firstSector < d->lastSector )
      d->blocksToRead -= d->firstSector.lba();
    if( !ok )
      kdError() << "(K3bReadcdReader) blocksToRead parsing error in line: " 
		<< line.mid(4) << endl;
  }

  else if( line.startsWith( "addr:" ) ) {
    bool ok;
    long currentReadBlock = line.mid( 6, line.find("cnt")-7 ).toInt(&ok);
    if( d->firstSector < d->lastSector )
      currentReadBlock -= d->firstSector.lba();
    if( ok ) {
      int p = (int)(100.0 * (double)currentReadBlock / (double)d->blocksToRead);
      if( p > d->lastProgress ) {
	emit percent( p );
	d->lastProgress = p;
      }
      int ps = currentReadBlock*2/1024;
      if( ps > d->lastProcessedSize ) {
	emit processedSize( ps, d->blocksToRead*2/1024 );
	d->lastProcessedSize = ps;
      }
    }
    else
      kdError() << "(K3bReadcdReader) currentReadBlock parsing error in line: " 
		<< line.mid( 6, line.find("cnt")-7 ) << endl;
  }

  else if( line.contains("Cannot read source disk") ) {
    emit infoMessage( i18n("Cannot read source disk."), ERROR );
  }

  else if( (pos = line.find("Retrying from sector")) >= 0 ) {
    // parse the sector
    pos += 21;
    bool ok;
    int problemSector = line.mid( pos, line.find( QRegExp("\\D"), pos )-pos ).toInt(&ok);
    if( !ok ) {
      kdError() << "(K3bReadcdReader) problemSector parsing error in line: " 
		<< line.mid( pos, line.find( QRegExp("\\D"), pos )-pos ) << endl;
    }
    emit infoMessage( i18n("Retrying from sector %1.").arg(problemSector), INFO );
  }

  else if( (pos = line.find("Error on sector")) >= 0 ) {
    d->unreadableBlocks++;

    pos += 16;
    bool ok;
    int problemSector = line.mid( pos, line.find( QRegExp("\\D"), pos )-pos ).toInt(&ok);
    if( !ok ) {
      kdError() << "(K3bReadcdReader) problemSector parsing error in line: " 
		<< line.mid( pos, line.find( QRegExp("\\D"), pos )-pos ) << endl;
    }

    if( line.contains( "not corrected") ) {
      emit infoMessage( i18n("Uncorrected error in sector %1").arg(problemSector), ERROR );
    }
    else {
      emit infoMessage( i18n("Corrected error in sector %1").arg(problemSector), ERROR );
    }
  }

  else {
    kdDebug() << "(readcd) " << line << endl;
  }
}

void K3bReadcdReader::slotProcessExited( KProcess* p )
{
  if( d->canceled ) {
    emit canceled();
    emit finished(false);
  }
  else if( p->normalExit() ) {
    if( p->exitStatus() == 0 ) {
      emit finished( true );
    }
    else {
      emit infoMessage( i18n("%1 returned error: %2").arg("Readcd").arg(p->exitStatus()), ERROR );
      emit finished( false );
    }
  }
  else {
    emit infoMessage( i18n("Readcd exited abnormally."), ERROR );
    emit finished( false );
  }
}


void K3bReadcdReader::setSectorRange( const K3b::Msf& first, const K3b::Msf& last )
{
  d->firstSector = first;
  d->lastSector = last;
}

#include "k3breadcdreader.moc"

