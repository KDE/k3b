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


#include "k3bcdrecordwriter.h"

#include <k3b.h>
#include <k3bcore.h>
#include <k3bexternalbinmanager.h>
#include <k3bprocess.h>
#include <device/k3bdevice.h>
#include <device/k3bdevicemanager.h>
#include <device/k3bdevicehandler.h>
#include <tools/k3bglobals.h>

#include <qstring.h>
#include <qstringlist.h>
#include <qurl.h>
#include <qvaluelist.h>
#include <qregexp.h>

#include <klocale.h>
#include <kdebug.h>
#include <kconfig.h>

#include <errno.h>
#include <string.h>


K3bCdrecordWriter::K3bCdrecordWriter( K3bDevice* dev, QObject* parent, const char* name )
  : K3bAbstractWriter( dev, parent, name ),
    m_rawWrite(false),
    m_stdin(false),
    m_clone(false),
    m_useCdrecordProDVD(false)
{
  m_process = 0;
  m_writingMode = K3b::TAO;
}


K3bCdrecordWriter::~K3bCdrecordWriter()
{
  delete m_process;
}


int K3bCdrecordWriter::fd() const
{
  if( m_process )
    return m_process->stdin();
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
}
  
void K3bCdrecordWriter::setClone( bool b )
{
  m_clone = b;
  if( b )
    m_useCdrecordProDVD = true;
}


void K3bCdrecordWriter::setUseProDVD( bool b )
{
  m_useCdrecordProDVD = b;
  if(b)
    m_clone = false;
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


void K3bCdrecordWriter::prepareArgumentList()
{
  kdDebug() << "(K3bCdrecordWriter) FIXME: REMOVE THIS METHOD AS IT DOES NOTHING!!!!" << endl;
}


void K3bCdrecordWriter::prepareProcess()
{
  if( m_process ) delete m_process;  // kdelibs want this!
  m_process = new K3bProcess();
  m_process->setSplitStdout(true);
  connect( m_process, SIGNAL(stdoutLine(const QString&)), this, SLOT(slotStdLine(const QString&)) );
  connect( m_process, SIGNAL(stderrLine(const QString&)), this, SLOT(slotStdLine(const QString&)) );
  connect( m_process, SIGNAL(processExited(KProcess*)), this, SLOT(slotProcessExited(KProcess*)) );
  connect( m_process, SIGNAL(wroteStdin(KProcess*)), this, SIGNAL(dataWritten()) );

  if( m_useCdrecordProDVD )
    m_cdrecordBinObject = K3bExternalBinManager::self()->binObject("cdrecord-prodvd");
  else
    m_cdrecordBinObject = K3bExternalBinManager::self()->binObject("cdrecord");

  if( !m_cdrecordBinObject )
    return;

  kapp->config()->setGroup("General Options");

  if( m_useCdrecordProDVD ) {
    QString proDvdKey = kapp->config()->readEntry( "cdrecord-prodvd_key" );
    if( !proDvdKey.isEmpty() )
      m_process->setEnvironment( "CDR_SECURITY", proDvdKey );
  }

  *m_process << m_cdrecordBinObject->path;

  // display progress
  *m_process << "-v";
    
  if( m_cdrecordBinObject->hasFeature( "gracetime") )
    *m_process << "gracetime=2";  // 2 is the lowest allowed value (Joerg, why do you do this to us?)
    
  // Again we assume the device to be set!
  *m_process << QString("dev=%1").arg(K3bCdDevice::externalBinDeviceParameter(burnDevice(), m_cdrecordBinObject));
  *m_process << QString("speed=%1").arg(burnSpeed());
    
  if( m_writingMode == K3b::DAO ) {
    if( burnDevice()->dao() )
      *m_process << "-dao";
    else
      emit infoMessage( i18n("Writer does not support disk at once (DAO) recording"), INFO );
  }
  else if( m_writingMode == K3b::RAW ) {
      *m_process << "-raw";
  }
    
  if( simulate() )
    *m_process << "-dummy";
    
  if( burnproof() ) {
    if( burnDevice()->burnproof() ) {
      // with cdrecord 1.11a02 burnproof was renamed to burnfree
      if( m_cdrecordBinObject->version < K3bVersion( "1.11a02" ) )
	*m_process << "driveropts=burnproof";
      else
	*m_process << "driveropts=burnfree";
    }
    else
      emit infoMessage( i18n("Writer does not support buffer underrun free recording (BURNPROOF)"), INFO );
  }
  
  if ( m_cue && !m_cueFile.isEmpty() ) {
      m_process->setWorkingDirectory(QUrl(m_cueFile).dirPath());
    *m_process << QString("cuefile=%1").arg( m_cueFile );
  }
  
  if( m_clone )
    *m_process << "-clone";
  
  if( k3bMain()->eject() )
    *m_process << "-eject";

  bool manualBufferSize = k3bcore->config()->readBoolEntry( "Manual buffer size", false );
  if( manualBufferSize ) {
    *m_process << QString("fs=%1m").arg( k3bcore->config()->readNumEntry( "Cdrecord buffer", 4 ) );
  }
    
  bool overburn = k3bcore->config()->readBoolEntry( "Allow overburning", false );
  if( overburn )
    if( m_cdrecordBinObject->hasFeature("overburn") )
      *m_process << "-overburn";
    else
      emit infoMessage( i18n("Cdrecord %1 does not support overburning!").arg(m_cdrecordBinObject->version), INFO );
    
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
  prepareProcess();

  if( !m_cdrecordBinObject ) {
    emit infoMessage( i18n("Could not find %1 executable.").arg(m_cdrecordBinObject->name()), ERROR );
    emit finished(false);
    return;
  }

  kdDebug() << "***** cdrecord parameters:\n";
  const QValueList<QCString>& args = m_process->args();
  QString s;
  for( QValueList<QCString>::const_iterator it = args.begin(); it != args.end(); ++it ) {
    s += *it + " ";
  }
  kdDebug() << s << flush << endl;
  emit debuggingOutput("cdrecord comand:", s);

  m_currentTrack = 0;
  m_cdrecordError = UNKNOWN;
  m_totalTracksParsed = false;
  m_alreadyWritten = 0;
  m_trackSizes.clear();
  m_totalSize = 0;

  emit newSubTask( i18n("Preparing write process...") );

  if( !m_process->start( KProcess::NotifyOnExit, m_stdin ? KProcess::All : KProcess::AllOutput ) ) {
    // something went wrong when starting the program
    // it "should" be the executable
    kdDebug() << "(K3bCdrecordWriter) could not start cdrecord" << endl;
    emit infoMessage( i18n("Could not start %1.").arg(m_cdrecordBinObject->name()), K3bJob::ERROR );
    emit finished(false);
  }
  else {
    if( simulate() ) {
      emit newTask( i18n("Simulating") );
      if( m_writingMode == K3b::DAO )
	emit infoMessage( i18n("Starting dao simulation at %1x speed...").arg(burnSpeed()), 
			  K3bJob::PROCESS );
      else if( m_writingMode == K3b::RAW )
	emit infoMessage( i18n("Starting raw simulation at %1x speed...").arg(burnSpeed()), 
			  K3bJob::PROCESS );
      else
	emit infoMessage( i18n("Starting tao simulation at %1x speed...").arg(burnSpeed()), 
			  K3bJob::PROCESS );
    }
    else {
      emit newTask( i18n("Writing") );

      if( m_writingMode == K3b::DAO )
	emit infoMessage( i18n("Starting dao writing at %1x speed...").arg(burnSpeed()), K3bJob::PROCESS );
      else if( m_writingMode == K3b::RAW )
	emit infoMessage( i18n("Starting raw writing at %1x speed...").arg(burnSpeed()), K3bJob::PROCESS );
      else
	emit infoMessage( i18n("Starting tao writing at %1x speed...").arg(burnSpeed()), K3bJob::PROCESS );
    }

    m_writeSpeedInitialized = false;
    emit started();
  }
}


void K3bCdrecordWriter::cancel()
{
  if( m_process ) {
    if( m_process->isRunning() ) {
      m_process->disconnect();
      m_process->kill();

      // we need to unlock the writer because cdrecord locked it while writing
      connect( K3bCdDevice::unblock( burnDevice() ), SIGNAL(finished(bool)),
	       this, SLOT(slotUnblockWhileCancellationFinished(bool)) );
    }
  }
}


void K3bCdrecordWriter::slotUnblockWhileCancellationFinished( bool success )
{
  if( !success )
    emit infoMessage( i18n("Could not unlock CD drive."), K3bJob::ERROR );
  else if( k3bMain()->eject() )
    K3bCdDevice::eject( burnDevice() );

  emit canceled();
  emit finished( false );
}


bool K3bCdrecordWriter::write( const char* data, int len )
{
  return m_process->writeStdin( data, len );
}


void K3bCdrecordWriter::slotStdLine( const QString& line )
{
  emit debuggingOutput( m_cdrecordBinObject->name(), line );

  if( line.startsWith( "Track" ) )
    {
      if( !m_totalTracksParsed ) {
	// this is not the progress display but the list of tracks that will get written
	// we always extract the tracknumber to get the highest at last
	bool ok;
	int tt = line.mid( 6, 2 ).toInt(&ok);
	if( ok )
	  m_totalTracks = tt;

	int sizeStart = line.find( QRegExp("\\d"), 10 );
	int sizeEnd = line.find( "MB", sizeStart );
	int ts = line.mid( sizeStart, sizeEnd-sizeStart ).toInt();

	m_trackSizes.append(ts);
	m_totalSize += ts;
      }

      else if( line.contains( "fifo", false ) > 0 )
	{
	  // parse progress
	  int num, made, size, fifo;
	  bool ok;

	  // --- parse number of track ---------------------------------------
	  // ----------------------------------------------------------------------
	  int pos1 = 5;
	  int pos2 = line.find(':');
	  if( pos1 == -1 ) {
	    kdDebug() << "parsing did not work" << endl;
	    return;
	  }
	  // now pos2 to the first colon :-)
	  num = line.mid(pos1,pos2-pos1).toInt(&ok);
	  if(!ok)
	    kdDebug() << "parsing did not work" << endl;

	  // --- parse already written Megs -----------------------------------
	  // ----------------------------------------------------------------------
	  pos1 = line.find(':');
	  if( pos1 == -1 ) {
	    kdDebug() << "parsing did not work" << endl;
	    return;
	  }
	  pos2 = line.find("of");
	  if( pos2 == -1 ) {
	    kdDebug() << "parsing did not work" << endl;
	    return;
	  }
	  // now pos1 point to the colon and pos2 to the 'o' of 'of' :-)
	  pos1++;
	  made = line.mid(pos1,pos2-pos1).toInt(&ok);
	  if(!ok)
	    kdDebug() << "parsing did not work" << endl;

	  // --- parse total size of track ---------------------------------------
	  // ------------------------------------------------------------------------
	  pos1 = line.find("MB");
	  if( pos1 == -1 ) {
	    kdDebug() << "parsing did not work" << endl;
	    return;
	  }
	  // now pos1 point to the 'M' of 'MB' and pos2 to the 'o' of 'of' :-)
	  pos2 += 2;
	  size = line.mid(pos2,pos1-pos2).toInt(&ok);
	  if(!ok)
	    kdDebug() << "parsing did not work" << endl;

	  // --- parse status of fifo --------------------------------------------
	  // ------------------------------------------------------------------------
	  pos1 = line.find("fifo");
	  if( pos1 == -1 ) {
	    kdDebug() << "parsing did not work" << endl;
	    return;
	  }
	  pos2 = line.find('%');
	  if( pos2 == -1 ) {
	    kdDebug() << "parsing did not work" << endl;
	    return;
	  }
	  // now pos1 point to the 'f' of 'fifo' and pos2 to the %o'  :-)
	  pos1+=4;
	  fifo = line.mid(pos1,pos2-pos1).toInt(&ok);
	  if(!ok)
	    kdDebug() << "parsing did not work" << endl;

	  // -------------------------------------------------------------------
	  // -------- parsing finished --------------------------------------


	  emit buffer( fifo );

	  //
	  // cdrecord's output sucks a bit.
	  // we get track sizes that differ from the sizes in the progress
	  // info since these are dependant on the writing mode.
	  // so we just use the track sizes and do a bit of math...
	  //

	  double convV = (double)m_trackSizes[m_currentTrack-1]/(double)size;
	  made = (int)((double)made * convV);
	  size = m_trackSizes[m_currentTrack-1];

	  emit processedSubSize( made, size );
	  emit subPercent( 100*made/size );

	  if( m_totalSize > 0 ) {
	    emit processedSize( m_alreadyWritten+made, m_totalSize );
	    emit percent( 100*(m_alreadyWritten+made)/m_totalSize );
	  }

	  createEstimatedWriteSpeed( m_alreadyWritten+made, !m_writeSpeedInitialized );
	  m_writeSpeedInitialized = true;
	}
    }
  else if( line.contains( "at speed" ) ) {
    // parse the speed and inform the user if cdrdao switched it down
    int pos = line.find( "at speed" );
    int po2 = line.find( QRegExp("\\D"), pos + 9 );
    int speed = line.mid( pos+9, po2-pos-9 ).toInt();
    if( speed < burnSpeed() ) {
      emit infoMessage( i18n("Medium does not support writing at %1x speed").arg(burnSpeed()), K3bJob::INFO );
      emit infoMessage( i18n("Switching down burn speed to %1x").arg(speed), K3bJob::PROCESS );
    }
  }
  else if( line.startsWith( "Starting new" ) ) {
    m_totalTracksParsed = true;
    if( m_currentTrack > 0 ) // nothing has been written at the start of track 1
      m_alreadyWritten += m_trackSizes[m_currentTrack-1];
    m_currentTrack++;
    kdDebug() << "(K3bCdrecordWriter) writing track " << m_currentTrack << " of " << m_totalTracks << " tracks." << endl;
    emit nextTrack( m_currentTrack, m_totalTracks );
  }
  else if( line.startsWith( "Fixating" ) ) {
    emit newSubTask( i18n("Fixating") );
  }
  else if( line.contains("seconds.") ) {
    int pos2 = line.find("seconds.") - 2;
    int pos1 = line.findRev( QRegExp("\\D"), pos2 ) + 1;
    emit infoMessage( i18n("Starting in 1 second", 
			   "Starting in %n seconds", 
			   line.mid(pos1, pos2-pos1+1).toInt()), K3bJob::PROCESS );
  }
  else if( line.startsWith( "Writing pregap" ) ) {
    emit newSubTask( i18n("Writing pregap") );
  }
  else if( line.startsWith( "Performing OPC" ) ) {
    emit infoMessage( i18n("Performing OPC"), K3bJob::PROCESS );
  }
  else if( line.startsWith( "Sending" ) ) {
    emit infoMessage( i18n("Sending CUE sheet"), K3bJob::PROCESS );
  }
  else if( line.contains( "Turning BURN-Proof" ) ) {
    emit infoMessage( i18n("Enabled BURN-Proof"), K3bJob::PROCESS );
  }
  else if( line.contains( "Drive needs to reload the media" ) ) {
    emit infoMessage( i18n("Reloading of media required"), K3bJob::PROCESS );
  }
  else if( line.contains( "Drive does not support SAO" ) ) {
    emit infoMessage( i18n("DAO (Disk At Once) recording not supported with this writer"), K3bJob::ERROR );
    emit infoMessage( i18n("Please choose TAO (Track At Once) and try again"), K3bJob::ERROR );
  }
  else if( line.contains("Data may not fit") ) {
    bool overburn = k3bcore->config()->readBoolEntry( "Allow overburning", false );
    if( overburn && m_cdrecordBinObject->hasFeature("overburn") )
      emit infoMessage( i18n("Trying to write more than the official disk capacity"), K3bJob::INFO );
    m_cdrecordError = OVERSIZE;
  }
  else if( line.contains("Bad Option") ) {
    m_cdrecordError = BAD_OPTION;
    // parse option
    int pos = line.find( "Bad Option" ) + 13;
    int len = line.length() - pos - 1;
    emit infoMessage( i18n("No valid cdrecord option: %1").arg(line.mid(pos, len)), ERROR );
  }
  else if( line.contains("shmget failed") ) {
    m_cdrecordError = SHMGET_FAILED;
  }
  else if( line.contains("OPC failed") ) {
    m_cdrecordError = OPC_FAILED;
  }
  else if( line.contains("Cannot set speed/dummy") ) {
    m_cdrecordError = CANNOT_SET_SPEED;
  }
  else if( line.contains("Cannot open new session") ) {
    m_cdrecordError = CANNOT_OPEN_NEW_SESSION;
  }
  else if( line.contains("Cannot send CUE sheet") ) {
    m_cdrecordError = CANNOT_SEND_CUE_SHEET;
  }
  else if( line.contains("Input/output error.") ) {
    emit infoMessage( i18n("Input/output error. Not necessarily serious."), ERROR );
  }
  else if( line.startsWith( "Cdrecord " ) ) {
    // display some credit for Joerg ;)
    emit infoMessage( line, INFO );
  }
  else {
    // debugging
    kdDebug() << "(" << m_cdrecordBinObject->name() << ") " << line << endl;
  }
}


void K3bCdrecordWriter::slotProcessExited( KProcess* p )
{
  if( p->normalExit() ) {
    switch( p->exitStatus() ) {
    case 0:
      if( simulate() )
	emit infoMessage( i18n("Simulation successfully finished"), K3bJob::STATUS );
      else
	emit infoMessage( i18n("Writing successfully finished"), K3bJob::STATUS );

      createAverageWriteSpeedInfoMessage();
      emit finished( true );
      break;

    default:
      kdDebug() << "(K3bCdrecordWriter) error: " << p->exitStatus() << endl;

      switch( m_cdrecordError ) {
      case OVERSIZE:
	if( k3bcore->config()->readBoolEntry( "Allow overburning", false ) &&
	    m_cdrecordBinObject->hasFeature("overburn") )
	  emit infoMessage( i18n("Data did not fit on disk."), ERROR );
	else
	  emit infoMessage( i18n("Data does not fit on disk."), ERROR );
	break;
      case BAD_OPTION:
	// error message has already been emited earlier since we needed the actual line
	break;
      case SHMGET_FAILED:
	emit infoMessage( i18n("Cdrecord could not reserve shared memory segment of requested size."), ERROR );
	emit infoMessage( i18n("Probably you chose a too large buffer size."), ERROR );
	break;
      case OPC_FAILED:
	emit infoMessage( i18n("OPC failed. Probably the writer does not like the medium."), ERROR );
	break;
      case CANNOT_SET_SPEED:
	emit infoMessage( i18n("Unable to set write speed to %1.").arg(burnSpeed()), ERROR );
	emit infoMessage( i18n("Probably this is lower than your writer's lowest writing speed."), ERROR );
	break;
      case CANNOT_SEND_CUE_SHEET:
	emit infoMessage( i18n("Unable to send CUE sheet."), ERROR );
	emit infoMessage( i18n("This may be caused by wrong settings."), ERROR );
	break;
      case CANNOT_OPEN_NEW_SESSION:
	emit infoMessage( i18n("Unable to open new session."), ERROR );
	emit infoMessage( i18n("Probably a problem with the medium."), ERROR );
	break;
      case UNKNOWN:
	// no recording device and also other errors!! :-(
	emit infoMessage( i18n("Cdrecord returned an unknown error! (code %1)").arg(p->exitStatus()), 
			  K3bJob::ERROR );
	emit infoMessage( strerror(p->exitStatus()), K3bJob::ERROR );
	emit infoMessage( i18n("Please send me an email with the last output."), K3bJob::ERROR );
	break;
      }
      emit finished( false );
    }
  }
  else {
    emit infoMessage( i18n("Cdrecord did not exit cleanly."), K3bJob::ERROR );
    emit finished( false );
  }
}


#include "k3bcdrecordwriter.moc"
