/***************************************************************************
                          k3bcdrdaowriter.cpp  -  description
                             -------------------
    begin                : Mon Mar 26 15:30:59 CEST 2001
    copyright            : (C) 2001 by Sebastian Trueg and
                                       Klaus-Dieter Krannich
    email                : trueg@informatik.uni-freiburg.de
                           kd@math.tu-cottbus.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "k3bcdrdaowriter.h"

#include <k3b.h>
#include <k3bexternalbinmanager.h>
#include <k3bdevicemanager.h>
#include <k3bprocess.h>
#include <device/k3bdevice.h>

#include <qstring.h>
#include <qstringlist.h>
#include <qvaluelist.h>
#include <qregexp.h>

#include <klocale.h>
#include <kdebug.h>
#include <kconfig.h>

#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>



K3bCdrdaoWriter::K3bCdrdaoWriter( K3bDevice* dev, QObject* parent, const char* name )
  : K3bAbstractWriter( dev, parent, name ),
    m_command(WRITE),
    m_blankMode(MINIMAL),
    m_sourceDevice(0),
    m_rawWrite(false),
    m_multi(false),
    m_force(false),
    m_onTheFly(false),
    m_fastToc(false),
    m_paranoiaMode(3),
    m_process(0),
    qsn(0),
    m_prepareArgumentListCalled(false)
{
  m_parser = new K3bCdrdaoParser();
  connect(m_parser,SIGNAL(newSubTask(const QString&)),
	  this,SIGNAL(newSubTask(const QString&)));
  connect(m_parser,SIGNAL(debuggingOutput( const QString&, const QString& )), 
          this,SIGNAL(debuggingOutput( const QString&, const QString& )));
  connect(m_parser,SIGNAL(infoMessage(const QString &, int)),
         this,SIGNAL(infoMessage(const QString &,int)));
  connect(m_parser,SIGNAL(percent(int)),
	  this,SIGNAL(percent(int)));
  connect(m_parser,SIGNAL(buffer(int)),
	  this,SIGNAL(buffer(int)));
  connect(m_parser,SIGNAL(subPercent(int)),
	  this,SIGNAL(subPercent(int)));
  connect( m_parser, SIGNAL(unknownCdrdaoLine(const QString&)),
	   this, SLOT(slotUnknownCdrdaoLine(const QString&)) );
}

K3bCdrdaoWriter::~K3bCdrdaoWriter()
{
  delete m_process;
  delete m_parser;
}


void K3bCdrdaoWriter::prepareArgumentList(bool copy)
{
  m_prepareArgumentListCalled = true;

  // Caution: we assume we have cdrecord. This could return NULL!!
  m_cdrdaoBinObject = K3bExternalBinManager::self()->binObject("cdrdao");
  if( m_process ) delete m_process;  // kdelibs want this!
  m_process = new K3bProcess();
  m_process->setSplitStdout(false);
  connect( m_process, SIGNAL(stderrLine(const QString&)), this, SLOT(slotStdLine(const QString&)) );
  connect( m_process, SIGNAL(processExited(KProcess*)), this, SLOT(slotProcessExited(KProcess*)) );
  connect( m_process, SIGNAL(wroteStdin(KProcess*)), this, SIGNAL(dataWritten()) );
 
  *m_process << m_cdrdaoBinObject->path;

  if (copy)
    *m_process << "copy"; 
  else
    *m_process << "write";

  // display debug info
  *m_process << "-n" << "-v" << "2";

  // Again we assume the device to be set!
  *m_process << "--device" << QString("%1").arg(burnDevice()->busTargetLun());

  if( burnDevice()->cdrdaoDriver() != "auto" ) {
    *m_process << "--driver";
    if( burnDevice()->cdTextCapable() == 1 )
      *m_process << QString("%1:0x00000010").arg( burnDevice()->cdrdaoDriver() );
    else
      *m_process << burnDevice()->cdrdaoDriver();
  }

  *m_process << "--speed" << QString("%1").arg(burnSpeed());

  if( k3bMain()->eject() )
    *m_process << "--eject";

  if( simulate() )
    *m_process << "--simulate";

  kapp->config()->setGroup("General Options");

  bool manualBufferSize = k3bMain()->config()->readBoolEntry( "Manual buffer size", false );
  if( manualBufferSize ) {
    *m_process << "--buffers" << QString::number( k3bMain()->config()->readNumEntry( "Cdrdao buffer", 32 ) );
  }

  bool overburn = k3bMain()->config()->readBoolEntry( "Allow overburning", false );
  if( overburn && m_cdrdaoBinObject->hasFeature("overburn") )
    *m_process << "--overburn";

  if( socketpair(AF_UNIX,SOCK_STREAM,0,cdrdaoComm) ) {
    kdDebug() << "(K3bCdrdaoWriter) could not open socketpair for cdrdao remote messages" << endl;
  }
  else {
    if (::fcntl(cdrdaoComm[1], F_SETFL, O_NONBLOCK) == -1) {
      kdDebug() << "(K3bCdrdaoWriter) Setting nonblocking communication mode failed" << endl;
      ::close( cdrdaoComm[0] );
      ::close( cdrdaoComm[1] );
    }
    else {
      if( qsn ) delete qsn;
      qsn = new QSocketNotifier(cdrdaoComm[1],QSocketNotifier::Read,this);
      connect( qsn, SIGNAL(activated(int)), this, SLOT(getCdrdaoMessage()));
      *m_process << "--remote" <<  QString("%1").arg(cdrdaoComm[0]);
    }
  }

  // additional parameters from config
  QStringList params = kapp->config()->readListEntry( "cdrdao parameters" );
  for( QStringList::Iterator it = params.begin(); it != params.end(); ++it )
    *m_process << *it;
}


K3bCdrdaoWriter* K3bCdrdaoWriter::addArgument( const QString& arg )
{
  *m_process << arg;
  return this;
}


void K3bCdrdaoWriter::start()
{
  if( !m_prepareArgumentListCalled ) {
    // Caution: we assume we have cdrdao. This could return NULL!!
    m_cdrdaoBinObject = K3bExternalBinManager::self()->binObject("cdrdao");
    if( m_process ) delete m_process;  // kdelibs want this!
    m_process = new K3bProcess();
    m_process->setSplitStdout(false);
    connect( m_process, SIGNAL(stderrLine(const QString&)), this, SLOT(slotStdLine(const QString&)) );
    connect( m_process, SIGNAL(processExited(KProcess*)), this, SLOT(slotProcessExited(KProcess*)) );
    connect( m_process, SIGNAL(wroteStdin(KProcess*)), this, SIGNAL(dataWritten()) );
 
    *m_process << m_cdrdaoBinObject->path;

    switch(m_command) {
    case BLANK:
      *m_process << "blank"; 
      *m_process << "--blank-mode" << ( m_blankMode == FULL ? "full" : "minimal" );
      break;
    case COPY:
      *m_process << "copy";
      // Again we assume the device to be set!
      *m_process << "--source-device" << QString("%1").arg(m_sourceDevice->busTargetLun());

      if( m_sourceDevice->cdrdaoDriver() != "auto" ) {
	*m_process << "--source-driver";
	if( m_sourceDevice->cdTextCapable() == 1 )
	  *m_process << QString("%1:0x00000010").arg( m_sourceDevice->cdrdaoDriver() );
	else
	  *m_process << m_sourceDevice->cdrdaoDriver();
      }

      if( m_onTheFly )
	*m_process << "--on-the-fly";

      if( m_dataFile.isEmpty() )
	m_dataFile = k3bMain()->findTempFile( "img" );

      *m_process << "--datafile" << m_dataFile;

      *m_process << "--paranoia-mode" << QString::number(m_paranoiaMode);

      if( m_fastToc )
	*m_process << "--fast-toc";
      break;
    default:
      *m_process << "write";
      if( m_multi )
	*m_process << "--multi";
      break;
    }

    // display debug info
    *m_process << "-n" << "-v" << "2";

    // Again we assume the device to be set!
    *m_process << "--device" << QString("%1").arg(burnDevice()->busTargetLun());

    if( burnDevice()->cdrdaoDriver() != "auto" ) {
      *m_process << "--driver";
      if( burnDevice()->cdTextCapable() == 1 )
	*m_process << QString("%1:0x00000010").arg( burnDevice()->cdrdaoDriver() );
      else
	*m_process << burnDevice()->cdrdaoDriver();
    }

    *m_process << "--speed" << QString("%1").arg(burnSpeed());

    if( k3bMain()->eject() )
      *m_process << "--eject";

    if( simulate() )
      *m_process << "--simulate";

    if( m_force )
      *m_process << "--force";

    kapp->config()->setGroup("General Options");

    bool manualBufferSize = k3bMain()->config()->readBoolEntry( "Manual buffer size", false );
    if( manualBufferSize ) {
      *m_process << "--buffers" << QString::number( k3bMain()->config()->readNumEntry( "Cdrdao buffer", 32 ) );
    }

    bool overburn = k3bMain()->config()->readBoolEntry( "Allow overburning", false );
    if( overburn && m_cdrdaoBinObject->hasFeature("overburn") )
      *m_process << "--overburn";

    if( socketpair(AF_UNIX,SOCK_STREAM,0,cdrdaoComm) ) {
      kdDebug() << "(K3bCdrdaoWriter) could not open socketpair for cdrdao remote messages" << endl;
    }
    else {
      if (::fcntl(cdrdaoComm[1], F_SETFL, O_NONBLOCK) == -1) {
	kdDebug() << "(K3bCdrdaoWriter) Setting nonblocking communication mode failed" << endl;
	::close( cdrdaoComm[0] );
	::close( cdrdaoComm[1] );
      }
      else {
	if( qsn ) delete qsn;
	qsn = new QSocketNotifier(cdrdaoComm[1],QSocketNotifier::Read,this);
	connect( qsn, SIGNAL(activated(int)), this, SLOT(getCdrdaoMessage()));
	*m_process << "--remote" <<  QString("%1").arg(cdrdaoComm[0]);
      }
    }

    // additional parameters from config
    QStringList params = kapp->config()->readListEntry( "cdrdao parameters" );
    for( QStringList::Iterator it = params.begin(); it != params.end(); ++it )
      *m_process << *it;

    // add tocfile
    if( m_tocFile.isEmpty() )
      m_tocFile = k3bMain()->findTempFile( "toc" );

    if( m_command != BLANK )
      *m_process << m_tocFile;
  }

  kdDebug() << "***** cdrdao parameters:\n";
  const QValueList<QCString>& args = m_process->args();
  QString s;
  for( QValueList<QCString>::const_iterator it = args.begin(); it != args.end(); ++it ) {
    s += *it + " ";
  }
  kdDebug() << s << endl << flush;

  m_currentTrack = 0;

  emit newSubTask( i18n("Preparing write process...") );

  if( !m_process->start( KProcess::NotifyOnExit, m_stdin ? KProcess::All : KProcess::AllOutput ) ) {
    // something went wrong when starting the program
    // it "should" be the executable
    kdDebug() << "(K3bCdrdaoWriter) could not start cdrdao" << endl;
    emit infoMessage( i18n("Could not start cdrdao!"), K3bJob::ERROR );
  }
  else {
    if( simulate() ) {
      emit infoMessage( i18n("Starting simulation at %1x speed...").arg(burnSpeed()), K3bJob::STATUS );
      emit newTask( i18n("Simulating") );
    }
    else {
      emit infoMessage( i18n("Starting recording at %1x speed...").arg(burnSpeed()), K3bJob::STATUS );
      emit newTask( i18n("Writing") );
    }
    
    emit started();
  }

  m_prepareArgumentListCalled = false;
}


void K3bCdrdaoWriter::cancel()
{
  if( m_process ) {
    if( m_process->isRunning() ) {
      m_process->disconnect();
      m_process->kill();

      // we need to unlock the writer because cdrdao locked it while writing
      bool block = burnDevice()->block( false );
      if( !block )
	emit infoMessage( i18n("Could not unlock CD drive."), K3bJob::ERROR );
      else if( k3bMain()->eject() )
	burnDevice()->eject();
    }

    // close the socket
    qsn->setEnabled(false);
    ::close( cdrdaoComm[0] );
    ::close( cdrdaoComm[1] );
    
    emit canceled();
    emit finished( false );
  }
}


bool K3bCdrdaoWriter::write( const char* data, int len )
{
  return m_process->writeStdin( data, len );
}


void K3bCdrdaoWriter::slotStdLine( const QString& line )
{
  m_parser->parseCdrdaoLine(line);
}


void K3bCdrdaoWriter::slotProcessExited( KProcess* p )
{
  if( p->normalExit() ) {
    switch( p->exitStatus() ) {
    case 0:
      if( simulate() )
	emit infoMessage( i18n("Simulation successfully finished"), K3bJob::STATUS );
      else
	emit infoMessage( i18n("Writing successfully finished"), K3bJob::STATUS );

      emit finished( true );
      break;

    default:
      // no recording device and also other errors!! :-(
      emit infoMessage( i18n("Cdrdao returned an error! (code %1)").arg(p->exitStatus()), K3bJob::ERROR );
      emit infoMessage( strerror(p->exitStatus()), K3bJob::ERROR );
      emit infoMessage( i18n("Please send me an email with the last output..."), K3bJob::ERROR );
      emit finished( false );
      break;
    }
  }
  else {
    emit infoMessage( i18n("Cdrdao did not exit cleanly."), K3bJob::ERROR );
    emit finished( false );
  }

  // close the socket
  qsn->setEnabled(false);
  ::close( cdrdaoComm[0] );
  ::close( cdrdaoComm[1] );
}


void K3bCdrdaoWriter::getCdrdaoMessage()
{
  m_parser->parseCdrdaoMessage(cdrdaoComm[1]);
}


void K3bCdrdaoWriter::slotUnknownCdrdaoLine( const QString& line )
{
 if( line.contains( "at speed" ) ) {
    // parse the speed and inform the user if cdrdao switched it down
    int pos = line.find( "at speed" );
    int po2 = line.find( QRegExp("\\D"), pos + 9 );
    int speed = line.mid( pos+9, po2-pos-9 ).toInt();
    if( speed < burnSpeed() ) {
      emit infoMessage( i18n("Medium does not support writing at %1x speed").arg(burnSpeed()), K3bJob::INFO );
      emit infoMessage( i18n("Switching down burn speed to %1x").arg(speed), K3bJob::PROCESS );
    }
  }
}

#include "k3bcdrdaowriter.moc"
