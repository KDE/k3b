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


#include "k3breadcdreader.h"

#include <k3bcore.h>
#include <k3bexternalbinmanager.h>
#include <device/k3bdevice.h>
#include <device/k3bdevicemanager.h>
#include <k3bprocess.h>

#include <kdebug.h>
#include <klocale.h>
#include <kconfig.h>

#include <qregexp.h>
#include <qvaluelist.h>
#include <qstringlist.h>


K3bReadcdReader::K3bReadcdReader( QObject* parent, const char* name )
  : K3bJob( parent, name ),
    m_process(0),
    m_noCorr(false),
    m_clone(false),
    m_noError(false),
    m_c2Scan(false),
    m_speed(0),
    m_fdToWriteTo(-1)
{
}


K3bReadcdReader::~K3bReadcdReader()
{
  if( m_process )
    delete m_process;
}


void K3bReadcdReader::writeToFd( int fd )
{
  m_fdToWriteTo = fd;
}


void K3bReadcdReader::start()
{
  m_blocksToRead = 1;
  m_unreadableBlocks = 0;

  // the first thing to do is to check for readcd
  m_readcdBinObject = k3bcore->externalBinManager()->binObject( "readcd" );
  if( !m_readcdBinObject ) {
    emit infoMessage( i18n("Could not find %1 executable.").arg("readcd"), ERROR );
    emit finished(false);
    return;
  }

  // check if we have clone support if we need it
  if( m_clone ) {
    bool foundCloneSupport = false;

    if( !m_readcdBinObject->hasFeature( "clone" ) ) {
      // search all readcd installations
      K3bExternalProgram* readcdProgram = k3bcore->externalBinManager()->program( "readcd" );
      const QPtrList<K3bExternalBin>& readcdBins = readcdProgram->bins();
      for( QPtrListIterator<K3bExternalBin> it( readcdBins ); it.current(); ++it ) {
	if( it.current()->hasFeature( "clone" ) ) {
	  m_readcdBinObject = it.current();
	  emit infoMessage( i18n("Using readcd %1 instead of default version for clone support.").arg(m_readcdBinObject->version), INFO );
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
  if( m_process )
    delete m_process;
  m_process = new K3bProcess();
  connect( m_process, SIGNAL(stderrLine(const QString&)), this, SLOT(slotStdLine(const QString&)) );
  connect( m_process, SIGNAL(processExited(KProcess*)), this, SLOT(slotProcessExited(KProcess*)) );


  *m_process << m_readcdBinObject->path;

  // display progress
  *m_process << "-v";

  // Again we assume the device to be set!
  *m_process << QString("dev=%1").arg(K3bCdDevice::externalBinDeviceParameter(m_readDevice, 
									      m_readcdBinObject));
  if( m_speed > 0 )
    *m_process << QString("speed=%1").arg(m_speed);


  // output
  if( m_fdToWriteTo != -1 ) {
    *m_process << "f=-";
    m_process->dupStdout( m_fdToWriteTo );
  }
  else {
    emit newTask( i18n("Writing image to %1.").arg(m_imagePath) );
    emit infoMessage( i18n("Writing image to %1.").arg(m_imagePath), INFO );
    *m_process << "f=" + m_imagePath;
  }


  if( m_noError )
    *m_process << "-noerror";
  if( m_clone ) {
    *m_process << "-clone";
    // noCorr can only be used with cloning
    if( m_noCorr )
      *m_process << "-nocorr";
  }
  if( m_c2Scan )
    *m_process << "-c2scan";


  // additional user parameters from config
  const QStringList& params = m_readcdBinObject->userParameters();
  for( QStringList::const_iterator it = params.begin(); it != params.end(); ++it )
    *m_process << *it;


  kdDebug() << "***** readcd parameters:\n";
  const QValueList<QCString>& args = m_process->args();
  QString s;
  for( QValueList<QCString>::const_iterator it = args.begin(); it != args.end(); ++it ) {
    s += *it + " ";
  }
  kdDebug() << s << endl << flush;

  emit debuggingOutput("readcd comand:", s);

  m_canceled = false;

  if( !m_process->start( KProcess::NotifyOnExit, KProcess::AllOutput) ) {
    // something went wrong when starting the program
    // it "should" be the executable
    kdError() << "(K3bReadcdReader) could not start readcd" << endl;
    emit infoMessage( i18n("Could not start readcd."), K3bJob::ERROR );
    emit finished( false );
  }
}


void K3bReadcdReader::cancel()
{
  if( m_process ) {
    if( m_process->isRunning() ) {  
      m_canceled = true;
      m_process->kill();
    }
  }
}


void K3bReadcdReader::slotStdLine( const QString& line )
{
  emit debuggingOutput( "readcd", line );

  int pos = -1;

  if( line.startsWith( "end:" ) ) {
    bool ok;
    m_blocksToRead = line.mid(4).toInt(&ok);
    if( !ok )
      kdError() << "(K3bReadcdReader) blocksToRead parsing error in line: " 
		<< line.mid(4) << endl;
  }

  else if( line.startsWith( "addr:" ) ) {
    bool ok;
    long currentReadBlock = line.mid( 6, line.find("cnt")-7 ).toInt(&ok);
    if( ok ) {
      emit percent( (int)(100.0 * (double)currentReadBlock / (double)m_blocksToRead) );
      emit processedSize( currentReadBlock*2/1024, m_blocksToRead*2/1024 );
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
    int problemSector = line.mid( pos, line.find( QRegExp("\\D"), pos )-pos-1 ).toInt(&ok);
    if( !ok ) {
      kdError() << "(K3bReadcdReader) problemSector parsing error in line: " 
		<< line.mid( pos, line.find( QRegExp("\\D"), pos )-pos-1 ) << endl;
    }
    emit infoMessage( i18n("Retrying from sector %1.").arg(problemSector), PROCESS );
  }

  else if( (pos = line.find("Error on sector")) >= 0 ) {
    m_unreadableBlocks++;

    pos += 16;
    bool ok;
    int problemSector = line.mid( pos, line.find( QRegExp("\\D"), pos )-pos-1 ).toInt(&ok);
    if( !ok ) {
      kdError() << "(K3bReadcdReader) problemSector parsing error in line: " 
		<< line.mid( pos, line.find( QRegExp("\\D"), pos )-pos-1 ) << endl;
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
  if( m_canceled ) {
    emit canceled();
    emit finished(false);
  }
  else if( p->normalExit() ) {
    if( p->exitStatus() == 0 ) {
      emit finished( true );
    }
    else {
      emit infoMessage( i18n("Readcd returned error: %1").arg(p->exitStatus()), ERROR );
      emit finished( false );
    }
  }
  else {
    emit infoMessage( i18n("Readcd exited abnormally."), ERROR );
    emit finished( false );
  }
}



#include "k3breadcdreader.moc"

