/***************************************************************************
                          k3bdatajob.cpp  -  description
                             -------------------
    begin                : Tue May 15 2001
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

#include "k3bisoimagejob.h"

#include "../k3b.h"
#include "../tools/k3bglobals.h"
#include "../device/k3bdevice.h"
#include "../device/k3bemptydiscwaiter.h"
#include "../tools/k3bexternalbinmanager.h"

#include <kdebug.h>
#include <kprocess.h>
#include <kconfig.h>
#include <klocale.h>
#include <kstddirs.h>

#include <qstring.h>
#include <qfile.h>
#include <qtimer.h>

#include <iostream>


K3bIsoImageJob::K3bIsoImageJob()
  : K3bBurnJob()
{
  m_cdrecordProcess = new KProcess();
  m_cdrdaoProcess = new KProcess();

  // connect to the cdrecord slots
  connect( m_cdrecordProcess, SIGNAL(processExited(KProcess*)),
	   this, SLOT(slotCdrecordFinished()) );
  connect( m_cdrecordProcess, SIGNAL(receivedStderr(KProcess*, char*, int)),
	   this, SLOT(slotParseCdrecordOutput(KProcess*, char*, int)) );
  connect( m_cdrecordProcess, SIGNAL(receivedStdout(KProcess*, char*, int)),
	   this, SLOT(slotParseCdrecordOutput(KProcess*, char*, int)) );

  // connect to the cdrdao slots
  connect( m_cdrdaoProcess, SIGNAL(processExited(KProcess*)),
	   this, SLOT(slotCdrdaoFinished()) );
  connect( m_cdrdaoProcess, SIGNAL(receivedStderr(KProcess*, char*, int)),
	   this, SLOT(parseCdrdaoOutput(KProcess*, char*, int)) );
  connect( m_cdrdaoProcess, SIGNAL(receivedStdout(KProcess*, char*, int)),
	   this, SLOT(parseCdrdaoOutput(KProcess*, char*, int)) );



  m_dao = true;
  m_dummy = false;
  m_burnproof = false;
  m_rawWrite = false;
  m_device = 0;
  m_speed = 1;
}

K3bIsoImageJob::~K3bIsoImageJob()
{
  delete m_cdrecordProcess;
  delete m_cdrdaoProcess;
}


K3bDevice* K3bIsoImageJob::writer() const
{
  return m_device;
}


void K3bIsoImageJob::setImagePath( const QString& path )
{
  m_imagePath = path;
}

void K3bIsoImageJob::setSpeed( int speed )
{
  m_speed = speed;
}

void K3bIsoImageJob::setWriter( K3bDevice* d )
{
  m_device = d;
}

void K3bIsoImageJob::setBurnproof( bool burnproof )
{
  m_burnproof = burnproof;
}

void K3bIsoImageJob::setDao( bool dao )
{
  m_dao = dao;
}

void K3bIsoImageJob::setDummy( bool dummy )
{
  m_dummy = dummy;
}

void K3bIsoImageJob::setRawWrite( bool raw )
{
  m_rawWrite = raw;
}

void K3bIsoImageJob::setNoFix( bool noFix )
{
  m_noFix = noFix;
}

void K3bIsoImageJob::start()
{
  emit started();

  // check if everything is set
  if( m_device == 0 || !m_device->burner() ) {
    emit infoMessage( i18n("Please specify a cd writing device"), K3bJob::ERROR );
    emit finished( false );
    return;
  }


  if( !QFile::exists( m_imagePath ) ) {
    emit infoMessage( i18n("Could not find image %1").arg(m_imagePath), K3bJob::ERROR );
    emit finished( false );
    return;
  }


  emit newSubTask( i18n("Preparing write process...") );


  if( m_writeCueBin )
    QTimer::singleShot( 0, this, SLOT(slotWriteCueBin()) );
  else
    QTimer::singleShot( 0, this, SLOT(slotWrite()) );
}


void K3bIsoImageJob::slotWrite()
{
  m_cdrecordProcess->clearArguments();

  // use cdrecord to burn the cd
  if( !k3bMain()->externalBinManager()->foundBin( "cdrecord" ) ) {
    kdDebug() << "(K3bAudioJob) could not find cdrecord executable" << endl;
    emit infoMessage( i18n("Cdrecord executable not found."), K3bJob::ERROR );
    emit finished( false );
    return;
  }

  *m_cdrecordProcess << k3bMain()->externalBinManager()->binPath( "cdrecord" );
	
  // and now we add the needed arguments...
  // display progress
  *m_cdrecordProcess << "-v";

  k3bMain()->config()->setGroup( "General Options" );
  bool manualBufferSize = k3bMain()->config()->readBoolEntry( "Manual buffer size", false );
  if( manualBufferSize ) {
    *m_cdrecordProcess << QString("fs=%1m").arg( k3bMain()->config()->readNumEntry( "Cdrecord buffer", 4 ) );
  }
  bool overburn = k3bMain()->config()->readBoolEntry( "Allow overburning", false );

  if( overburn )
    *m_cdrecordProcess << "-overburn";

  if( m_dummy )
    *m_cdrecordProcess << "-dummy";
  if( m_rawWrite )
    *m_cdrecordProcess << "-raw";
  else if( m_dao )
    *m_cdrecordProcess << "-dao";
  if( m_noFix )
    *m_cdrecordProcess << "-nofix";
  if( k3bMain()->eject() )
    *m_cdrecordProcess << "-eject";
  if( m_burnproof && m_device->burnproof() )
    *m_cdrecordProcess << "driveropts=burnproof";

  // add speed
  QString s = QString("-speed=%1").arg( m_speed );
  *m_cdrecordProcess << s;

  // add the device (e.g. 0,0,0)
  s = QString("dev=%1").arg( m_device->busTargetLun() );
  *m_cdrecordProcess << s;

  // additional parameters from config
  QStringList _params = kapp->config()->readListEntry( "cdrecord parameters" );
  for( QStringList::Iterator it = _params.begin(); it != _params.end(); ++it )
    *m_cdrecordProcess << *it;

  *m_cdrecordProcess << "-data" << m_imagePath;
			

  cout << "***** cdrecord parameters:\n";
  const QValueList<QCString>& args = m_cdrecordProcess->args();
  for( QValueList<QCString>::const_iterator it = args.begin(); it != args.end(); ++it ) {
    cout << *it << " ";
  }
  cout << endl << flush;



  K3bEmptyDiscWaiter waiter( m_device, k3bMain() );
  if( waiter.waitForEmptyDisc() == K3bEmptyDiscWaiter::CANCELED ) {
    emit infoMessage( i18n("Writing canceled."), K3bJob::ERROR );
    emit canceled();
    emit finished( false );
    return;
  }


  if( !m_cdrecordProcess->start( KProcess::NotifyOnExit, KProcess::AllOutput ) )
    {
      // something went wrong when starting the program
      // it "should" be the executable
      kdDebug() << "(K3bIsoImageJob) could not start cdrecord" << endl;
      emit infoMessage( i18n("Could not start cdrecord!"), K3bJob::ERROR );
      emit finished( false );
    }
  else
    {
      if( m_dummy )
	emit infoMessage( i18n("Starting simulation at %1x speed...").arg(m_speed), K3bJob::STATUS );
      else
	emit infoMessage( i18n("Starting recording at %1x speed...").arg(m_speed), K3bJob::STATUS );

      emit newTask( i18n("Writing ISO Image") );
    }
}


void K3bIsoImageJob::slotWriteCueBin()
{
  m_cdrdaoProcess->clearArguments();


  if( !k3bMain()->externalBinManager()->foundBin( "cdrdao" ) ) {
    kdDebug() << "(K3bAudioJob) could not find cdrdao executable" << endl;
    emit infoMessage( i18n("Cdrdao executable not found."), K3bJob::ERROR );
    emit finished( false );
    return;
  }

  *m_cdrdaoProcess << k3bMain()->externalBinManager()->binPath( "cdrdao" );

  *m_cdrdaoProcess << "write";

  *m_cdrdaoProcess << "--device" << m_device->busTargetLun();
  if( m_device->cdrdaoDriver() != "auto" ) {
    *m_cdrdaoProcess << "--driver";
    if( m_device->cdTextCapable() == 1 )
      *m_cdrdaoProcess << QString("%1:0x00000010").arg( m_device->cdrdaoDriver() );
    else
      *m_cdrdaoProcess << m_device->cdrdaoDriver();
  }
    
  // additional parameters from config
  QStringList _params = kapp->config()->readListEntry( "cdrdao parameters" );
  for( QStringList::Iterator it = _params.begin(); it != _params.end(); ++it )
    *m_cdrdaoProcess << *it;

  k3bMain()->config()->setGroup( "General Options" );
  bool manualBufferSize = k3bMain()->config()->readBoolEntry( "Manual buffer size", false );
  if( manualBufferSize ) {
    *m_cdrdaoProcess << "--buffers" << QString::number( k3bMain()->config()->readNumEntry( "Cdrdao buffer", 32 ) );
  }
  bool overburn = k3bMain()->config()->readBoolEntry( "Allow overburning", false );

  if( overburn )
    *m_cdrdaoProcess << "--overburn";

  if( m_dummy )
    *m_cdrdaoProcess << "--simulate";
  if( k3bMain()->eject() )
    *m_cdrdaoProcess << "--eject";

  // writing speed
  *m_cdrdaoProcess << "--speed" << QString::number(  m_speed );
    
  // supress the 10 seconds gap to the writing
  *m_cdrdaoProcess << "-n";
    
  // cue-file
  *m_cdrdaoProcess << m_imagePath;



  K3bEmptyDiscWaiter waiter( m_device, k3bMain() );
  if( waiter.waitForEmptyDisc() == K3bEmptyDiscWaiter::CANCELED ) {
    emit infoMessage( i18n("Writing canceled."), K3bJob::ERROR );
    emit canceled();
    emit finished( false );
    return;
  }


  if( !m_cdrdaoProcess->start( KProcess::NotifyOnExit, KProcess::AllOutput ) )
    {
      // something went wrong when starting the program
      // it "should" be the executable
      kdDebug() << "(K3bIsoImageJob) could not start cdrdao" << endl;
      emit infoMessage( i18n("Could not start cdrdao!"), K3bJob::ERROR );
      emit finished( false );
    }
  else
    {
      if( m_dummy )
	emit infoMessage( i18n("Starting simulation at %1x speed...").arg(m_speed), K3bJob::STATUS );
      else
	emit infoMessage( i18n("Starting recording at %1x speed...").arg(m_speed), K3bJob::STATUS );

      emit newTask( i18n("Writing cue/bin image") );
    }
}


void K3bIsoImageJob::cancel()
{
  if( m_cdrdaoProcess->isRunning() || m_cdrecordProcess->isRunning() ) {
    m_cdrecordProcess->kill();
    m_cdrdaoProcess->kill();

    // we need to unlock the writer because cdrecord locked it while writing
    bool block = m_device->block( false );
    if( !block )
      emit infoMessage( i18n("Could not unlock cd drive."), K3bJob::ERROR );
    else if( k3bMain()->eject() )
      m_device->eject();
  }

  emit infoMessage( i18n("Writing canceled."), K3bJob::ERROR );
  emit canceled();
  emit finished( false );
}


void K3bIsoImageJob::slotParseCdrecordOutput( KProcess*, char* output, int len )
{
  QString buffer = QString::fromLatin1( output, len );
	

  emit debuggingOutput( "cdrecord", buffer );


  // split to lines
  QStringList lines = QStringList::split( "\n", buffer );
	
  // do every line
  for( QStringList::Iterator str = lines.begin(); str != lines.end(); str++ )
    {
      *str = (*str).stripWhiteSpace();
      if( (*str).startsWith( "Track" ) )
	{
	  //	kdDebug() << "Parsing line [[" << *str << "]]" << endl;
			
	  if( (*str).contains( "fifo", false ) > 0 )
	    {
	      // parse progress
	      int num, made, size, fifo;
	      bool ok;

	      // --- parse number of track ---------------------------------------				
	      // ----------------------------------------------------------------------
	      int pos1 = 5;
	      int pos2 = (*str).find(':');
	      if( pos1 == -1 ) {
		kdDebug() << "parsing did not work" << endl;
		continue;
	      }
	      // now pos2 to the first colon :-)
	      num = (*str).mid(pos1,pos2-pos1).toInt(&ok);				
	      if(!ok)
		kdDebug() << "parsing did not work" << endl;
				
	      // --- parse already written Megs -----------------------------------				
	      // ----------------------------------------------------------------------
	      pos1 = (*str).find(':');
	      if( pos1 == -1 ) {
		kdDebug() << "parsing did not work" << endl;
		continue;
	      }
	      pos2 = (*str).find("of");
	      if( pos2 == -1 ) {
		kdDebug() << "parsing did not work" << endl;
		continue;
	      }
	      // now pos1 point to the colon and pos2 to the 'o' of 'of' :-)
	      pos1++;
	      made = (*str).mid(pos1,pos2-pos1).toInt(&ok);
	      if(!ok)
		kdDebug() << "parsing did not work" << endl;
					
	      // --- parse total size of track ---------------------------------------
	      // ------------------------------------------------------------------------
	      pos1 = (*str).find("MB");
	      if( pos1 == -1 ) {
		kdDebug() << "parsing did not work" << endl;
		continue;
	      }
	      // now pos1 point to the 'M' of 'MB' and pos2 to the 'o' of 'of' :-)
	      pos2 += 2;
	      size = (*str).mid(pos2,pos1-pos2).toInt(&ok);
	      if(!ok)
		kdDebug() << "parsing did not work" << endl;
				
	      // --- parse status of fifo --------------------------------------------
	      // ------------------------------------------------------------------------
	      pos1 = (*str).find("fifo");
	      if( pos1 == -1 ) {
		kdDebug() << "parsing did not work" << endl;
		continue;
	      }
	      pos2 = (*str).find('%');
	      if( pos2 == -1 ) {
		kdDebug() << "parsing did not work" << endl;
		continue;
	      }
	      // now pos1 point to the 'f' of 'fifo' and pos2 to the %o'  :-)
	      pos1+=4;
	      fifo = (*str).mid(pos1,pos2-pos1).toInt(&ok);
	      if(!ok)
		kdDebug() << "parsing did not work" << endl;

	      // -------------------------------------------------------------------
	      // -------- parsing finished --------------------------------------

	      emit bufferStatus( fifo );
	      emit processedSize( made, size );

	      emit percent( 100*made/size );
	    }
	}
      else if( (*str).startsWith( "Starting new" ) )
	{
	  emit newSubTask( i18n("Writing iso data") );
	}
      else if( (*str).startsWith( "Fixating" ) ) {
	emit newSubTask( i18n("Fixating") );
      }
      else if( (*str).contains("seconds.") ) {
	emit infoMessage( "in " + (*str).mid( (*str).find("seconds") - 2 ), K3bJob::PROCESS );
      }
      else if( (*str).startsWith( "Writing pregap" ) ) {
	emit newSubTask( i18n("Writing pregap") );
      }
      else if( (*str).startsWith( "Performing OPC" ) ) {
	emit infoMessage( i18n("Performing OPC"), K3bJob::PROCESS );
      }
      else if( (*str).startsWith( "Sending" ) ) {
	emit infoMessage( i18n("Sending CUE sheet"), K3bJob::PROCESS );
      }
      else if( (*str).contains( "Turning BURN-Proof" ) ) {
	emit infoMessage( i18n("Enabled BURN-Proof"), K3bJob::PROCESS );
      }
      else {
	// debugging
	kdDebug() << "(cdrecord) " << (*str) << endl;
      }
    } // for every line

}


void K3bIsoImageJob::slotCdrecordFinished()
{
  bool unblock = false;

  if( m_cdrecordProcess->normalExit() )
    {
      // TODO: check the process' exitStatus()
      switch( m_cdrecordProcess->exitStatus() )
	{
	case 0:
	  if( m_dummy )
	    emit infoMessage( i18n("Simulation successfully finished"), K3bJob::STATUS );
	  else
	    emit infoMessage( i18n("Writing successfully finished"), K3bJob::STATUS );

	  emit finished( true );
	  break;
				
	default:
	  // no recording device and also other errors!! :-(
	  emit infoMessage( i18n("Cdrecord returned some error! (code %1)").arg(m_cdrecordProcess->exitStatus()), K3bJob::ERROR );
	  emit infoMessage( i18n("Sorry, no error handling yet! :-(("), K3bJob::ERROR );
	  emit infoMessage( i18n("Please send me a mail with the last output..."), K3bJob::ERROR );
	  emit finished( false );
	  unblock = true;
	  break;
	}
    }
  else
    {
      emit infoMessage( i18n("Cdrecord did not exit cleanly."), K3bJob::ERROR );
      emit finished( false );
      unblock = true;
    }

  if( unblock ) {
    // we need to unlock the writer because cdrecord locked it while writing
    bool block = m_device->block( false );
    if( !block )
      emit infoMessage( i18n("Could not unlock cd drive."), K3bJob::ERROR );
    else if( k3bMain()->eject() )
      m_device->eject();
  }
}


void K3bIsoImageJob::slotCdrdaoFinished()
{
  bool unblock = false;

  if( m_cdrdaoProcess->normalExit() )
    {
      // TODO: check the process' exitStatus()
      switch( m_cdrdaoProcess->exitStatus() )
	{
	case 0:
	  if( m_dummy )
	    emit infoMessage( i18n("Simulation successfully completed"), K3bJob::STATUS );
	  else
	    emit infoMessage( i18n("Writing successfully completed"), K3bJob::STATUS );

	  emit finished( true );
	  break;
				
	default:
	  // no recording device and also other errors!! :-(
	  emit infoMessage( i18n("Cdrdao returned some error! (code %1)").arg(m_cdrdaoProcess->exitStatus()), K3bJob::ERROR );
	  emit infoMessage( i18n("Sorry, no error handling yet!") + " :-((", K3bJob::ERROR );
	  emit infoMessage( i18n("Please send me a mail with the last output..."), K3bJob::ERROR );
	  unblock = true;

	  emit finished( false );
	  return;
	}
    }
  else
    {
      emit infoMessage( i18n("Cdrdao did not exit cleanly!"), K3bJob::ERROR );
      unblock = true;
      emit finished( false );
      return;
    }

  if( unblock ) {
    // we need to unlock the writer because cdrecord locked it while writing
    bool block = m_device->block( false );
    if( !block )
      emit infoMessage( i18n("Could not unlock cd drive."), K3bJob::ERROR );
    else if( k3bMain()->eject() )
      m_device->eject();
  }
}


#include "k3bisoimagejob.moc"
