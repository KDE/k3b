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

#include <k3b.h>
#include <k3bexternalbinmanager.h>
#include <device/k3bdevicemanager.h>
#include <k3bprocess.h>
#include <device/k3bdevice.h>

#include <qstring.h>
#include <qstringlist.h>
#include <qvaluelist.h>
#include <qregexp.h>
#include <qfile.h>
#include <qdir.h>
#include <qurl.h>
#include <klocale.h>
#include <kdebug.h>
#include <kconfig.h>
#include <kio/netaccess.h>
#include <kstandarddirs.h>

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


K3bCdrdaoWriter::K3bCdrdaoWriter( K3bDevice* dev, QObject* parent, const char* name )
    : K3bAbstractWriter( dev, parent, name ),
    m_command(WRITE),
    m_blankMode(MINIMAL),
    m_sourceDevice(0),
    m_dataFile(QString("")),
    m_tocFile(QString("")),
    m_readRaw(false),
    m_multi(false),
    m_force(false),
    m_onTheFly(false),
    m_fastToc(false),
    m_readSubchan(None),
    m_taoSource(false),
    m_taoSourceAdjust(-1),
    m_paranoiaMode(-1),
    m_session(-1),
    m_eject(k3bMain()->eject()),
    m_process(0),
    m_comSock(0),
    m_currentTrack(0)
{
  QPtrList<K3bDevice> devices;
  K3bDevice *d;
  if ( !dev )
  {
    devices = k3bMain()->deviceManager()->burningDevices();
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
  devices = k3bMain()->deviceManager()->readingDevices();
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
    return m_process->stdin();
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
      << m_sourceDevice->busTargetLun();
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
  << QString("%1").arg(burnDevice()->busTargetLun());

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

  kapp->config()->setGroup("General Options");

  bool manualBufferSize =
    k3bMain()->config()->readBoolEntry( "Manual buffer size", false );
  if( manualBufferSize )
  {
    *m_process << "--buffers"
    << QString::number( k3bMain()->config()->
                        readNumEntry( "Cdrdao buffer", 32 ) );
  }

  bool overburn =
    k3bMain()->config()->readBoolEntry( "Allow overburning", false );
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
  *m_process << "--source-device" << m_sourceDevice->busTargetLun();
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
  << QString("%1").arg(burnDevice()->busTargetLun());

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
  if( m_eject )
    *m_process << "--eject";

  // remote
  *m_process << "--remote" <<  QString("%1").arg(m_cdrdaoComm[0]);

  // data File
  if ( ! m_dataFile.isEmpty() )
    *m_process << "--datafile" << m_dataFile;

  // TOC File
  if ( ! m_tocFile.isEmpty() )
    *m_process << m_tocFile;
}

K3bCdrdaoWriter* K3bCdrdaoWriter::addArgument( const QString& arg )
{
  *m_process << arg;
  return this;
}


void K3bCdrdaoWriter::start()
{
  if( m_process )
    delete m_process;  // kdelibs want this!
  m_process = new K3bProcess();
  m_process->setSplitStdout(false);
  connect( m_process, SIGNAL(stderrLine(const QString&)),
           this, SLOT(slotStdLine(const QString&)) );
  connect( m_process, SIGNAL(processExited(KProcess*)),
           this, SLOT(slotProcessExited(KProcess*)) );
  connect( m_process, SIGNAL(wroteStdin(KProcess*)),
           this, SIGNAL(dataWritten()) );

  m_canceled = false;
  m_knownError = false;

  m_cdrdaoBinObject = K3bExternalBinManager::self()->binObject("cdrdao");

  if( !m_cdrdaoBinObject )
  {
    emit infoMessage( i18n("Could not find cdrdao executable."), ERROR );
    emit finished(false);
    return;
  }

  switch ( m_command )
  {
  case WRITE:
  case COPY:
    if (!m_tocFile.isEmpty())
    {
      // workaround, cdrdao deletes the tocfile when --remote parameter is set

      m_backupTocFile = m_tocFile + ".k3bbak";
      if ( !KIO::NetAccess::copy(m_tocFile,m_backupTocFile) )
      {
        kdDebug() << "(K3bCdrdaoWriter) could not backup " << m_tocFile << " to " << m_backupTocFile << endl;
        emit infoMessage( i18n("Could not backup tocfile."), ERROR );
        emit finished(false);
        return;
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
    emit infoMessage( i18n("Could not start cdrdao!"), K3bJob::ERROR );
  }
  else
  {
    switch ( m_command )
    {
    case WRITE:
      if( simulate() )
      {
        emit infoMessage(i18n("Starting simulation at %1x speed...").arg(burnSpeed()), K3bJob::PROCESS );
        emit newTask( i18n("Simulating") );
      }
      else
      {
        emit infoMessage( i18n("Starting writing at %1x speed...").arg(burnSpeed()), K3bJob::PROCESS );
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

    emit started();
  }
}


void K3bCdrdaoWriter::cancel()
{
  m_canceled = true;

  if( m_process )
  {
    if( m_process->isRunning() )
    {
      m_process->kill();
      // we need to unlock the writer because cdrdao locked it while writing
      if ( burnDevice() )
      {
        bool block = burnDevice()->block( false );
        if( !block )
          emit infoMessage( i18n("Could not unlock CD drive."), K3bJob::ERROR );
        else if( m_eject )
          burnDevice()->eject();
      }
      if ( m_command == COPY || m_command == READ )
      {
        bool block = m_sourceDevice->block( false );
        if( !block )
          emit infoMessage( i18n("Could not unlock CD drive."), K3bJob::ERROR );
        else if( m_eject )
          m_sourceDevice->eject();
      }

    }
  }


  emit canceled();
  emit finished( false );
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
    if( !QFile::exists( m_tocFile ) && !m_onTheFly )
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

      if( m_command == WRITE || m_command == COPY )
        createAverageWriteSpeedInfoMessage();

      emit finished( true );
      break;

    default:
      if( !m_knownError )
      {
        emit infoMessage( i18n("Cdrdao returned an error! (code %1)").arg(p->exitStatus()), K3bJob::ERROR );
        emit infoMessage( strerror(p->exitStatus()), K3bJob::ERROR );
        emit infoMessage( i18n("Please send me an email with the last output..."), K3bJob::ERROR );
        emit finished( false );
      }
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
      emit infoMessage( i18n("Medium does not support writing at %1x speed").arg(burnSpeed()), K3bJob::INFO );
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
  if( line.contains( "No driver found" ) )
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
  else if( !line.contains( "remote progress message" ) )
    emit infoMessage( line, K3bJob::ERROR );
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

  createEstimatedWriteSpeed( processed, !m_writeSpeedInitialized );
  m_writeSpeedInitialized = true;

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

#include "k3bcdrdaowriter.moc"
