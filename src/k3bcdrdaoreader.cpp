/***************************************************************************
                          k3bcdrdaoreader.cpp  -  description
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

#include "k3bcdrdaoreader.h"

#include <k3b.h>
#include <k3bexternalbinmanager.h>
#include <k3bdevicemanager.h>
#include <k3bprocess.h>
#include <device/k3bdevice.h>
#include "remote.h"

#include <qstring.h>
#include <qstringlist.h>
#include <qvaluelist.h>

#include <klocale.h>
#include <kdebug.h>
#include <kconfig.h>

#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>



K3bCdrdaoReader::K3bCdrdaoReader( QObject* parent, const char* name )
  : K3bAbstractReader( parent, name )
{
  m_process = 0;
  qsn = 0;

  QPtrListIterator<K3bDevice> devices( k3bMain()->deviceManager()->readingDevices() );
  while( K3bDevice* dev = devices.current() ) {
    // cdrdao only supports SCSI devices
    if( dev->interfaceType() == K3bDevice::SCSI ) {
      setReadDevice(dev);
      break;
    }
    ++devices;
  }

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
  connect(m_parser,SIGNAL(nextTrack(int, int)),
	  this,SIGNAL(nextTrack(int, int)));
}


K3bCdrdaoReader::~K3bCdrdaoReader()
{
  delete m_process;
  delete m_parser;
}


void K3bCdrdaoReader::prepareArgumentList()
{
  // Caution: we assume we have cdrdao. This could return NULL!!
  m_cdrdaoBinObject = K3bExternalBinManager::self()->binObject("cdrdao");
  if( m_process ) delete m_process;  // kdelibs want this!
  m_process = new K3bProcess();
  m_process->setSplitStdout(false);
  connect( m_process, SIGNAL(stderrLine(const QString&)), this, SLOT(slotStdLine(const QString&)) );
  connect( m_process, SIGNAL(processExited(KProcess*)), this, SLOT(slotProcessExited(KProcess*)) );
 
  *m_process << m_cdrdaoBinObject->path;

  *m_process << "read-cd"; 

  // display debug info
  *m_process << "-n" << "-v" << "2";

  // Again we assume the device to be set!
  *m_process << "--device" << QString("%1").arg(readDevice()->busTargetLun());

  if(readDevice()->cdrdaoDriver() != "auto" )
    *m_process << "--driver" << readDevice()->cdrdaoDriver();

  if( k3bMain()->eject() )
    *m_process << "--eject";

  if( socketpair(AF_UNIX,SOCK_STREAM,0,cdrdaoComm) ) {
    kdDebug() << "(K3bCdrdaoReader) could not open socketpair for cdrdao remote messages" << endl;
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


K3bCdrdaoReader* K3bCdrdaoReader::addArgument( const QString& arg )
{
  *m_process << arg;
  return this;
}


void K3bCdrdaoReader::start()
{
  kdDebug() << "***** cdrdao parameters:\n";
  const QValueList<QCString>& args = m_process->args();
  QString s;
  for( QValueList<QCString>::const_iterator it = args.begin(); it != args.end(); ++it ) {
    s += *it + " ";
  }
  kdDebug() << s << endl << flush;

  m_currentTrack = 0;

  emit newSubTask( i18n("Preparing read process...") );

  if( !m_process->start( KProcess::NotifyOnExit, KProcess::AllOutput ) ) {
    // something went wrong when starting the program
    // it "should" be the executable
    kdDebug() << "(K3bCdrdaoReader) could not start cdrdao" << endl;
    emit infoMessage( i18n("Could not start cdrdao!"), K3bJob::ERROR );
  }
  else {
      emit infoMessage( i18n("Start reading ..."), K3bJob::STATUS );
      emit newTask( i18n("Reading") );
    }
    emit started();
}


void K3bCdrdaoReader::cancel()
{
  if( m_process ) {
    if( m_process->isRunning() ) {
      m_process->disconnect();
      m_process->kill();

      // we need to unlock the writer because cdrdao locked it while writing
      bool block = readDevice()->block( false );
      if( !block )
	emit infoMessage( i18n("Could not unlock CD drive."), K3bJob::ERROR );
      else if( k3bMain()->eject() )
	readDevice()->eject();
    }

    // close the socket
    qsn->setEnabled(false);
    ::close( cdrdaoComm[0] );
    ::close( cdrdaoComm[1] );
    
    emit canceled();
    emit finished( false );
  }
}



void K3bCdrdaoReader::slotStdLine( const QString& line )
{
  m_parser->parseCdrdaoLine(line);
}


void K3bCdrdaoReader::slotProcessExited( KProcess* p )
{
  if( p->normalExit() ) {
    switch( p->exitStatus() ) {
    case 0:
	emit infoMessage(i18n("Reading successfully finished"), K3bJob::STATUS );

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


void K3bCdrdaoReader::getCdrdaoMessage()
{
  m_parser->parseCdrdaoMessage(cdrdaoComm[1]);
}

#include "k3bcdrdaoreader.moc"
