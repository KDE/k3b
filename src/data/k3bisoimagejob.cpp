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
#include "../k3bglobals.h"
#include "../device/k3bdevice.h"
#include "../device/k3bemptydiscwaiter.h"

#include <kprocess.h>
#include <kconfig.h>
#include <klocale.h>
#include <kstddirs.h>

#include <qstring.h>
#include <qfile.h>

#include <iostream>


K3bIsoImageJob::K3bIsoImageJob()
  : K3bBurnJob()
{
  m_process = new KProcess();

  // connect to the cdrecord slots
  connect( m_process, SIGNAL(processExited(KProcess*)),
	   this, SLOT(slotCdrecordFinished()) );
  connect( m_process, SIGNAL(receivedStderr(KProcess*, char*, int)),
	   this, SLOT(slotParseCdrecordOutput(KProcess*, char*, int)) );
  connect( m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
	   this, SLOT(slotParseCdrecordOutput(KProcess*, char*, int)) );


  m_dao = true;
  m_dummy = false;
  m_burnproof = false;
  m_device = 0;
  m_speed = 1;
}

K3bIsoImageJob::~K3bIsoImageJob()
{
  delete m_process;
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


void K3bIsoImageJob::start()
{
  // check if everything is set
  if( m_device == 0 || !m_device->burner() ) {
    emit infoMessage( i18n("Please specify a cd writing device"), K3bJob::ERROR );
    emit finished( this );
    return;
  }


  if( !QFile::exists( m_imagePath ) ) {
    emit infoMessage( i18n("Could not find image %1").arg(m_imagePath), K3bJob::ERROR );
    emit finished( this );
    return;
  }


  K3bEmptyDiscWaiter* waiter = new K3bEmptyDiscWaiter( m_device, k3bMain() );
  connect( waiter, SIGNAL(discReady()), this, SLOT(slotStartWriting()) );
  connect( waiter, SIGNAL(canceled()), this, SLOT(cancel()) );
  waiter->waitForEmptyDisc();
}


void K3bIsoImageJob::slotStartWriting()
{
  emit started();

  emit newSubTask( i18n("Preparing write process...") );

  m_process->clearArguments();
	
  // now add the cdrecord parameters
  kapp->config()->setGroup( "External Programs" );
  *m_process << kapp->config()->readEntry( "cdrecord path" );
	
  // and now we add the needed arguments...
  // display progress
  *m_process << "-v";

  if( m_dummy )
    *m_process << "-dummy";
  if( m_dao )
    *m_process << "-dao";
  if( k3bMain()->eject() )
    *m_process << "-eject";
  if( m_burnproof && m_device->burnproof() )
    *m_process << "driveropts=burnproof";

  // add speed
  QString s = QString("-speed=%1").arg( m_speed );
  *m_process << s;

  // add the device (e.g. /dev/sg1)
  s = QString("-dev=%1").arg( m_device->genericDevice() );
  *m_process << s;

  // additional parameters from config
  QStringList _params = kapp->config()->readListEntry( "cdrecord parameters" );
  for( QStringList::Iterator it = _params.begin(); it != _params.end(); ++it )
    *m_process << *it;
			
  // debugging output
  cout << "***** cdrecord parameters:\n";
  QStrList* _args = m_process->args();
  QStrListIterator _it(*_args);
  while( _it ) {
    cout << *_it << " ";
    ++_it;
  }
  cout << endl << flush;

  *m_process << m_imagePath;
				


  if( !m_process->start( KProcess::NotifyOnExit, KProcess::AllOutput ) )
    {
      // something went wrong when starting the program
      // it "should" be the executable
      qDebug("(K3bIsoImageJob) could not start cdrecord");
      m_error = K3b::CDRECORD_ERROR;
      emit infoMessage( i18n("Could not start cdrecord!"), K3bJob::ERROR );
      emit finished( this );
    }
  else
    {
      m_error = K3b::WORKING;
      if( m_dummy )
	emit infoMessage( i18n("Starting simulation at %1x speed...").arg(m_speed), K3bJob::STATUS );
      else
	emit infoMessage( i18n("Starting recording at %1x speed...").arg(m_speed), K3bJob::STATUS );

      emit newTask( i18n("Writing ISO Image") );

    }
}


void K3bIsoImageJob::cancel()
{
  if( m_process->isRunning() ) {
    m_process->kill();

    // we need to unlock the writer because cdrecord locked it while writing
    bool block = m_device->block( false );
    if( !block )
      emit infoMessage( i18n("Could not unlock cd drive."), K3bJob::ERROR );
    //    else if( k3bMain()->eject() )
    // m_doc->burner()->eject();
  }

  m_error = K3b::CANCELED;
  emit infoMessage( i18n("Writing canceled."), K3bJob::ERROR );
  emit finished( this );
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
	  //			qDebug("Parsing line [[" + *str + "]]" );
			
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
		qDebug("parsing did not work");
		continue;
	      }
	      // now pos2 to the first colon :-)
	      num = (*str).mid(pos1,pos2-pos1).toInt(&ok);				
	      if(!ok)
		qDebug("parsing did not work");
				
	      // --- parse already written Megs -----------------------------------				
	      // ----------------------------------------------------------------------
	      pos1 = (*str).find(':');
	      if( pos1 == -1 ) {
		qDebug("parsing did not work");
		continue;
	      }
	      pos2 = (*str).find("of");
	      if( pos2 == -1 ) {
		qDebug("parsing did not work");
		continue;
	      }
	      // now pos1 point to the colon and pos2 to the 'o' of 'of' :-)
	      pos1++;
	      made = (*str).mid(pos1,pos2-pos1).toInt(&ok);
	      if(!ok)
		qDebug("parsing did not work");
					
	      // --- parse total size of track ---------------------------------------
	      // ------------------------------------------------------------------------
	      pos1 = (*str).find("MB");
	      if( pos1 == -1 ) {
		qDebug("parsing did not work");
		continue;
	      }
	      // now pos1 point to the 'M' of 'MB' and pos2 to the 'o' of 'of' :-)
	      pos2 += 2;
	      size = (*str).mid(pos2,pos1-pos2).toInt(&ok);
	      if(!ok)
		qDebug("parsing did not work");
				
	      // --- parse status of fifo --------------------------------------------
	      // ------------------------------------------------------------------------
	      pos1 = (*str).find("fifo");
	      if( pos1 == -1 ) {
		qDebug("parsing did not work");
		continue;
	      }
	      pos2 = (*str).find('%');
	      if( pos2 == -1 ) {
		qDebug("parsing did not work");
		continue;
	      }
	      // now pos1 point to the 'f' of 'fifo' and pos2 to the %o'  :-)
	      pos1+=4;
	      fifo = (*str).mid(pos1,pos2-pos1).toInt(&ok);
	      if(!ok)
		qDebug("parsing did not work");

	      // -------------------------------------------------------------------
	      // -------- parsing finished --------------------------------------

	      emit bufferStatus( fifo );
	      emit processedSize( made, size );

	      emit percent( 100*made/size );
	      emit subPercent( 100*made/size );
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
	cout << *str << endl;
      }
    } // for every line

}


void K3bIsoImageJob::slotCdrecordFinished()
{
  if( m_process->normalExit() )
    {
      // TODO: check the process' exitStatus()
      switch( m_process->exitStatus() )
	{
	case 0:
	  m_error = K3b::SUCCESS;
	  if( m_dummy )
	    emit infoMessage( i18n("Simulation successfully finished"), K3bJob::STATUS );
	  else
	    emit infoMessage( i18n("Writing successfully finished"), K3bJob::STATUS );
	  break;
				
	default:
	  // no recording device and also other errors!! :-(
	  emit infoMessage( i18n("Cdrecord returned some error! (code %1)").arg(m_process->exitStatus()), K3bJob::ERROR );
	  emit infoMessage( i18n("Sorry, no error handling yet! :-(("), K3bJob::ERROR );
	  emit infoMessage( i18n("Please send me a mail with the last output..."), K3bJob::ERROR );
	  m_error = K3b::CDRECORD_ERROR;
	  break;
	}
    }
  else
    {
      m_error = K3b::CDRECORD_ERROR;
      emit infoMessage( i18n("Cdrecord did not exit cleanly."), K3bJob::ERROR );
    }

		
  emit finished( this );
}
