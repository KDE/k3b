/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *                    Klaus-Dieter Krannich <kd@k3b.org>
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


#include "k3bcdrdaowriter.h"

#include <k3bcore.h>
#include <k3bexternalbinmanager.h>
#include <device/k3bdevicemanager.h>
#include <k3bprocess.h>
#include <device/k3bdevice.h>
#include <device/k3bdevicehandler.h>
#include <k3bthroughputestimator.h>

#include <qstring.h>
#include <qstringlist.h>
#include <qvaluelist.h>
#include <qregexp.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qdir.h>
#include <qurl.h>
#include <qsocket.h>
#include <qsocketdevice.h>

#include <klocale.h>
#include <kdebug.h>
#include <kconfig.h>
#include <kio/netaccess.h>
#include <kstandarddirs.h>
#include <ktempfile.h>

#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>



inline bool operator<( const ProgressMsg& m1, const ProgressMsg& m2 )
{
  return m1.track < m2.track
         || ( m1.track == m2.track
              && m1.trackProgress < m2.trackProgress )
         || m1.totalProgress < m2.totalProgress;
}


inline bool operator==( const ProgressMsg& m1, const ProgressMsg& m2 )
{
  return m1.status == m2.status
         && m1.track == m2.track
         && m1.totalTracks == m2.totalTracks
         && m1.trackProgress == m2.trackProgress
         && m1.totalProgress == m2.totalProgress
         && m1.bufferFillRate == m2.bufferFillRate;
}

inline bool operator!=( const ProgressMsg& m1, const ProgressMsg& m2 )
{
  return !( m1 == m2 );
}



class K3bCdrdaoWriter::Private
{
public:
  Private() {
  }

  K3bThroughputEstimator* speedEst;
};


K3bCdrdaoWriter::K3bCdrdaoWriter( K3bDevice* dev, QObject* parent, const char* name )
  : K3bAbstractWriter( dev, parent, name ),
    m_command(WRITE),
    m_blankMode(MINIMAL),
    m_sourceDevice(0),
    m_readRaw(false),
    m_multi(false),
    m_force(false),
    m_burnproof(true),
    m_onTheFly(false),
    m_fastToc(false),
    m_readSubchan(None),
    m_taoSource(false),
    m_taoSourceAdjust(-1),
    m_paranoiaMode(-1),
    m_session(-1),
    m_process(0),
    m_comSock(0),
    m_currentTrack(0),
    m_forceNoEject(false)
{
  d = new Private();
  d->speedEst = new K3bThroughputEstimator( this );
  connect( d->speedEst, SIGNAL(throughput(int)),
	   this, SLOT(slotThroughput(int)) );

  k3bcore->config()->setGroup("General Options");
  m_eject = !k3bcore->config()->readBoolEntry( "No cd eject", false );

  QPtrList<K3bDevice> devices;
  K3bDevice *d;
  if ( !dev )
  {
    devices = k3bcore->deviceManager()->burningDevices();
    d = devices.first();
    while( d )
    {
      if( d->interfaceType() == K3bDevice::SCSI )
      {
        setBurnDevice(d);
        break;
      }
      d = devices.next();
    }
  }
  devices = k3bcore->deviceManager()->readingDevices();
  d = devices.first();
  while( d )
  {
    if( d->interfaceType() == K3bDevice::SCSI )
    {
      m_sourceDevice = d;
      break;
    }
    d = devices.next();
  }
  if ( !m_sourceDevice )
    m_sourceDevice = burnDevice();

  m_oldMsg = new ProgressMsg;
  m_newMsg = new ProgressMsg;

  m_oldMsg->track = 0;
  m_oldMsg->trackProgress = 0;
  m_oldMsg->totalProgress = 0;

  if( socketpair(AF_UNIX,SOCK_STREAM,0,m_cdrdaoComm) )
  {
    kdDebug() << "(K3bCdrdaoWriter) could not open socketpair for cdrdao remote messages" << endl;
  }
  else
  {
    if( m_comSock )
      delete m_comSock;
    m_comSock = new QSocket();
    m_comSock->setSocket(m_cdrdaoComm[1]);
    m_comSock->socketDevice()->setReceiveBufferSize(49152);
    // magic number from Qt documentation
    m_comSock->socketDevice()->setBlocking(false);
    connect( m_comSock, SIGNAL(readyRead()),
             this, SLOT(getCdrdaoMessage()));
  }
}

K3bCdrdaoWriter::~K3bCdrdaoWriter()
{
  delete d;

  // close the socket
  if( m_comSock )
  {
    m_comSock->close();
    ::close( m_cdrdaoComm[0] );
  }
  delete m_process;
  delete m_comSock;
  delete m_oldMsg;
  delete m_newMsg;
}


int K3bCdrdaoWriter::fd() const
{
  if( m_process )
    return m_process->stdinFd();
  else
    return -1;
}


void K3bCdrdaoWriter::prepareArgumentList()
{

  // binary
  *m_process << m_cdrdaoBinObject->path;

  // command
  switch ( m_command )
  {
  case COPY:
    *m_process << "copy";
    setWriteArguments();
    setReadArguments();
    setCopyArguments();
    break;
  case WRITE:
    *m_process << "write";
    setWriteArguments();
    break;
  case READ:
    *m_process << "read-cd";
    // source device and source driver
    if ( m_sourceDevice )
      *m_process << "--device"
      << K3bCdDevice::externalBinDeviceParameter(m_sourceDevice, m_cdrdaoBinObject);
    if ( m_sourceDevice->cdrdaoDriver() != "auto" )
      *m_process << "--driver" << m_sourceDevice->cdrdaoDriver();
    setReadArguments();
    break;
  case BLANK:
    *m_process << "blank";
    setBlankArguments();
    break;
  }

  setCommonArguments();
}

void K3bCdrdaoWriter::setWriteArguments()
{
  // device and driver
  *m_process << "--device"
	     << K3bCdDevice::externalBinDeviceParameter(burnDevice(), m_cdrdaoBinObject);

  if( burnDevice()->cdrdaoDriver() != "auto" )
  {
    *m_process << "--driver";
    if( burnDevice()->cdTextCapable() == 1 )
      *m_process << QString("%1:0x00000010").arg( burnDevice()->cdrdaoDriver() );
    else
      *m_process << burnDevice()->cdrdaoDriver();
  }

  // burn speed
  *m_process << "--speed" << QString("%1").arg(burnSpeed());

  //simulate
  if( simulate() )
    *m_process << "--simulate";

  // multi
  if( m_multi )
    *m_process << "--multi";

  // force
  if( m_force )
    *m_process << "--force";

  /*
  FIX: Do not work now :(
  // burnproof
  if ( !m_burnproof )
    *m_process << "--buffer-under-run-protection 0";
  */
  
  k3bcore->config()->setGroup("General Options");

  bool manualBufferSize =
    k3bcore->config()->readBoolEntry( "Manual buffer size", false );
  if( manualBufferSize )
  {
    *m_process << "--buffers"
    << QString::number( k3bcore->config()->
                        readNumEntry( "Cdrdao buffer", 32 ) );
  }

  bool overburn =
    k3bcore->config()->readBoolEntry( "Allow overburning", false );
  if( overburn ) {
    if( m_cdrdaoBinObject->hasFeature("overburn") )
      *m_process << "--overburn";
    else
      emit infoMessage( i18n("Cdrdao %1 does not support overburning!").arg(m_cdrdaoBinObject->version), INFO );
  }

}

void K3bCdrdaoWriter::setReadArguments()
{
  // readRaw
  if ( m_readRaw )
    *m_process << "--read-raw";

  // subchan
  if ( m_readSubchan != None )
  {
    *m_process << "--read-subchan";
    switch ( m_readSubchan )
    {
    case RW:
      *m_process << "rw";
      break;
    case RW_RAW:
      *m_process << "rw_raw";
      break;
    case None:
      break;
    }
  }

  // TAO Source
  if ( m_taoSource )
    *m_process << "--tao-source";

  // TAO Source Adjust
  if ( m_taoSourceAdjust != -1 )
    *m_process << "--tao-source-adjust"
    << QString("%1").arg(m_taoSourceAdjust);

  // paranoia Mode
  if ( m_paranoiaMode != -1 )
    *m_process << "--paranoia-mode"
    << QString("%1").arg(m_paranoiaMode);

  // session
  if ( m_session != -1 )
    *m_process << "--session"
    << QString("%1").arg(m_session);

  // fast TOC
  if ( m_fastToc )
    *m_process << "--fast-toc";

}

void K3bCdrdaoWriter::setCopyArguments()
{
  // source device and source driver
  *m_process << "--source-device" << K3bCdDevice::externalBinDeviceParameter(m_sourceDevice, m_cdrdaoBinObject);
  if ( m_sourceDevice->cdrdaoDriver() != "auto" )
    *m_process << "--source-driver" << m_sourceDevice->cdrdaoDriver();

  // on-the-fly
  if ( m_onTheFly )
    *m_process << "--on-the-fly";
}

void K3bCdrdaoWriter::setBlankArguments()
{
  // device and driver
  *m_process << "--device"
	     << K3bCdDevice::externalBinDeviceParameter(burnDevice(), m_cdrdaoBinObject);

  if( burnDevice()->cdrdaoDriver() != "auto" )
  {
    *m_process << "--driver";
    if( burnDevice()->cdTextCapable() == 1 )
      *m_process << QString("%1:0x00000010").arg( burnDevice()->cdrdaoDriver() );
    else
      *m_process << burnDevice()->cdrdaoDriver();
  }

  // burn speed
  *m_process << "--speed" << QString("%1").arg(burnSpeed());

  // blank-mode
  *m_process << "--blank-mode";
  switch (m_blankMode)
  {
  case FULL:
    *m_process << "full";
    break;
  case MINIMAL:
    *m_process << "minimal";
    break;
  }
}

void K3bCdrdaoWriter::setCommonArguments()
{

  // additional user parameters from config
  const QStringList& params = m_cdrdaoBinObject->userParameters();
  for( QStringList::const_iterator it = params.begin(); it != params.end(); ++it )
    *m_process << *it;


  // display debug info
  *m_process << "-n" << "-v" << "2";
  // eject
  if( m_eject && !m_forceNoEject )
    *m_process << "--eject";

  // remote
  *m_process << "--remote" <<  QString("%1").arg(m_cdrdaoComm[0]);

  // data File
  if ( ! m_dataFile.isEmpty() )
    *m_process << "--datafile" << m_dataFile;

  // BIN/CUE
  if ( ! m_cueFileLnk.isEmpty() )
    *m_process << m_cueFileLnk;
  // TOC File
  else if ( ! m_tocFile.isEmpty() )
    *m_process << m_tocFile;
}

K3bCdrdaoWriter* K3bCdrdaoWriter::addArgument( const QString& arg )
{
  *m_process << arg;
  return this;
}


void K3bCdrdaoWriter::start()
{
  emit started();

  d->speedEst->reset();

  if( m_process )
    delete m_process;  // kdelibs want this!
  m_process = new K3bProcess();
  m_process->setRunPrivileged(true);
  m_process->setSplitStdout(false);
  connect( m_process, SIGNAL(stderrLine(const QString&)),
           this, SLOT(slotStdLine(const QString&)) );
  connect( m_process, SIGNAL(processExited(KProcess*)),
           this, SLOT(slotProcessExited(KProcess*)) );
  connect( m_process, SIGNAL(wroteStdin(KProcess*)),
           this, SIGNAL(dataWritten()) );

  m_canceled = false;
  m_knownError = false;

  m_cdrdaoBinObject = k3bcore->externalBinManager()->binObject("cdrdao");

  if( !m_cdrdaoBinObject ) {
    emit infoMessage( i18n("Could not find %1 executable.").arg("cdrdao"), ERROR );
    emit finished(false);
    return;
  }

  if( !m_cdrdaoBinObject->copyright.isEmpty() )
    emit infoMessage( i18n("Using %1 %2 - Copyright (C) %3").arg(m_cdrdaoBinObject->name()).arg(m_cdrdaoBinObject->version).arg(m_cdrdaoBinObject->copyright), INFO );


  switch ( m_command )
    {
    case WRITE:
    case COPY:
      if (!m_tocFile.isEmpty())
	{

	  // if tocfile is a cuesheet than create symlinks to *.cue and the binary listed inside the cuesheet.
	  // now works without the .bin extension too.
	  if ( !cueSheet() ) {
	    m_backupTocFile = m_tocFile + ".k3bbak";

	    // workaround, cdrdao deletes the tocfile when --remote parameter is set
	    if ( !KIO::NetAccess::copy(m_tocFile,m_backupTocFile) )
	      {
		kdDebug() << "(K3bCdrdaoWriter) could not backup " << m_tocFile << " to " << m_backupTocFile << endl;
		emit infoMessage( i18n("Could not backup tocfile."), ERROR );
		emit finished(false);
		return;
	      }
	  }
	}
      break;
    case BLANK:
    case READ:
      break;
    }
  prepareArgumentList();
  // set working dir to dir part of toc file (to allow rel names in toc-file)
  m_process->setWorkingDirectory(QUrl(m_tocFile).dirPath());

  kdDebug() << "***** cdrdao parameters:\n";
  const QValueList<QCString>& args = m_process->args();
  QString s;
  for( QValueList<QCString>::const_iterator it = args.begin(); it != args.end(); ++it )
    {
      s += *it + " ";
    }
  kdDebug() << s << flush << endl;
  emit debuggingOutput("cdrdao comand:", s);

  m_currentTrack = 0;
  reinitParser();

  switch ( m_command )
    {
    case READ:
      emit newSubTask( i18n("Preparing read process...") );
      break;
    case WRITE:
      emit newSubTask( i18n("Preparing write process...") );
      break;
    case COPY:
      emit newSubTask( i18n("Preparing copy process...") );
      break;
    case BLANK:
      emit newSubTask( i18n("Preparing blanking process...") );
      break;
    }

  if( !m_process->start( KProcess::NotifyOnExit, m_stdin ? KProcess::All : KProcess::AllOutput ) )
    {
      // something went wrong when starting the program
      // it "should" be the executable
      kdDebug() << "(K3bCdrdaoWriter) could not start cdrdao" << endl;
      emit infoMessage( i18n("Could not start %1.").arg("cdrdao"), K3bJob::ERROR );
      emit finished(false);
    }
  else
    {
      switch ( m_command )
	{
	case WRITE:
	  if( simulate() )
	    {
	      emit infoMessage(i18n("Starting dao simulation at %1x speed...").arg(burnSpeed()), 
			       K3bJob::PROCESS );
	      emit newTask( i18n("Simulating") );
	    }
	  else
	    {
	      emit infoMessage( i18n("Starting dao writing at %1x speed...").arg(burnSpeed()), K3bJob::PROCESS );
	      emit newTask( i18n("Writing") );
	    }
	  break;
	case READ:
	  emit infoMessage(i18n("Starting reading..."), K3bJob::PROCESS );
	  emit newTask( i18n("Reading") );
	  break;
	case COPY:
	  if( simulate() )
	    {
	      emit infoMessage(i18n("Starting simulation copy at %1x speed...").arg(burnSpeed()), K3bJob::PROCESS );
	      emit newTask( i18n("Simulating") );
	    }
	  else
	    {
	      emit infoMessage( i18n("Starting copy at %1x speed...").arg(burnSpeed()), K3bJob::PROCESS );
	      emit newTask( i18n("Copying") );
	    }
	  break;
	case BLANK:
	  emit infoMessage(i18n("Starting blanking..."), K3bJob::PROCESS );
	  emit newTask( i18n("Blanking") );
	}
    }
}


void K3bCdrdaoWriter::cancel()
{
  m_canceled = true;

  if( m_process ) {
    if( m_process->isRunning() ) {
      m_process->disconnect();
      m_process->kill();
      
      // we need to unlock the device because cdrdao locked it while writing
      //
      // FIXME: try to determine wheater we are writing or reading and choose
      // the device to unblock based on that result.
      //
      if( m_command == READ ) {
	// FIXME: this is a hack
	setBurnDevice( m_sourceDevice );
      }

      // this will unblock and eject the drive and emit the finished/canceled signals
      K3bAbstractWriter::cancel();
    }
  }
}


bool K3bCdrdaoWriter::cueSheet()
{
    if ( m_tocFile.lower().endsWith( ".cue" ) ) {
        QFile f( m_tocFile );
        if ( f.open( IO_ReadOnly ) ) {
            QTextStream ts( &f );
            if ( !ts.eof() ) {
                QString line = ts.readLine();
                f.close();
                int pos = line.find( "FILE \"" );
                if( pos < 0 )
                    return false;

                pos += 6;
                int endPos = line.find( "\" BINARY", pos+1 );
                if( endPos < 0 )
                    return false;

                line = line.mid( pos, endPos-pos );
                QFileInfo fi( QFileInfo( m_tocFile ).dirPath() + "/" + QFileInfo( line ).fileName() );
                QString binpath = fi.filePath();
                kdDebug() << QString("K3bCdrdaoWriter::cueSheet() BinFilePath from CueFile: %1").arg( line ) << endl;
                kdDebug() << QString("K3bCdrdaoWriter::cueSheet() absolute BinFilePath: %1").arg( binpath ) << endl;

                if ( !fi.exists() )
                    return false;

                KTempFile tempF;
                QString tempFile = tempF.name();
                tempF.unlink();

                if ( symlink(QFile::encodeName( binpath ), QFile::encodeName( tempFile + ".bin") ) == -1 )
                    return false;
                if ( symlink(QFile::encodeName( m_tocFile ), QFile::encodeName( tempFile + ".cue") ) == -1 )
                    return false;

                kdDebug() << QString("K3bCdrdaoWriter::cueSheet() symlink BinFileName: %1.bin").arg( tempFile ) << endl;
                kdDebug() << QString("K3bCdrdaoWriter::cueSheet() symlink CueFileName: %1.cue").arg( tempFile ) << endl;
                m_binFileLnk = tempFile + ".bin";
                m_cueFileLnk = tempFile + ".cue";
                return true;
            }
        }
    }

    return false;
}

void K3bCdrdaoWriter::slotStdLine( const QString& line )
{
  parseCdrdaoLine(line);
}


void K3bCdrdaoWriter::slotProcessExited( KProcess* p )
{

  switch ( m_command )
  {
  case WRITE:
  case COPY:
    if ( !m_binFileLnk.isEmpty() ) {
        KIO::NetAccess::del(m_cueFileLnk);
        KIO::NetAccess::del(m_binFileLnk);
    }
    else if( !QFile::exists( m_tocFile ) && !m_onTheFly )
    {
      // cdrdao removed the tocfile :(
      // we need to recover it
      if ( !KIO::NetAccess::copy(m_backupTocFile, m_tocFile) )
      {
        kdDebug() << "(K3bCdrdaoWriter) restoring tocfile " << m_tocFile << " failed." << endl;
        emit infoMessage( i18n("Due to a bug in cdrdao the toc/cue file %1 has been deleted. "
                               "K3b was unable to restore it from the backup %2.").arg(m_tocFile).arg(m_backupTocFile), ERROR );
      }
      else if ( !KIO::NetAccess::del(m_backupTocFile) )
      {
        kdDebug() << "(K3bCdrdaoWriter) delete tocfile backkup " << m_backupTocFile << " failed." << endl;
      }
    }
    break;
  case BLANK:
  case READ:
    break;
  }

  if( m_canceled )
    return;

  if( p->normalExit() )
  {
    switch( p->exitStatus() )
    {
    case 0:
      if( simulate() )
        emit infoMessage( i18n("Simulation successfully finished"), K3bJob::STATUS );
      else
        switch ( m_command )
        {
        case READ:
          emit infoMessage( i18n("Reading successfully finished"), K3bJob::STATUS );
          break;
        case WRITE:
          emit infoMessage( i18n("Writing successfully finished"), K3bJob::STATUS );
          break;
        case COPY:
          emit infoMessage( i18n("Copying successfully finished"), K3bJob::STATUS );
          break;
        case BLANK:
          emit infoMessage( i18n("Blanking successfully finished"), K3bJob::STATUS );
          break;
        }

      if( m_command == WRITE || m_command == COPY ) {
	int s = d->speedEst->average();
	emit infoMessage( i18n("Average overall write speed: %1 kb/s (%2x)").arg(s).arg(KGlobal::locale()->formatNumber((double)s/150.0), 2), INFO );
      }

      emit finished( true );
      break;

    default:
      if( !m_knownError ) {
        emit infoMessage( i18n("%1 returned an unknown error (code %2).").arg(m_cdrdaoBinObject->name()).arg(p->exitStatus()), 
			  K3bJob::ERROR );
	emit infoMessage( strerror(p->exitStatus()), K3bJob::ERROR );
	emit infoMessage( i18n("Please send me an email with the last output."), K3bJob::ERROR );
      }

      emit finished( false );
      break;
    }
  }
  else
  {
    emit infoMessage( i18n("Cdrdao did not exit cleanly."), K3bJob::ERROR );
    emit finished( false );
  }
}

void K3bCdrdaoWriter::getCdrdaoMessage()
{
  parseCdrdaoMessage(m_comSock);
}

bool K3bCdrdaoWriter::write( const char* data, int len )
{
  return m_process->writeStdin( data, len );
}



void K3bCdrdaoWriter::unknownCdrdaoLine( const QString& line )
{
  if( line.contains( "at speed" ) )
  {
    // parse the speed and inform the user if cdrdao switched it down
    int pos = line.find( "at speed" );
    int po2 = line.find( QRegExp("\\D"), pos + 9 );
    int speed = line.mid( pos+9, po2-pos-9 ).toInt();
    if( speed < burnSpeed() )
    {
      emit infoMessage( i18n("Medium or burner do not support writing at %1x speed").arg(burnSpeed()), K3bJob::INFO );
      emit infoMessage( i18n("Switching down burn speed to %1x").arg(speed), K3bJob::PROCESS );
    }
  }
}


void K3bCdrdaoWriter::reinitParser()
{
  delete m_oldMsg;
  delete m_newMsg;
  m_oldMsg = new ProgressMsg;
  m_newMsg = new ProgressMsg;

  m_oldMsg->track = 0;
  m_oldMsg->trackProgress = 0;
  m_oldMsg->totalProgress = 0;
  m_currentTrack=0;
}

void K3bCdrdaoWriter::parseCdrdaoLine( const QString& str )
{
  emit debuggingOutput( "cdrdao", str );
  //  kdDebug() << "(cdrdaoparse)" << str << endl;
  // find some messages from cdrdao
  // -----------------------------------------------------------------------------------------
  if( (str).startsWith( "Warning" ) || (str).startsWith( "WARNING" ) || (str).startsWith( "ERROR" ) )
  {
    parseCdrdaoError( str );
  }
  else if( (str).startsWith( "Wrote" ) && !str.contains("blocks") )
  {
    parseCdrdaoWrote( str );
  }
  else if( (str).startsWith( "Executing power" ) )
  {
    emit newSubTask( i18n("Executing Power calibration") );
  }
  else if( (str).startsWith( "Power calibration successful" ) )
  {
    emit infoMessage( i18n("Power calibration successful"), K3bJob::PROCESS );
    emit newSubTask( i18n("Preparing burn process...") );
  }
  else if( (str).startsWith( "Flushing cache" ) )
  {
    emit newSubTask( i18n("Flushing cache") );
  }
  else if( (str).startsWith( "Writing CD-TEXT lead" ) )
  {
    emit newSubTask( i18n("Writing CD-Text lead-in...") );
  }
  else if( (str).startsWith( "Turning BURN-Proof on" ) )
  {
    emit infoMessage( i18n("Turning BURN-Proof on"), K3bJob::PROCESS );
  }
  else if( str.startsWith( "Copying" ) )
  {
    emit infoMessage( str, K3bJob::PROCESS );
  }
  else if( str.startsWith( "Found ISRC" ) )
  {
    emit infoMessage( i18n("Found ISRC code"), K3bJob::PROCESS );
  }
  else if( str.startsWith( "Found pre-gap" ) )
  {
    emit infoMessage( i18n("Found pregap: %1").arg( str.mid(str.find(":")+1) ), K3bJob::PROCESS );
  }
  else
    unknownCdrdaoLine(str);
}

void K3bCdrdaoWriter::parseCdrdaoError( const QString& line )
{
  int pos = -1;

  if( line.contains( "No driver found" ) ||
      line.contains( "use option --driver" ) )
  {
    emit infoMessage( i18n("No cdrdao driver found."), K3bJob::ERROR );
    emit infoMessage( i18n("Please select one manually in the device settings."), K3bJob::ERROR );
    emit infoMessage( i18n("For most current drives this would be 'generic-mmc'."), K3bJob::ERROR );
    m_knownError = true;
  }
  else if( line.contains( "Cannot setup device" ) )
  {
    // no nothing...
  }
  else if( line.contains( "not ready") )
  {
    emit infoMessage( i18n("Device not ready, waiting."),K3bJob::PROCESS );
  }
  else if( line.contains("Drive does not accept any cue sheet") )
  {
    emit infoMessage( i18n("Cue sheet not accepted."), K3bJob::ERROR );
    emit infoMessage( i18n("Try setting the first pregap to 0."), K3bJob::ERROR );
    m_knownError = true;
  }
  else if( (pos = line.find( "Illegal option" )) > 0 ) {
    // ERROR: Illegal option: -wurst
    emit infoMessage( i18n("No valid %1 option: %2").arg(m_cdrdaoBinObject->name()).arg(line.mid(pos+16)), 
		      ERROR );
    m_knownError = true;
  }
 //  else if( !line.contains( "remote progress message" ) )
//     emit infoMessage( line, K3bJob::ERROR );
}

void K3bCdrdaoWriter::parseCdrdaoWrote( const QString& line )
{
  int pos, po2;
  pos = line.find( "Wrote" );
  po2 = line.find( " ", pos + 6 );
  int processed = line.mid( pos+6, po2-pos-6 ).toInt();

  pos = line.find( "of" );
  po2 = line.find( " ", pos + 3 );
  m_size = line.mid( pos+3, po2-pos-3 ).toInt();

  d->speedEst->dataWritten( processed*1024 );

  emit processedSize( processed, m_size );
}

void K3bCdrdaoWriter::parseCdrdaoMessage(QSocket *comSock)
{
  char msgSync[] = { 0xff, 0x00, 0xff, 0x00 };
  char buf;
  int  state;
  unsigned int  avail,count;
  QString task;
  int msgs;

  avail = comSock->bytesAvailable();
  count = 0;

  msgs = avail / ( sizeof(msgSync)+sizeof(struct ProgressMsg) );
  if ( msgs < 1 )
    return;
  else if ( msgs > 1)
  {
    // move the read-index forward to the beginnig of the most recent message
    count = ( msgs-1 ) * ( sizeof(msgSync)+sizeof(struct ProgressMsg) );
    comSock->at(count);
    kdDebug() << "(K3bCdrdaoParser) " << msgs-1 << " message(s) skipped" << endl;
  }
  while (count < avail)
  {
    state = 0;
    while (state < 4)
    {
      buf=comSock->getch();
      count++;
      if (count == avail)
      {
        kdDebug() << "(K3bCdrdaoParser) remote message sync not found (" << count << ")" << endl;
        return;
      }

      if (buf == msgSync[state])
        state++;
      else
        state = 0;
    }

    if( (avail - count) < (int)sizeof(struct ProgressMsg) )
    {
      kdDebug() << "(K3bCdrdaoParser) could not read complete remote message." << endl;
      return;
    }

    // read one message
    int size = comSock->readBlock((char *)m_newMsg,sizeof(struct ProgressMsg));
    if( size == -1 )
      kdDebug() << "(K3bCdrdaoParser) read error" << endl;
    count += size;

    //   kdDebug() << "Status: " << msg.status
    // 	    << " Total:  " << msg.totalTracks
    // 	    << " Track:  " << msg.track
    // 	    << " TrackProgress: " << msg.trackProgress/10
    // 	    << " TotalProgress: " << msg.totalProgress/10
    // 	    << endl;

    // sometimes the progress takes one step back (on my system when using paranoia-level 3)
    // so we just use messages that are greater than the previous or first messages
    if( *m_oldMsg < *m_newMsg
        || ( m_newMsg->track == 1 &&
             m_newMsg->trackProgress <= 10 ))
    {
      emit subPercent( m_newMsg->trackProgress/10 );
      emit percent( m_newMsg->totalProgress/10 );
      emit buffer(m_newMsg->bufferFillRate);

      if ( m_newMsg->track != m_currentTrack)
      {
        switch (m_newMsg->status)
        {
        case PGSMSG_RCD_EXTRACTING:
          //	task = i18n("Reading (Track %1 of %2)").arg(m_newMsg->track).arg(m_newMsg->totalTracks);
          emit nextTrack( m_newMsg->track, m_newMsg->totalTracks );
          break;
        case PGSMSG_WCD_LEADIN:
          task = i18n("Writing leadin ");
          emit newSubTask( task );
          break;
        case PGSMSG_WCD_DATA:
          //	task = i18n("Writing (Track %1 of %2)").arg(m_newMsg->track).arg(m_newMsg->totalTracks);
          emit nextTrack( m_newMsg->track, m_newMsg->totalTracks );
          break;
        case PGSMSG_WCD_LEADOUT:
          task = i18n("Writing leadout ");
          emit newSubTask( task );
          break;
        }

        m_currentTrack = m_newMsg->track;
      }

      struct ProgressMsg* m = m_newMsg;
      m_newMsg = m_oldMsg;
      m_oldMsg = m;
    }
  }
}


void K3bCdrdaoWriter::slotThroughput( int t )
{
  emit writeSpeed( t, 150 );
}

#include "k3bcdrdaowriter.moc"
