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

#include "k3bdatajob.h"

#include "k3bdatadoc.h"
#include "k3bdataitem.h"
#include "k3bdiritem.h"
#include "../k3b.h"
#include "../k3bglobals.h"
#include "../device/k3bdevice.h"
#include "../device/k3bemptydiscwaiter.h"
#include "k3bdiritem.h"
#include "../tools/k3bexternalbinmanager.h"


#include <kprocess.h>
#include <kapp.h>
#include <kconfig.h>
#include <klocale.h>
#include <kstddirs.h>

#include <qstring.h>
#include <qstringlist.h>
#include <qdatetime.h>
#include <qtextstream.h>
#include <qfile.h>

#include <iostream>


K3bDataJob::K3bDataJob( K3bDataDoc* doc )
  : K3bBurnJob()
{
  m_doc = doc;
  m_process = 0;

  m_imageFinished = true;

  m_process = new KShellProcess();
}

K3bDataJob::~K3bDataJob()
{
}


K3bDoc* K3bDataJob::doc() const
{
  return m_doc;
}


K3bDevice* K3bDataJob::writer() const
{
  return doc()->burner();
}


void K3bDataJob::start()
{
  if( m_doc->multiSessionMode() == K3bDataDoc::CONTINUE ||
      m_doc->multiSessionMode() == K3bDataDoc::FINISH ) {

    K3bEmptyDiscWaiter waiter( m_doc->burner(), k3bMain() );
    if( waiter.waitForEmptyDisc( true ) == K3bEmptyDiscWaiter::CANCELED ) {
      cancel();
      return;
    }


    // check msinfo
    m_process->clearArguments();
    m_process->disconnect();

    if( !k3bMain()->externalBinManager()->foundBin( "cdrecord" ) ) {
      qDebug("(K3bAudioJob) could not find cdrecord executable" );
      emit infoMessage( i18n("Cdrecord executable not found."), K3bJob::ERROR );
      cancelAll();
      emit finished( false );
      return;
    }

    *m_process << k3bMain()->externalBinManager()->binPath( "cdrecord" );
    *m_process << "-msinfo";

    // add the device (e.g. /dev/sg1)
    *m_process << QString("dev=%1").arg( m_doc->burner()->genericDevice() );

    connect( m_process, SIGNAL(receivedStderr(KProcess*, char*, int)),
	     this, SLOT(slotParseMsInfo(KProcess*, char*, int)) );
    connect( m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
	     this, SLOT(slotParseMsInfo(KProcess*, char*, int)) );

    if( !m_process->start( KProcess::Block, KProcess::AllOutput ) ) {
      qDebug( "(K3bDataJob) could not start cdrecord" );
      emit infoMessage( i18n("Could not start cdrecord!"), K3bJob::ERROR );
      cancelAll();
      emit finished( false );
      return;
    }
		
    if( m_msInfo.isEmpty() ) {
      emit infoMessage( i18n("Could not retrieve multisession information from disk."), K3bJob::ERROR );
      emit infoMessage( i18n("The disk is either empty or not appendable."), K3bJob::ERROR );
      cancelAll();
      emit finished( false );
    }
  }




  if( m_doc->onTheFly() ) {
    m_pathSpecFile = locateLocal( "appdata", "temp/" ) + "k3b_" + QTime::currentTime().toString() + ".mkisofs";
    if( !writePathSpec( m_pathSpecFile ) ) {
      emit infoMessage( i18n("Could not write to temporary file %1").arg( m_pathSpecFile ), K3bJob::ERROR );
      cancelAll();
      emit finished( false );
    }
		
    // determine iso-size
    m_process->clearArguments();
    m_process->disconnect();

    if( !addMkisofsParameters() )
      return;

    *m_process << "-print-size" << "-q";
    // add empty dummy dir since one path-spec is needed
    *m_process << m_doc->dummyDir();
	
    connect( m_process, SIGNAL(receivedStderr(KProcess*, char*, int)),
	     this, SLOT(slotParseMkisofsSize(KProcess*, char*, int)) );
    connect( m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
	     this, SLOT(slotParseMkisofsSize(KProcess*, char*, int)) );
				
    if( !m_process->start( KProcess::Block, KProcess::AllOutput ) ) {
      qDebug( "(K3bDataJob) could not start mkisofs: %s", kapp->config()->readEntry( "mkisofs path" ).latin1() );
      emit infoMessage( i18n("Could not start mkisofs!"), K3bJob::ERROR );
      cancelAll();
      emit finished( false );
      return;
    }
		
    if( m_isoSize.isEmpty() ) {
      emit infoMessage( i18n("Could not retrieve size of data. On-the-fly writing did not work."), K3bJob::ERROR );
      emit infoMessage( i18n("Please creata an image first!"), K3bJob::ERROR );
      cancelAll();
      emit finished( false );
    }
    else {
      writeCD();
    }
  }
  else {
    writeImage();
  }
}


void K3bDataJob::writeCD()
{
  bool appendable = (m_doc->multiSessionMode() == K3bDataDoc::CONTINUE ||
		     m_doc->multiSessionMode() == K3bDataDoc::FINISH );

  K3bEmptyDiscWaiter waiter( m_doc->burner(), k3bMain() );
  if( waiter.waitForEmptyDisc( appendable ) == K3bEmptyDiscWaiter::CANCELED ) {
    cancel();
    return;
  }

  emit newSubTask( i18n("Preparing write process...") );

  // start the writing process -------------------------------------------------------------
  // create a kshellprocess and do it on the fly!

  m_process->clearArguments();
  m_process->disconnect();

  if( m_doc->onTheFly() ) {
    if( !addMkisofsParameters() ) {
      cancelAll();
      emit finished( false );
      return;
    }
				
    // add empty dummy dir since one path-spec is needed
    *m_process << m_doc->dummyDir();
    *m_process << "|";
  }
		
  // use cdrecord to burn the cd
  if( !k3bMain()->externalBinManager()->foundBin( "cdrecord" ) ) {
    qDebug("(K3bAudioJob) could not find cdrecord executable" );
    emit infoMessage( i18n("Cdrecord executable not found."), K3bJob::ERROR );
    cancelAll();
    emit finished( false );
    return;
  }

  *m_process << k3bMain()->externalBinManager()->binPath( "cdrecord" );

  // and now we add the needed arguments...
  // display progress
  *m_process << "-v";

  k3bMain()->config()->setGroup( "General Options" );
  bool manualBufferSize = k3bMain()->config()->readBoolEntry( "Manual buffer size", false );
  if( manualBufferSize ) {
    *m_process << QString("fs=%1").arg( k3bMain()->config()->readNumEntry( "Cdrecord buffer", 4 ) );
  }

  if( m_doc->dummy() )
    *m_process << "-dummy";

  // multisession
  if( m_doc->multiSessionMode() == K3bDataDoc::START ||
      m_doc->multiSessionMode() == K3bDataDoc::CONTINUE )
    *m_process << "-multi";
  else if( m_doc->multiSessionMode() == K3bDataDoc::NONE && m_doc->dao() )
    *m_process << "-dao";



  if( k3bMain()->eject() )
    *m_process << "-eject";
  if( m_doc->burnproof() && m_doc->burner()->burnproof() )
    *m_process << "driveropts=burnproof";

  // add speed
  QString s = QString("-speed=%1").arg( m_doc->speed() );
  *m_process << s;

  // add the device (e.g. /dev/sg1)
  s = QString("dev=%1").arg( m_doc->burner()->genericDevice() );
  *m_process << s;

  // additional parameters from config
  QStringList _params = kapp->config()->readListEntry( "cdrecord parameters" );
  for( QStringList::Iterator it = _params.begin(); it != _params.end(); ++it )
    *m_process << *it;

  if( m_doc->onTheFly() ) {
    // cdrecord needs to know that it should receive data from stdin
    s = "-tsize=" + m_isoSize;
    *m_process << s;
    *m_process << "-";
  }
  else
    *m_process << m_doc->isoImage();

		
  // debugging output
//   cout << "***** mkisofs parameters:\n";
//   QStrList* _args = m_process->args();
//   QStrListIterator _it(*_args);
//   while( _it ) {
//     cout << *_it << " ";
//     ++_it;
//   }
//   cout << endl << flush;

			
  // connect to the cdrecord slots
  connect( m_process, SIGNAL(processExited(KProcess*)),
	   this, SLOT(slotCdrecordFinished()) );
  connect( m_process, SIGNAL(receivedStderr(KProcess*, char*, int)),
	   this, SLOT(slotParseCdrecordOutput(KProcess*, char*, int)) );
  connect( m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
	   this, SLOT(slotParseCdrecordOutput(KProcess*, char*, int)) );
	
  if( !m_process->start( KProcess::NotifyOnExit, KProcess::AllOutput ) )
    {
      // something went wrong when starting the program
      // it "should" be the executable
      qDebug("(K3bDataJob) could not start mkisofs/cdrecord");
      emit infoMessage( i18n("Could not start mkisofs/cdrecord!"), K3bJob::ERROR );
      cancelAll();
      emit finished( false );
    }
  else
    {
      if( m_doc->multiSessionMode() == K3bDataDoc::START )
	emit infoMessage( i18n("Starting multi session disk."), K3bJob::PROCESS );
      else if( m_doc->multiSessionMode() == K3bDataDoc::CONTINUE )
	emit infoMessage( i18n("Appending session"), K3bJob::PROCESS );
      else if( m_doc->multiSessionMode() == K3bDataDoc::FINISH )
	emit infoMessage( i18n("Finishing multi session disk"), K3bJob::PROCESS );


      if( m_doc->dummy() )
	emit infoMessage( i18n("Starting simulation at %1x speed...").arg(m_doc->speed()), K3bJob::STATUS );
      else
	emit infoMessage( i18n("Starting recording at %1x speed...").arg(m_doc->speed()), K3bJob::STATUS );
	
      emit newTask( i18n("Writing ISO") );
      emit started();
    }
}


void K3bDataJob::writeImage()
{
  m_pathSpecFile = locateLocal( "appdata", "temp/" ) + "k3b_" + QTime::currentTime().toString() + ".mkisofs";
  if( !writePathSpec( m_pathSpecFile ) ) {
    emit infoMessage( i18n("Could not write to temporary file %1").arg( m_pathSpecFile ), K3bJob::ERROR );
    emit finished( false );
  }
	
  // get image file path
  if( m_doc->isoImage().isEmpty() )
    m_doc->setIsoImage( k3bMain()->findTempFile( "iso" ) );
		
  m_process->clearArguments();
  m_process->disconnect();
			
  if( !addMkisofsParameters() ) {
    cancelAll();
    emit finished( false );
    return;
  }
	
  *m_process << "-o" << m_doc->isoImage();
	
  // add empty dummy dir since one path-spec is needed
  *m_process << m_doc->dummyDir();

  // connect to the mkisofs slots
  connect( m_process, SIGNAL(processExited(KProcess*)),
	   this, SLOT(slotMkisofsFinished()) );
  connect( m_process, SIGNAL(receivedStderr(KProcess*, char*, int)),
	   this, SLOT(slotParseMkisofsOutput(KProcess*, char*, int)) );
  //	connect( m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
  //			 this, SLOT(slotParseMkisofsOutput(KProcess*, char*, int)) );
		
  if( !m_process->start( KProcess::NotifyOnExit, KProcess::AllOutput ) )
    {
      // something went wrong when starting the program
      // it "should" be the executable
      qDebug("(K3bDataJob) could not start mkisofs");
				
      // remove pathspec-file
      QFile::remove( m_pathSpecFile );
      m_pathSpecFile = QString::null;
		
      emit infoMessage( i18n("Could not start mkisofs!"), K3bJob::ERROR );
      cancelAll();
      emit finished( false );
    }
  else
    {
      m_imageFinished = false;
      emit infoMessage( i18n("Creating ISO-image in %1").arg(m_doc->isoImage()), K3bJob::STATUS );
      emit newSubTask( i18n("Creating ISO-image") );
      emit started();
    }
}


void K3bDataJob::cancel()
{
  if( m_process->isRunning() ) {
    m_process->kill();

    // we need to unlock the writer because cdrecord locked it while writing
    bool block = m_doc->burner()->block( false );
    if( !block )
      emit infoMessage( i18n("Could not unlock cd drive."), K3bJob::ERROR );
    //    else if( k3bMain()->eject() )
    // m_doc->burner()->eject();
      
	
    // remove toc-file
    if( QFile::exists( m_pathSpecFile ) ) {
      QFile::remove( m_pathSpecFile );
      m_pathSpecFile = QString::null;
    }

    // remove iso-image if it is unfinished or the user selected to remove image
    if( QFile::exists( m_doc->isoImage() ) ) {
      if( m_doc->deleteImage() || !m_imageFinished ) {
	emit infoMessage( i18n("Removing iso-image %1").arg(m_doc->isoImage()), K3bJob::STATUS );
	QFile::remove( m_doc->isoImage() );
	m_doc->setIsoImage("");
	m_pathSpecFile = QString::null;
      }
      else {
	emit infoMessage( i18n("Image successfully created in %1").arg(m_doc->isoImage()), K3bJob::STATUS );
      }
    }
  }	

  emit infoMessage( i18n("Writing canceled."), K3bJob::ERROR );
  emit finished( false );
}


void K3bDataJob::slotParseMkisofsOutput( KProcess*, char* output, int len )
{
  QString buffer = QString::fromLatin1( output, len );


  emit debuggingOutput( "mkisofs", buffer );

	
  // split to lines
  QStringList lines = QStringList::split( "\n", buffer );
	
  // do every line
  for( QStringList::Iterator str = lines.begin(); str != lines.end(); str++ )
    {
      if( (*str).contains( "done, estimate" ) ) {

	QString _perStr = *str;
	_perStr.truncate( _perStr.find('%') );
	bool ok;
	double _percent = _perStr.toDouble( &ok );
	if( !ok ) {
	  qDebug( "Parsing did not work for " + _perStr );
	}
	else {
	  emit subPercent( (int)_percent );
	  if( m_doc->onlyCreateImage() )
	    emit percent( (int)_percent );
	  else
	    emit percent( (int)(_percent / 2.0) );
	}
      }
      else if( (*str).contains( "extents written" ) ) {

	emit subPercent( 100 );
	if( m_doc->onlyCreateImage() )
	  emit percent( 100 );
	else
	  emit percent( 50 );
      }

      else {
	qDebug("(mkisofs) " + *str );
      }
    }
}


void K3bDataJob::slotParseCdrecordOutput( KProcess*, char* output, int len )
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

	      // when writing an image the image creating process
	      // is treated as half of the whole process
	      // when writing on-the-fly subPercent is parsed
	      // from the mkisofs output 

	      if( m_doc->onTheFly() ) {
		emit percent( 100*made/size );
	      }
	      else {
		emit subPercent( 100*made/size );
		emit percent( 50 + 50*made/size );
	      }

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
      else if( (*str).contains( "done, estimate" ) ) {

	// mkisofs percent output
	// only avaliable in on-the-fly mode

	QString _perStr = *str;
	_perStr.truncate( _perStr.find('%') );
	bool ok;
	int _percent = (int)_perStr.toDouble( &ok );
	if( !ok ) {
	  qDebug( "Parsing did not work for " + _perStr );
	}
	else
	  emit subPercent( _percent );
      }
      else if( (*str).contains( "extents written" ) ) {

	// mkisofs finishing output
	// only avaliable in on-the-fly mode
	emit subPercent( 100 );
      }
      else {
	// debugging
	qDebug("(cdrecord) " + *str);
      }
    } // for every line

}


void K3bDataJob::slotMkisofsFinished()
{
  if( m_process->normalExit() )
    {
      // TODO: check the process' exitStatus()
      switch( m_process->exitStatus() )
	{
	case 0:
	  emit infoMessage( i18n("Image successfully created in %1").arg(m_doc->isoImage()), K3bJob::STATUS );
	  m_imageFinished = true;
				
	  if( m_doc->onlyCreateImage() ) {

	    // weird, but possible
	    if( m_doc->deleteImage() ) {
	      QFile::remove( m_doc->isoImage() );
	      m_doc->setIsoImage("");
	      emit infoMessage( i18n("Removed image file %1").arg(m_doc->isoImage()), K3bJob::STATUS );
	    }

	    emit finished( true );
	  }
	  else {
	    writeCD();
	  }
	  break;
				
	default:
	  emit infoMessage( i18n("Mkisofs returned some error. (code %1)").arg(m_process->exitStatus()), K3bJob::ERROR );
	  emit infoMessage( i18n("Sorry, no error handling yet! :-(("), K3bJob::ERROR );
	  emit infoMessage( i18n("Please send me a mail with the last output..."), K3bJob::ERROR );
	  emit finished( false );
	  break;
	}
    }
  else
    {
      emit infoMessage( i18n("Mkisofs did not exit cleanly."), K3bJob::ERROR );
      emit finished( false );
    }

  // remove toc-file
  if( QFile::exists( m_pathSpecFile ) ) {
    QFile::remove( m_pathSpecFile );
    m_pathSpecFile = QString::null;
  }

  //	m_process->disconnect();
}


void K3bDataJob::slotCdrecordFinished()
{
  if( m_process->normalExit() )
    {
      // TODO: check the process' exitStatus()
      switch( m_process->exitStatus() )
	{
	case 0:
	  if( m_doc->dummy() )
	    emit infoMessage( i18n("Simulation successfully finished"), K3bJob::STATUS );
	  else
	    emit infoMessage( i18n("Writing successfully finished"), K3bJob::STATUS );

	  emit finished( true );
	  break;
				
	default:
	  // no recording device and also other errors!! :-(
	  emit infoMessage( i18n("Cdrecord returned some error! (code %1)").arg(m_process->exitStatus()), K3bJob::ERROR );
	  emit infoMessage( i18n("Sorry, no error handling yet! :-(("), K3bJob::ERROR );
	  emit infoMessage( i18n("Please send me a mail with the last output..."), K3bJob::ERROR );
	  emit finished( false );
	  break;
	}
    }
  else
    {
      emit infoMessage( i18n("Cdrecord did not exit cleanly."), K3bJob::ERROR );
      emit finished( false );
    }

  // remove path-spec-file
  if( QFile::exists( m_pathSpecFile ) ) {
    QFile::remove( m_pathSpecFile );
    m_pathSpecFile = QString::null;
  }

  if( m_doc->deleteImage() ) {
    QFile::remove( m_doc->isoImage() );
    m_doc->setIsoImage("");
  }
		
  m_process->disconnect();
}



bool K3bDataJob::addMkisofsParameters()
{
  if( !k3bMain()->externalBinManager()->foundBin( "mkisofs" ) ) {
    qDebug("(K3bAudioJob) could not find mkisofs executable" );
    emit infoMessage( i18n("Mkisofs executable not found."), K3bJob::ERROR );
    return false;
  }

  *m_process << k3bMain()->externalBinManager()->binPath( "mkisofs" );
	
  // add the arguments
  *m_process << "-gui";
  *m_process << "-graft-points";

  if( !m_doc->volumeID().isEmpty() )
    *m_process << "-V \"" + m_doc->volumeID() + "\"";
  if( !m_doc->applicationID().isEmpty() )
    *m_process << "-A \"" + m_doc->applicationID() + "\"";
  if( !m_doc->publisher().isEmpty() )
    *m_process << "-P \"" + m_doc->publisher() + "\"";
  if( !m_doc->preparer().isEmpty() )
    *m_process << "-p \"" + m_doc->preparer() + "\"";
		
  if( m_doc->createRockRidge() )
    *m_process << "-r";
  if( m_doc->createJoliet() )
    *m_process << "-J";

  if( m_doc->ISOuntranslatedFilenames()  ) {
    *m_process << "-U";
  }
  else {
    if( m_doc->ISOallowPeriodAtBegin()  )
      *m_process << "-L";
    if( m_doc->ISOallow31charFilenames()  )
      *m_process << "-l";	
    if( m_doc->ISOomitVersionNumbers() && !m_doc->ISOmaxFilenameLength() )	
      *m_process << "-N";		
    if( m_doc->ISOrelaxedFilenames()  )
      *m_process << "-relaxed-filenames";		
    if( m_doc->ISOallowLowercase()  )
      *m_process << "-allow-lowercase";		
    if( m_doc->ISOnoIsoTranslate()  )
      *m_process << "-no-iso-translate";
    if( m_doc->ISOallowMultiDot()  )
      *m_process << "-allow-multidot";
  }
		
  if( m_doc->ISOmaxFilenameLength()  )
    *m_process << "-max-iso9660-filenames";	
  if( m_doc->noDeepDirectoryRelocation()  )
    *m_process << "-D";	


  // this should be handled internally by K3b.
  // the config dialog should give the option to choose between using symlinks and not
  //  if( m_doc->followSymbolicLinks()  )
    *m_process << "-f";	


  if( m_doc->hideRR_MOVED()  )
    *m_process << "-hide-rr-moved";	
  if( m_doc->createTRANS_TBL()  )
    *m_process << "-T";	
  if( m_doc->hideTRANS_TBL()  )
    *m_process << "-hide-joliet-trans-tbl";	
  if( m_doc->padding()  )
    *m_process << "-pad";	

  *m_process << "-iso-level" << QString::number(m_doc->ISOLevel());

  *m_process << "-path-list" << QFile::encodeName(m_pathSpecFile);


  // add multisession info
  if( m_doc->multiSessionMode() == K3bDataDoc::CONTINUE ||
      m_doc->multiSessionMode() == K3bDataDoc::FINISH ) {

    // it has to be the device we are writing to cause only this makes sense
    *m_process << "-M" << m_doc->burner()->genericDevice();
    *m_process << "-C" << m_msInfo;
  }

	
  // additional parameters from config
  QStringList _params = kapp->config()->readListEntry( "mkisofs parameters" );
  for( QStringList::Iterator it = _params.begin(); it != _params.end(); ++it )
    *m_process << *it;

  return true;
}



void K3bDataJob::slotParseMkisofsSize(KProcess*, char* output, int len)
{
  QString buffer = QString::fromLatin1( output, len ).stripWhiteSpace();
  qDebug("*** parsing line: " + buffer );

  // this seems to be the format for mkisofs version < 1.14 (to stdout)
  if( buffer.contains( "=" ) )
    m_isoSize = buffer.mid( buffer.find('=') + 1 ).stripWhiteSpace() + "s";

  // and mkisofs >= 1.14 prints out only the number (to stderr)
  else {
    bool ok;
    buffer.toInt( &ok );
    if( ok )
      m_isoSize = buffer + "s";
  }

  qDebug("ISO-Size should be: " + m_isoSize );
}


void K3bDataJob::slotParseMsInfo( KProcess*, char* output, int len )
{
  QString buffer = QString::fromLatin1( output, len ).stripWhiteSpace();
  if( buffer.startsWith("cdrecord:") ) {
    qDebug("(K3bDataJob) msinfo error: " + buffer );
    m_msInfo = QString::null;
  }
  else if( buffer.contains(",") ) {
    qDebug("(K3bDataJob) found msinfo? " + buffer );
    QStringList list = QStringList::split( ",", buffer );
    if( list.count() == 2 ) {
      bool ok1, ok2;
      list.first().toInt( &ok1 );
      list[1].toInt( &ok2 );
      if( ok1 && ok2 )
	m_msInfo = buffer;
      else
	m_msInfo = QString::null;
    }
    else {
      m_msInfo = QString::null;
    }
  }
}


bool K3bDataJob::writePathSpec( const QString& filename )
{
  QFile file( filename );
  if( !file.open( IO_WriteOnly ) ) {
    return false;
  }
	
  QTextStream t(&file);

  // start writing the path-specs
  // iterate over all the dataItems
  K3bDataItem* item = m_doc->root()->nextSibling();
	
  while( item ) {
    t << m_doc->treatWhitespace(item->k3bPath()) << "=" << item->localPath() << "\n";
		
    item = item->nextSibling();
  }
	

  file.close();
  
  return filename;
}

/*
void K3bDataJob::splitDoc()
{
  m_splittedLists.setAutoDelete( true );
  m_splittedLists.clear();

  // very easy and dump first splitting

  K3bDataItem* item = m_doc->root();
  long size = 0;
  QPtrList<K3bDataItem> *newList = new QList<K3bDataItem>();

  while( item ) {

    item = item->nextSibling();

    if( size + item->size() < 650 ) {    // we should let the user choose the size of the images
      newList->append( item );
      size += item->size();
    }
    else {
      m_splittedLists.append( newList );
      newList = new QPtrList<K3bDataItem>();
      size = 0;
    }
  }
}
*/


void K3bDataJob::cancelAll()
{
  if( m_process )
    if( m_process->isRunning() ) {
      m_process->disconnect(this);
      m_process->kill();
      
      // we need to unlock the writer because cdrdao/cdrecord locked it while writing
      bool block = m_doc->burner()->block( false );
      if( !block )
	emit infoMessage( i18n("Could not unlock cd drive."), K3bJob::ERROR );
    }

  // remove toc-file
//   if( QFile::exists( m_tocFile ) ) {
//      qDebug("(K3bAudioOnTheFlyJob) Removing temporary TOC-file");
//      QFile::remove( m_tocFile );
//   }
//   m_tocFile = QString::null;

  // remove path-spec-file
  if( QFile::exists( m_pathSpecFile ) ) {
    QFile::remove( m_pathSpecFile );
    m_pathSpecFile = QString::null;
  }

  if( m_doc->deleteImage() ) {
    QFile::remove( m_doc->isoImage() );
    m_doc->setIsoImage("");
  }
}


#include "k3bdatajob.moc"
