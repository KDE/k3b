/***************************************************************************
                          k3bcdrecordwriter.cpp  -  description
                             -------------------
    begin                : Mon Mar 26 15:30:59 CEST 2001
    copyright            : (C) 2001 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "k3bcdrecordwriter.h"

#include <k3b.h>
#include <k3bexternalbinmanager.h>
#include <k3bprocess.h>
#include <device/k3bdevice.h>

#include <qstring.h>
#include <qstringlist.h>
#include <qvaluelist.h>

#include <klocale.h>
#include <kdebug.h>
#include <kconfig.h>

#include <errno.h>
#include <string.h>


K3bCdrecordWriter::K3bCdrecordWriter( QObject* parent, const char* name )
  : K3bAbstractWriter( parent, name ),
    m_dao(false),
    m_rawWrite(false),
    m_stdin(false)
{
  m_process = 0;
}


K3bCdrecordWriter::~K3bCdrecordWriter()
{
  delete m_process;
}


void K3bCdrecordWriter::prepareArgumentList()
{
  // Caution: we assume we have cdrecord. This could return NULL!!
  m_cdrecordBinObject = K3bExternalBinManager::self()->binObject("cdrecord");
  if( m_process ) delete m_process;  // kdelibs want this!
  m_process = new K3bProcess();
  m_process->setSplitStdout(true);
  connect( m_process, SIGNAL(stdoutLine(const QString&)), this, SLOT(slotStdLine(const QString&)) );
  connect( m_process, SIGNAL(stderrLine(const QString&)), this, SLOT(slotStdLine(const QString&)) );
  connect( m_process, SIGNAL(processExited(KProcess*)), this, SLOT(slotProcessExited(KProcess*)) );
  connect( m_process, SIGNAL(wroteStdin(KProcess*)), this, SIGNAL(dataWritten()) );

  *m_process << m_cdrecordBinObject->path;

  // display progress
  *m_process << "-v";

  if( m_cdrecordBinObject->hasFeature( "gracetime") )
    *m_process << "gracetime=2";  // 2 is the lowest allowed value (Joerg, why do you do this to us?)

  // Again we assume the device to be set!
  *m_process << QString("dev=%1").arg(burnDevice()->busTargetLun());
  *m_process << QString("speed=%1").arg(burnSpeed());

  if( m_dao ) {
    if( burnDevice()->dao() )
      *m_process << "-dao";
    else
      emit infoMessage( i18n("Writer does not support disk at once (DAO) recording"), INFO );
  }

  if( simulate() )
    *m_process << "-dummy";

  if( burnproof() ) {
    if( burnDevice()->burnproof() ) {
      // with cdrecord 1.11a02 burnproof was renamed to burnfree
      if( m_cdrecordBinObject->version >= "1.11a02" )
	*m_process << "driveropts=burnfree";
      else
	*m_process << "driveropts=burnproof";
    }
    else
      emit infoMessage( i18n("Writer does not support buffer underrun free recording (BURNPROOF)"), INFO );
  }

  if( k3bMain()->eject() )
    *m_process << "-eject";

  kapp->config()->setGroup("General Options");

  bool manualBufferSize = k3bMain()->config()->readBoolEntry( "Manual buffer size", false );
  if( manualBufferSize ) {
    *m_process << QString("fs=%1m").arg( k3bMain()->config()->readNumEntry( "Cdrecord buffer", 4 ) );
  }

  bool overburn = k3bMain()->config()->readBoolEntry( "Allow overburning", false );
  if( overburn )
    if( m_cdrecordBinObject->hasFeature("overburn") )
      *m_process << "-overburn";
    else
      emit infoMessage( i18n("Cdrecord version <= 1.10 does not support overburning!"), INFO );

  // additional parameters from config
  QStringList params = kapp->config()->readListEntry( "cdrecord parameters" );
  for( QStringList::Iterator it = params.begin(); it != params.end(); ++it )
    *m_process << *it;
}


K3bCdrecordWriter* K3bCdrecordWriter::addArgument( const QString& arg )
{
  *m_process << arg;
  return this;
}


void K3bCdrecordWriter::start()
{
  kdDebug() << "***** cdrecord parameters:\n";
  const QValueList<QCString>& args = m_process->args();
  QString s;
  for( QValueList<QCString>::const_iterator it = args.begin(); it != args.end(); ++it ) {
    s += *it + " ";
  }
  kdDebug() << s << endl << flush;



  emit newSubTask( i18n("Preparing write process...") );

  if( !m_process->start( KProcess::NotifyOnExit, m_stdin ? KProcess::All : KProcess::AllOutput ) ) {
    // something went wrong when starting the program
    // it "should" be the executable
    kdDebug() << "(K3bCdrecordWriter) could not start cdrecord" << endl;
    emit infoMessage( i18n("Could not start cdrecord!"), K3bJob::ERROR );
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
}


void K3bCdrecordWriter::cancel()
{
  if( m_process ) {
    if( m_process->isRunning() ) {
      m_process->disconnect();
      m_process->kill();

      // we need to unlock the writer because cdrecord locked it while writing
      bool block = burnDevice()->block( false );
      if( !block )
	emit infoMessage( i18n("Could not unlock CD drive."), K3bJob::ERROR );
      else if( k3bMain()->eject() )
	burnDevice()->eject();
    }
    
    emit infoMessage( i18n("Writing canceled."), K3bJob::ERROR ); 
    emit canceled();
    emit finished( false );
  }
}


bool K3bCdrecordWriter::write( const char* data, int len )
{
  return m_process->writeStdin( data, len );
}


void K3bCdrecordWriter::slotStdLine( const QString& line )
{
  if( line.startsWith( "Track" ) )
    {
      //			kdDebug() << "Parsing line [[" << line << "]]"endl;

      if( line.contains( "fifo", false ) > 0 )
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
	  emit processedSize( made, size );
	  emit percent( 100*made/size );
	}
    }
  else if( line.startsWith( "Starting new" ) )
    {
      emit newSubTask( i18n("Writing ISO data") );
    }
  else if( line.startsWith( "Fixating" ) ) {
    emit newSubTask( i18n("Fixating") );
  }
  else if( line.contains("seconds.") ) {
    emit infoMessage( "in " + line.mid( line.find("seconds") - 2 ), K3bJob::PROCESS );
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
    emit infoMessage( i18n("SAO, DAO recording not supported by the writer"), K3bJob::ERROR );
    emit infoMessage( i18n("Please turn off DAO (disk at once) and try again"), K3bJob::ERROR );
  }
  else {
    // debugging
    kdDebug() << "(cdrecord) " << line << endl;
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

      emit finished( true );
      break;

    default:
      // no recording device and also other errors!! :-(
      emit infoMessage( i18n("Cdrecord returned an error! (code %1)").arg(p->exitStatus()), K3bJob::ERROR );
      emit infoMessage( strerror(p->exitStatus()), K3bJob::ERROR );
      emit infoMessage( i18n("Please send me an email with the last output..."), K3bJob::ERROR );
      emit finished( false );
      break;
    }
  }
  else {
    emit infoMessage( i18n("Cdrecord did not exit cleanly."), K3bJob::ERROR );
    emit finished( false );
  }
}


#include "k3bcdrecordwriter.moc"
