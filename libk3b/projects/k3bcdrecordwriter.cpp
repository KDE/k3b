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

#include <config.h>


#include "k3bcdrecordwriter.h"

#include <k3bcore.h>
#include <k3bexternalbinmanager.h>
#include <k3bprocess.h>
#include <k3bdevice.h>
#include <k3bdevicemanager.h>
#include <k3bdevicehandler.h>
#include <k3bglobals.h>
#include <k3bthroughputestimator.h>
#include <k3bglobalsettings.h>
#include <k3binterferingsystemshandler.h>

#include <qstring.h>
#include <qstringlist.h>
#include <qurl.h>
#include <qvaluelist.h>
#include <qregexp.h>
#include <qfile.h>

#include <klocale.h>
#include <kdebug.h>
#include <ktempfile.h>
#include <kglobal.h>

#include <errno.h>
#include <string.h>



class K3bCdrecordWriter::Private
{
public:
  Private()
    : cdTextFile(0) {
  }

  K3bThroughputEstimator* speedEst;
  bool canceled;
  bool usingBurnfree;
  int usedSpeed;

  struct Track {
    int size;
    bool audio;
  };

  QValueList<Track> tracks;

  KTempFile* cdTextFile;

  K3bInterferingSystemsHandler* interferingSystemHndl;
};


K3bCdrecordWriter::K3bCdrecordWriter( K3bDevice::Device* dev, K3bJobHandler* hdl, 
				      QObject* parent, const char* name )
  : K3bAbstractWriter( dev, hdl, parent, name ),
    m_clone(false),
    m_cue(false),
    m_forceNoEject(false)
{
  d = new Private();
  d->speedEst = new K3bThroughputEstimator( this );
  connect( d->speedEst, SIGNAL(throughput(int)),
	   this, SLOT(slotThroughput(int)) );

  d->interferingSystemHndl = new K3bInterferingSystemsHandler( this, this );
  connect( d->interferingSystemHndl, SIGNAL(infoMessage(const QString&, int)),
	   this, SIGNAL(infoMessage(const QString&, int)) );

  m_process = 0;
  m_writingMode = K3b::TAO;
}


K3bCdrecordWriter::~K3bCdrecordWriter()
{
  delete d->cdTextFile;
  delete d;
  delete m_process;
}


bool K3bCdrecordWriter::active() const
{
  return ( m_process && m_process->isRunning() );
}


int K3bCdrecordWriter::fd() const
{
  if( m_process )
    return m_process->stdinFd();
  else
    return -1;
}


void K3bCdrecordWriter::setDao( bool b )
{
  m_writingMode = ( b ? K3b::DAO : K3b::TAO );
}

void K3bCdrecordWriter::setCueFile( const QString& s)
{
  m_cue = true;
  m_cueFile = s;

  // cuefile only works in DAO mode
  setWritingMode( K3b::DAO );
}
  
void K3bCdrecordWriter::setClone( bool b )
{
  m_clone = b;
}


void K3bCdrecordWriter::setWritingMode( int mode )
{
  if( mode == K3b::DAO ||
      mode == K3b::TAO ||
      mode == K3b::RAW )
    m_writingMode = mode;
  else 
    kdError() << "(K3bCdrecordWriter) wrong writing mode: " << mode << endl;
}


void K3bCdrecordWriter::prepareProcess()
{
  if( m_process ) delete m_process;  // kdelibs want this!
  m_process = new K3bProcess();
  m_process->setRunPrivileged(true);
  //  m_process->setPriority( KProcess::PrioHighest );
  m_process->setSplitStdout(true);
  m_process->setSuppressEmptyLines(true);
  m_process->setRawStdin(true);  // we only use stdin when writing on-the-fly
  connect( m_process, SIGNAL(stdoutLine(const QString&)), this, SLOT(slotStdLine(const QString&)) );
  connect( m_process, SIGNAL(stderrLine(const QString&)), this, SLOT(slotStdLine(const QString&)) );
  connect( m_process, SIGNAL(processExited(KProcess*)), this, SLOT(slotProcessExited(KProcess*)) );

  m_cdrecordBinObject = k3bcore->externalBinManager()->binObject("cdrecord");

  if( !m_cdrecordBinObject )
    return;

  *m_process << m_cdrecordBinObject;

  // display progress
  *m_process << "-v";
    
  if( m_cdrecordBinObject->hasFeature( "gracetime") )
    *m_process << "gracetime=2";  // 2 is the lowest allowed value (Joerg, why do you do this to us?)
    
  // Again we assume the device to be set!
  *m_process << QString("dev=%1").arg(K3b::externalBinDeviceParameter(burnDevice(), m_cdrecordBinObject));

  d->usedSpeed = burnSpeed();
  if( d->usedSpeed == 0 ) {
    // try to determine the writeSpeed
    // if it fails determineMaximalWriteSpeed() will return 0 and
    // the choice is left to cdrecord
    d->usedSpeed = burnDevice()->determineMaximalWriteSpeed();
  }
  d->usedSpeed /= 175;
  if( d->usedSpeed != 0 )
    *m_process << QString("speed=%1").arg(d->usedSpeed);
    
  if( m_writingMode == K3b::DAO || m_cue ) {
    if( burnDevice()->dao() )
      *m_process << "-dao";
    else {
      if( m_cdrecordBinObject->hasFeature( "tao" ) )
	*m_process << "-tao";
      emit infoMessage( i18n("Writer does not support disk at once (DAO) recording"), WARNING );
    }
  }
  else if( m_writingMode == K3b::RAW ) {
    if( burnDevice()->supportsWritingMode( K3bDevice::RAW_R96R ) )
      *m_process << "-raw96r";
    else if( burnDevice()->supportsWritingMode( K3bDevice::RAW_R16 ) )
      *m_process << "-raw16";
    else if( burnDevice()->supportsWritingMode( K3bDevice::RAW_R96P ) )
      *m_process << "-raw96p";
    else {
      emit infoMessage( i18n("Writer does not support raw writing."), WARNING );
      if( m_cdrecordBinObject->hasFeature( "tao" ) )
	*m_process << "-tao";
    }
  }
  else if( m_cdrecordBinObject->hasFeature( "tao" ) )
    *m_process << "-tao";

  if( simulate() )
    *m_process << "-dummy";
    
  d->usingBurnfree = false;
  if( k3bcore->globalSettings()->burnfree() ) {
    if( burnDevice()->burnproof() ) {

      d->usingBurnfree = true;

      // with cdrecord 1.11a02 burnproof was renamed to burnfree
      if( m_cdrecordBinObject->version < K3bVersion( "1.11a02" ) )
	*m_process << "driveropts=burnproof";
      else
	*m_process << "driveropts=burnfree";
    }
    else
      emit infoMessage( i18n("Writer does not support buffer underrun free recording (Burnfree)"), WARNING );
  }
  
  if( m_cue ) {
    m_process->setWorkingDirectory(QUrl(m_cueFile).dirPath());
    *m_process << QString("cuefile=%1").arg( m_cueFile );
  }
  
  if( m_clone )
    *m_process << "-clone";
  
  if( m_rawCdText.size() > 0 ) {
    delete d->cdTextFile;
    d->cdTextFile = new KTempFile( QString::null, ".dat" );
    d->cdTextFile->setAutoDelete(true);
    d->cdTextFile->file()->writeBlock( m_rawCdText );
    d->cdTextFile->close();
    
    *m_process << "textfile=" + d->cdTextFile->name();
  }

  if( k3bcore->globalSettings()->ejectMedia() &&
      !m_forceNoEject )
    *m_process << "-eject";

  bool manualBufferSize = k3bcore->globalSettings()->useManualBufferSize();
  if( manualBufferSize ) {
    *m_process << QString("fs=%1m").arg( k3bcore->globalSettings()->bufferSize() );
  }
    
  bool overburn = k3bcore->globalSettings()->overburn();
  if( overburn )
    if( m_cdrecordBinObject->hasFeature("overburn") )
      *m_process << "-overburn";
    else
      emit infoMessage( i18n("Cdrecord %1 does not support overburning.").arg(m_cdrecordBinObject->version), WARNING );
    
  // additional user parameters from config
  const QStringList& params = m_cdrecordBinObject->userParameters();
  for( QStringList::const_iterator it = params.begin(); it != params.end(); ++it )
    *m_process << *it;

  // add the user parameters
  for( QStringList::const_iterator it = m_arguments.begin(); it != m_arguments.end(); ++it )
    *m_process << *it;
}


K3bCdrecordWriter* K3bCdrecordWriter::addArgument( const QString& arg )
{
  m_arguments.append( arg );
  return this;
}


void K3bCdrecordWriter::clearArguments()
{
  m_arguments.clear();
}


void K3bCdrecordWriter::start()
{
  jobStarted();

  d->canceled = false;
  d->speedEst->reset();

  prepareProcess();

  if( !m_cdrecordBinObject ) {
    emit infoMessage( i18n("Could not find %1 executable.").arg("cdrecord"), ERROR );
    jobFinished(false);
    return;
  }

  emit debuggingOutput( "Used versions", "cdrecord: " + m_cdrecordBinObject->version );

  if( !m_cdrecordBinObject->copyright.isEmpty() )
    emit infoMessage( i18n("Using %1 %2 - Copyright (C) %3").arg(m_cdrecordBinObject->name()).arg(m_cdrecordBinObject->version).arg(m_cdrecordBinObject->copyright), INFO );


  kdDebug() << "***** " << m_cdrecordBinObject->name() << " parameters:\n";
  const QValueList<QCString>& args = m_process->args();
  QString s;
  for( QValueList<QCString>::const_iterator it = args.begin(); it != args.end(); ++it ) {
    s += *it + " ";
  }
  kdDebug() << s << flush << endl;
  emit debuggingOutput( m_cdrecordBinObject->name() + " command:", s);

  m_currentTrack = 0;
  m_cdrecordError = UNKNOWN;
  m_totalTracksParsed = false;
  m_alreadyWritten = 0;
  d->tracks.clear();
  m_totalSize = 0;

  emit newSubTask( i18n("Preparing write process...") );

  // FIXME: check the return value
  k3bcore->blockDevice( burnDevice() );

  d->interferingSystemHndl->disable( burnDevice() );

  if( !m_process->start( KProcess::NotifyOnExit, KProcess::All ) ) {
    // something went wrong when starting the program
    // it "should" be the executable
    kdDebug() << "(K3bCdrecordWriter) could not start " << m_cdrecordBinObject->name() << endl;
    emit infoMessage( i18n("Could not start %1.").arg(m_cdrecordBinObject->name()), K3bJob::ERROR );
    jobFinished(false);
  }
  else {
    if( simulate() ) {
      emit newTask( i18n("Simulating") );
      emit infoMessage( i18n("Starting %1 simulation at %2x speed...")
			.arg(K3b::writingModeString(m_writingMode))
			.arg(d->usedSpeed), 
			K3bJob::INFO );
    }
    else {
      emit newTask( i18n("Writing") );
      emit infoMessage( i18n("Starting %1 writing at %2x speed...")
			.arg(K3b::writingModeString(m_writingMode))
			.arg(d->usedSpeed), 
			K3bJob::INFO );
    }
  }
}


void K3bCdrecordWriter::cancel()
{
  if( active() ) {
    d->canceled = true;
    if( m_process && m_process->isRunning() )
      m_process->kill();
  }
}


void K3bCdrecordWriter::slotStdLine( const QString& line )
{
  static QRegExp s_burnfreeCounterRx( "^BURN\\-Free\\swas\\s(\\d+)\\stimes\\sused" );
  static QRegExp s_burnfreeCounterRxPredict( "^Total\\sof\\s(\\d+)\\s\\spossible\\sbuffer\\sunderruns\\spredicted" );

  // tracknumber: cap(1)
  // done: cap(2)
  // complete: cap(3)
  // fifo: cap(4)  (it seems as if some patched cdrecord versions do not emit the fifo info but only the buf... :(
  // buffer: cap(5)
  static QRegExp s_progressRx( "Track\\s(\\d\\d)\\:\\s*(\\d*)\\sof\\s*(\\d*)\\sMB\\swritten\\s(?:\\(fifo\\s*(\\d*)\\%\\)\\s*)?(?:\\[buf\\s*(\\d*)\\%\\])?.*" );

  emit debuggingOutput( m_cdrecordBinObject->name(), line );
  
  //
  // Progress and toc parsing
  //

  if( line.startsWith( "Track " ) ) {
    if( !m_totalTracksParsed ) {
      // this is not the progress display but the list of tracks that will get written
      // we always extract the tracknumber to get the highest at last
      bool ok;
      int tt = line.mid( 6, 2 ).toInt(&ok);

      if( ok ) {
	struct Private::Track track;
	track.audio  = ( line.mid( 10, 5 ) == "audio" );

	m_totalTracks = tt;

	int sizeStart = line.find( QRegExp("\\d"), 10 );
	int sizeEnd = line.find( "MB", sizeStart );
	track.size = line.mid( sizeStart, sizeEnd-sizeStart ).toInt(&ok);
	  
	if( ok ) {
	  d->tracks.append(track);
	  m_totalSize += track.size;
	}
	else
	  kdDebug() << "(K3bCdrecordWriter) track number parse error: " 
		    << line.mid( sizeStart, sizeEnd-sizeStart ) << endl;
      }
      else
	kdDebug() << "(K3bCdrecordWriter) track number parse error: " 
		  << line.mid( 6, 2 ) << endl;
    }

    else if( s_progressRx.exactMatch( line ) ) {
      //      int num = s_progressRx.cap(1).toInt();
      int made = s_progressRx.cap(2).toInt();
      int size = s_progressRx.cap(3).toInt();
      int fifo = s_progressRx.cap(4).toInt();

      emit buffer( fifo );
      m_lastFifoValue = fifo;

      if( s_progressRx.numCaptures() > 4 )
	emit deviceBuffer( s_progressRx.cap(5).toInt() );

      //
      // cdrecord's output sucks a bit.
      // we get track sizes that differ from the sizes in the progress
      // info since these are dependant on the writing mode.
      // so we just use the track sizes and do a bit of math...
      //

      if( d->tracks.count() > m_currentTrack-1 && size > 0 ) {
	double convV = (double)d->tracks[m_currentTrack-1].size/(double)size;
	made = (int)((double)made * convV);
	size = d->tracks[m_currentTrack-1].size;
      }
      else {
	kdError() << "(K3bCdrecordWriter) Did not parse all tracks sizes!" << endl;
      }

      if( size > 0 ) {
	emit processedSubSize( made, size );
	emit subPercent( 100*made/size );
      }

      if( m_totalSize > 0 ) {
	emit processedSize( m_alreadyWritten+made, m_totalSize );
	emit percent( 100*(m_alreadyWritten+made)/m_totalSize );
      }

      d->speedEst->dataWritten( (m_alreadyWritten+made)*1024 );
    }
  }

  //
  // Cdrecord starts all error and warning messages with it's path
  // With Debian's script it starts with cdrecord (or /usr/bin/cdrecord or whatever! I hate this script!)
  //

  else if( line.startsWith( "cdrecord" ) || 
	   line.startsWith( m_cdrecordBinObject->path ) ||
	   line.startsWith( m_cdrecordBinObject->path.left(m_cdrecordBinObject->path.length()-5) ) ) {
    // get rid of the path and the following colon and space
    QString errStr = line.mid( line.find(':') + 2 );

    if( errStr.startsWith( "Drive does not support SAO" ) ) {
      emit infoMessage( i18n("DAO (Disk At Once) recording not supported with this writer"), K3bJob::ERROR );
      emit infoMessage( i18n("Please choose TAO (Track At Once) and try again"), K3bJob::ERROR );
    }
    else if( errStr.startsWith( "Drive does not support RAW" ) ) {
      emit infoMessage( i18n("RAW recording not supported with this writer"), K3bJob::ERROR );
    }
    else if( errStr.startsWith("Input/output error.") ) {
      emit infoMessage( i18n("Input/output error. Not necessarily serious."), WARNING );
    }
    else if( errStr.startsWith("shmget failed") ) {
      m_cdrecordError = SHMGET_FAILED;
    }
    else if( errStr.startsWith("OPC failed") ) {
      m_cdrecordError = OPC_FAILED;
    }
    else if( errStr.startsWith( "Drive needs to reload the media" ) ) {
      emit infoMessage( i18n("Reloading of media required"), K3bJob::INFO );
    }
    else if( errStr.startsWith( "The current problem looks like a buffer underrun" ) ) {
      m_cdrecordError = BUFFER_UNDERRUN;
    }
    else if( errStr.startsWith("WARNING: Data may not fit") ) {
      bool overburn = k3bcore->globalSettings()->overburn();
      if( overburn && m_cdrecordBinObject->hasFeature("overburn") )
	emit infoMessage( i18n("Trying to write more than the official disk capacity"), K3bJob::WARNING );
      m_cdrecordError = OVERSIZE;
    }
    else if( errStr.startsWith("Bad Option") ) {
      m_cdrecordError = BAD_OPTION;
      // parse option
      int pos = line.find( "Bad Option" ) + 13;
      int len = line.length() - pos - 1;
      emit infoMessage( i18n("No valid %1 option: %2").arg(m_cdrecordBinObject->name()).arg(line.mid(pos, len)), 
			ERROR );
    }
    else if( errStr.startsWith("Cannot set speed/dummy") ) {
      m_cdrecordError = CANNOT_SET_SPEED;
    }
    else if( errStr.startsWith("Cannot open new session") ) {
      m_cdrecordError = CANNOT_OPEN_NEW_SESSION;
    }
    else if( errStr.startsWith("Cannot send CUE sheet") ) {
      m_cdrecordError = CANNOT_SEND_CUE_SHEET;
    }
    else if( errStr.startsWith( "Trying to use ultra high speed medium" ) ||
	     errStr.startsWith( "Trying to use high speed medium" ) ) {
      m_cdrecordError = HIGH_SPEED_MEDIUM;
    }
    else if( errStr.startsWith( "You may have used an ultra low speed medium" ) ) {
      m_cdrecordError = LOW_SPEED_MEDIUM;
    }
    else if( errStr.startsWith( "Permission denied. Cannot open" ) ) {
      m_cdrecordError = PERMISSION_DENIED;
    }
    else if( errStr.startsWith( "Can only copy session # 1") ) {
      emit infoMessage( i18n("Only session 1 will be cloned."), WARNING );
    }
    else if( errStr == "Cannot fixate disk." ) {
      emit infoMessage( i18n("Unable to fixate the disk."), ERROR );
      if( m_cdrecordError == UNKNOWN )
	m_cdrecordError = CANNOT_FIXATE_DISK;
    }
    else if( errStr == "A write error occured." ) {
      m_cdrecordError = WRITE_ERROR;
    }
  }

  //
  // All other messages
  //

  else if( line.contains( "at speed" ) ) {
    // parse the speed and inform the user if cdrdao switched it down
    int pos = line.find( "at speed" );
    int pos2 = line.find( "in", pos+9 );
    int speed = static_cast<int>( line.mid( pos+9, pos2-pos-10 ).toDouble() );  // cdrecord-dvd >= 2.01a25 uses 8.0 and stuff
    if( speed != d->usedSpeed ) {
      emit infoMessage( i18n("Medium or burner do not support writing at %1x speed").arg(d->usedSpeed), K3bJob::WARNING );
      if( speed > d->usedSpeed )
	emit infoMessage( i18n("Switching burn speed up to %1x").arg(speed), K3bJob::WARNING );
      else
	emit infoMessage( i18n("Switching burn speed down to %1x").arg(speed), K3bJob::WARNING );
    }
  }
  else if( line.startsWith( "Starting new" ) ) {
    m_totalTracksParsed = true;
    if( m_currentTrack > 0 ) {// nothing has been written at the start of track 1
      if( d->tracks.count() > m_currentTrack-1 )
	m_alreadyWritten += d->tracks[m_currentTrack-1].size;
      else
	kdError() << "(K3bCdrecordWriter) Did not parse all tracks sizes!" << endl;
    }
    else
      emit infoMessage( i18n("Starting writing"), INFO );

    m_currentTrack++;

    if( m_currentTrack > d->tracks.count() ) {
      kdDebug() << "(K3bCdrecordWriter) need to add dummy track struct." << endl;
      struct Private::Track t;
      t.size = 1;
      t.audio = false;
      d->tracks.append(t);
    }

    kdDebug() << "(K3bCdrecordWriter) writing track " << m_currentTrack << " of " << m_totalTracks << " tracks." << endl;
    emit nextTrack( m_currentTrack, m_totalTracks );
  }
  else if( line.startsWith( "Fixating" ) ) {
    emit newSubTask( i18n("Closing Session") );
  }
  else if( line.startsWith( "Writing lead-in" ) ) {
    m_totalTracksParsed = true;
    emit newSubTask( i18n("Writing Leadin") );
  }
  else if( line.startsWith( "Writing Leadout") ) {
    emit newSubTask( i18n("Writing Leadout") );
  }
  else if( line.startsWith( "Writing pregap" ) ) {
    emit newSubTask( i18n("Writing pregap") );
  }
  else if( line.startsWith( "Performing OPC" ) ) {
    emit infoMessage( i18n("Performing Optimum Power Calibration"), K3bJob::INFO );
  }
  else if( line.startsWith( "Sending" ) ) {
    emit infoMessage( i18n("Sending CUE sheet"), K3bJob::INFO );
  }
  else if( line.startsWith( "Turning BURN-Free on" ) || line.startsWith( "BURN-Free is ON") ) {
    emit infoMessage( i18n("Enabled Burnfree"), K3bJob::INFO );
  }
  else if( line.startsWith( "Turning BURN-Free off" ) ) {
    emit infoMessage( i18n("Disabled Burnfree"), K3bJob::WARNING );
  }
  else if( line.startsWith( "Re-load disk and hit" ) ) {
    // this happens on some notebooks where cdrecord is not able to close the
    // tray itself, so we need to ask the user to do so
    blockingInformation( i18n("Please reload the medium and press 'ok'"),
			 i18n("Unable to close the tray") );

    // now send a <CR> to cdrecord
    // hopefully this will do it since I have no possibility to test it!
    ::write( fd(), "\n", 1 );
  }
  else if( s_burnfreeCounterRx.search( line ) ) {
    bool ok;
    int num = s_burnfreeCounterRx.cap(1).toInt(&ok);
    if( ok )
      emit infoMessage( i18n("Burnfree was used 1 time.", "Burnfree was used %n times.", num), INFO );
  }
  else if( s_burnfreeCounterRxPredict.search( line ) ) {
    bool ok;
    int num = s_burnfreeCounterRxPredict.cap(1).toInt(&ok);
    if( ok )
      emit infoMessage( i18n("Buffer was low 1 time.", "Buffer was low %n times.", num), INFO );
  }
  else if( line.contains("Medium Error") ) {
    m_cdrecordError = MEDIUM_ERROR;
  }
  else if( line.startsWith( "Error trying to open" ) && line.contains( "(Device or resource busy)" ) ) {
    m_cdrecordError = DEVICE_BUSY;
  }
  else {
    // debugging
    kdDebug() << "(" << m_cdrecordBinObject->name() << ") " << line << endl;
  }
}


void K3bCdrecordWriter::slotProcessExited( KProcess* p )
{
  // remove temporary cdtext file
  delete d->cdTextFile;
  d->cdTextFile = 0;

  d->interferingSystemHndl->enable();

  k3bcore->unblockDevice( burnDevice() );

  if( d->canceled ) {
    // this will unblock and eject the drive and emit the finished/canceled signals
    K3bAbstractWriter::cancel();
    return;
  }


  if( p->normalExit() ) {
    switch( p->exitStatus() ) {
    case 0:
      {
	if( simulate() )
	  emit infoMessage( i18n("Simulation successfully finished"), K3bJob::SUCCESS );
	else
	  emit infoMessage( i18n("Writing successfully finished"), K3bJob::SUCCESS );
	
	int s = d->speedEst->average();
	emit infoMessage( i18n("Average overall write speed: %1 KB/s (%2x)").arg(s).arg(KGlobal::locale()->formatNumber((double)s/150.0), 2), INFO );
	
	jobFinished( true );
      }
      break;

    default:
      kdDebug() << "(K3bCdrecordWriter) error: " << p->exitStatus() << endl;

      if( m_cdrecordError == UNKNOWN && m_lastFifoValue <= 3 )
	m_cdrecordError = BUFFER_UNDERRUN;

      switch( m_cdrecordError ) {
      case OVERSIZE:
	if( k3bcore->globalSettings()->overburn() &&
	    m_cdrecordBinObject->hasFeature("overburn") )
	  emit infoMessage( i18n("Data did not fit on disk."), ERROR );
	else {
	  emit infoMessage( i18n("Data does not fit on disk."), ERROR );
	  if( m_cdrecordBinObject->hasFeature("overburn") )
	    emit infoMessage( i18n("Enable overburning in the advanced K3b settings to burn anyway."), INFO );
	}
	break;
      case BAD_OPTION:
	// error message has already been emited earlier since we needed the actual line
	break;
      case SHMGET_FAILED:
	emit infoMessage( i18n("%1 could not reserve shared memory segment of requested size.").arg(m_cdrecordBinObject->name()), ERROR );
	emit infoMessage( i18n("Probably you chose a too large buffer size."), ERROR );
	break;
      case OPC_FAILED:
	emit infoMessage( i18n("OPC failed. Probably the writer does not like the medium."), ERROR );
	break;
      case CANNOT_SET_SPEED:
	emit infoMessage( i18n("Unable to set write speed to %1.").arg(d->usedSpeed), ERROR );
	emit infoMessage( i18n("Probably this is lower than your writer's lowest writing speed."), ERROR );
	break;
      case CANNOT_SEND_CUE_SHEET:
	emit infoMessage( i18n("Unable to send CUE sheet."), ERROR );
	if( m_writingMode == K3b::DAO )
	  emit infoMessage( i18n("Sometimes using TAO writing mode solves this issue."), ERROR );
	break;
      case CANNOT_OPEN_NEW_SESSION:
	emit infoMessage( i18n("Unable to open new session."), ERROR );
	emit infoMessage( i18n("Probably a problem with the medium."), ERROR );
	break;
      case CANNOT_FIXATE_DISK:
	emit infoMessage( i18n("The disk might still be readable."), ERROR );
	if( m_writingMode == K3b::TAO && burnDevice()->dao() )
	  emit infoMessage( i18n("Try DAO writing mode."), ERROR );
	break;
      case PERMISSION_DENIED:
	emit infoMessage( i18n("%1 has no permission to open the device.").arg("Cdrecord"), ERROR );
	emit infoMessage( i18n("You may use K3bsetup2 to solve this problem."), ERROR );
	break;
      case BUFFER_UNDERRUN:
	  emit infoMessage( i18n("Probably a buffer underrun occurred."), ERROR );
	  if( !d->usingBurnfree && burnDevice()->burnproof() )
	    emit infoMessage( i18n("Please enable Burnfree or choose a lower burning speed."), ERROR );
	  else
	    emit infoMessage( i18n("Please choose a lower burning speed."), ERROR );
	break;
      case HIGH_SPEED_MEDIUM:
	emit infoMessage( i18n("Found a high-speed medium not suitable for the writer being used."), ERROR );
	break;
      case LOW_SPEED_MEDIUM:
	emit infoMessage( i18n("Found a low-speed medium not suitable for the writer being used."), ERROR );
	break;
      case MEDIUM_ERROR:
	emit infoMessage( i18n("Most likely the burning failed due to low-quality media."), ERROR );
	break;
      case DEVICE_BUSY:
	emit infoMessage( i18n("Another application is blocking the device (most likely automounting)."), ERROR );
	break;
      case WRITE_ERROR:
	emit infoMessage( i18n("A write error occurred."), ERROR );
	if( m_writingMode == K3b::DAO )
	  emit infoMessage( i18n("Sometimes using TAO writing mode solves this issue."), ERROR );
	break;
      case UNKNOWN:
	if( p->exitStatus() == 12 && K3b::kernelVersion() >= K3bVersion( 2, 6, 8 ) && m_cdrecordBinObject->hasFeature( "suidroot" ) ) {
	  emit infoMessage( i18n("Since kernel version 2.6.8 cdrecord cannot use SCSI transport when running suid root anymore."), ERROR );
	  emit infoMessage( i18n("You may use K3bSetup to solve this problem or remove the suid bit manually."), ERROR );
	}
	else if( !wasSourceUnreadable() ) {
	  emit infoMessage( i18n("%1 returned an unknown error (code %2).")
			    .arg(m_cdrecordBinObject->name()).arg(p->exitStatus()), 
			    K3bJob::ERROR );
	  emit infoMessage( strerror(p->exitStatus()), K3bJob::ERROR );

	  if( p->exitStatus() >= 254 && m_writingMode == K3b::DAO ) {
	    emit infoMessage( i18n("Sometimes using TAO writing mode solves this issue."), ERROR );
	  }
	  else {
	    emit infoMessage( i18n("If you are running an unpatched cdrecord version..."), ERROR );
	    emit infoMessage( i18n("...and this error also occurs with high quality media..."), ERROR );
	    emit infoMessage( i18n("...and the K3b FAQ does not help you..."), ERROR );
	    emit infoMessage( i18n("...please include the debugging output in your problem report."), ERROR );
	  }
	}
	break;
      }
      jobFinished( false );
    }
  }
  else {
    emit infoMessage( i18n("%1 did not exit cleanly.").arg(m_cdrecordBinObject->name()), 
		      ERROR );
    jobFinished( false );
  }
}


void K3bCdrecordWriter::slotThroughput( int t )
{
  emit writeSpeed( t, d->tracks[m_currentTrack-1].audio ? 175 : 150 );
}

#include "k3bcdrecordwriter.moc"
