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
#include "../k3b.h"
#include "../k3bglobals.h"
#include "../device/k3bdevice.h"

#include <kprocess.h>
#include <kapp.h>
#include <kconfig.h>
#include <klocale.h>
#include <kstddirs.h>

#include <qstring.h>
#include <qstringlist.h>
#include <qdatetime.h>

#include <iostream>


K3bDataJob::K3bDataJob( K3bDataDoc* doc )
  : K3bBurnJob()
{
  m_doc = doc;
  m_process = 0;

  m_imageFinished = true;
}

K3bDataJob::~K3bDataJob()
{
}


K3bDoc* K3bDataJob::doc() const
{
  return m_doc;
}


void K3bDataJob::start()
{
  if( m_process ) {
    delete m_process;
    m_process = 0;
  }
  if( m_doc->onTheFly() ) {
    m_pathSpecFile = locateLocal( "appdata", "temp/" ) + "k3b_" + QTime::currentTime().toString() + ".mkisofs";
    if( m_doc->writePathSpec( m_pathSpecFile ) == QString::null ) {
      emit infoMessage( i18n("Could not write to temporary file %1").arg( m_pathSpecFile ) );
      emit finished( this );
    }
		
    // determine iso-size
    m_process = new KProcess();
    addMkisofsParameters();
    *m_process << "-print-size" << "-q";
    // add empty dummy dir since one path-spec is needed
    *m_process << m_doc->dummyDir();
	
    connect( m_process, SIGNAL(receivedStderr(KProcess*, char*, int)),
	     this, SLOT(slotParseMkisofsSize(KProcess*, char*, int)) );
    connect( m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
	     this, SLOT(slotParseMkisofsSize(KProcess*, char*, int)) );
				
    if( !m_process->start( KProcess::Block, KProcess::AllOutput ) ) {
      qDebug( "(K3bDataJob) could not start mkisofs: %s", kapp->config()->readEntry( "mkisofs path" ).latin1() );
      emit infoMessage( i18n("Could not start mkisofs!") );
      m_error = K3b::MKISOFS_ERROR;
      delete m_process;
      emit finished(this);
      return;
    }
		
    delete m_process;
		
    // start the writing process -------------------------------------------------------------
    // create a kshellprocess and do it on the fly!
    m_process = new KShellProcess();

    addMkisofsParameters();
				
    // add empty dummy dir since one path-spec is needed
    *m_process << m_doc->dummyDir();
    *m_process << "|";
		
    // now add the cdrecord parameters
    *m_process << kapp->config()->readEntry( "cdrecord path" );

    // and now we add the needed arguments...
    // display progress
    *m_process << "-v";

    if( m_doc->dummy() )
      *m_process << "-dummy";
    if( m_doc->dao() )
      *m_process << "-dao";
    if( k3bMain()->eject() )
      *m_process << "-eject";
    if( m_doc->burnProof() && m_doc->burner()->burnproof() )
      *m_process << "driveropts=burnproof";

    // add speed
    QString s = QString("-speed=%1").arg( m_doc->speed() );
    *m_process << s;

    // add the device (e.g. /dev/sg1)
    s = QString("-dev=%1").arg( m_doc->burner()->devicename() );
    *m_process << s;

    // additional parameters from config
    QStringList _params = kapp->config()->readListEntry( "cdrecord parameters" );
    for( QStringList::Iterator it = _params.begin(); it != _params.end(); ++it )
      *m_process << *it;

    // cdrecord needs to know that it should receive data from stdin
    s = "-tsize=" + m_isoSize;
    *m_process << s;
    *m_process << "-";
			
    // debugging output
    cout << "***** mkisofs parameters:\n";
    QStrList* _args = m_process->args();
    QStrListIterator _it(*_args);
    while( _it ) {
      cout << *_it << " ";
      ++_it;
    }
    cout << endl << flush;

			
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
	m_error = K3b::CDRECORD_ERROR;
	emit infoMessage( "could not start mkisofs/cdrecord!" );
	emit finished( this );
      }
    else
      {
	m_error = K3b::WORKING;
	emit infoMessage( i18n("Start recording at %1x speed...").arg(m_doc->speed()) );
	emit newTask( i18n("Writing ISO") );
	emit started();
      }

  }
  else {
    m_process = new KProcess();
    writeImage();
  }
}


void K3bDataJob::writeImage()
{
  emit newTask( "Writing ISO-image" );
	
  m_pathSpecFile = locateLocal( "appdata", "temp/" ) + "k3b_" + QTime::currentTime().toString() + ".mkisofs";
  m_doc->writePathSpec( m_pathSpecFile );
	
  // get image file path
  if( m_doc->isoImage().isEmpty() )
    m_doc->setIsoImage( k3bMain()->findTempFile( "iso" ) );
		
  m_process->clearArguments();
  m_process->disconnect();
			
  addMkisofsParameters();
	
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
      m_error = K3b::MKISOFS_ERROR;
				
      // remove pathspec-file
      QFile::remove( m_pathSpecFile );
      m_pathSpecFile = QString::null;
		
      emit infoMessage( "Could not start mkisofs!" );
      emit finished( this );
    }
  else
    {
      m_error = K3b::WORKING;
      m_imageFinished = false;
      emit infoMessage( i18n("Start writing iso-image to %1").arg(m_doc->isoImage()) );
      emit newTask( i18n("Writing ISO-image") );
      emit started();
    }
}


void K3bDataJob::writeCD()
{
  m_process->clearArguments();
  m_process->disconnect();
	
  // now add the cdrecord parameters
  kapp->config()->setGroup( "External Programs" );
  *m_process << kapp->config()->readEntry( "cdrecord path" );
	
  // and now we add the needed arguments...
  // display progress
  *m_process << "-v";

  if( m_doc->dummy() )
    *m_process << "-dummy";
  if( m_doc->dao() )
    *m_process << "-dao";
  if( k3bMain()->eject() )
    *m_process << "-eject";
  if( m_doc->burnProof() && m_doc->burner()->burnproof() )
    *m_process << "driveropts=burnproof";

  // add speed
  QString s = QString("-speed=%1").arg( m_doc->speed() );
  *m_process << s;

  // add the device (e.g. /dev/sg1)
  s = QString("-dev=%1").arg( m_doc->burner()->devicename() );
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

  *m_process << m_doc->isoImage();
				
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
      qDebug("(K3bDataJob) could not start cdrecord");
      m_error = K3b::CDRECORD_ERROR;
      emit infoMessage( "could not start cdrecord!" );
      emit finished( this );
    }
  else
    {
      m_error = K3b::WORKING;
      emit infoMessage( i18n("Start recording at %1x speed...").arg(m_doc->speed()) );
      emit newTask( i18n("Writing ISO") );
    }
}


void K3bDataJob::cancel()
{
  if( m_process->isRunning() ) {
    m_process->kill();
    emit infoMessage("Writing canceled.");
	
    // remove toc-file
    if( QFile::exists( m_pathSpecFile ) ) {
      QFile::remove( m_pathSpecFile );
      m_pathSpecFile = QString::null;
    }

    // remove iso-image if it is unfinished or the user selected to remove image
    if( QFile::exists( m_doc->isoImage() ) ) {
      if( m_doc->deleteImage() || !m_imageFinished ) {
	emit infoMessage( i18n("Removing iso-image %s", m_doc->isoImage().latin1() ) );
	QFile::remove( m_doc->isoImage() );
	m_pathSpecFile = QString::null;
      }
      else {
	emit infoMessage( i18n("Image successfully created in ") + m_doc->isoImage() );	
      }
    }
				
    m_error = K3b::CANCELED;
    emit finished( this );
  }
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
      if( (*str).at(6) == '%' ) {     // percent
	QString _perStr = *str;
	_perStr.truncate(3);
	bool ok;
	int _percent = _perStr.toInt( &ok );
	if( !ok ) {
	  qDebug( "Parsing did not work for " + _perStr );
	}
	else {
	  emit percent( _percent );
	}
      }
      else {
	cout << (*str).latin1() << endl;
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

				
	      emit percent( 100*made/size );
	      emit processedSize( made, size );
	      emit bufferStatus( fifo );
	    }
	}
      else if( (*str).startsWith( "Starting new" ) )
	{
	  emit newSubTask( i18n("Writing data") );
	}
      else if( (*str).startsWith( "Fixating" ) ) {
	emit newSubTask( i18n("Fixating") );
      }
      else if( (*str).contains("seconds.") ) {
	emit infoMessage( (*str).stripWhiteSpace() + " to start of writing..." );
      }
      else if( (*str).startsWith( "Writing pregap" ) ) {
	emit newSubTask( i18n("Writing pregap") );
      }
      else if( (*str).startsWith( "Performing OPC" ) ) {
	emit infoMessage( i18n("Performing OPC") );
      }
      else if( (*str).startsWith( "Sending" ) ) {
	emit infoMessage( i18n("Sending CUE sheet") );
      }
      else if( (*str).contains( "Turning BURN-Proof" ) ) {
	emit infoMessage( i18n("Enabled BURN-Proof") );
      }
      else {
	// debugging
	cout << *str << endl;
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
	  m_error = K3b::SUCCESS;
	  emit percent( 100 );
	  emit infoMessage( "Image successfully created" );
	  m_imageFinished = true;
				
	  if( m_doc->onlyCreateImage() ) {
	    emit infoMessage( i18n("Image successfully created in ") + m_doc->isoImage() );
	    emit finished(this);
	  }
	  else {
	    writeCD();
	    return;
	  }
	  break;
				
	default:
	  emit infoMessage( "Mkisofs returned some error!" );
	  emit infoMessage( "Sorry, no error handling yet! :-((" );
	  emit infoMessage( "Please send me a mail with the last output..." );
	  m_error = K3b::MKISOFS_ERROR;
	  break;
	}
    }
  else
    {
      m_error = K3b::MKISOFS_ERROR;
      emit infoMessage( "Mkisofs did not exit cleanly!" );
    }

  // remove toc-file
  if( QFile::exists( m_pathSpecFile ) ) {
    QFile::remove( m_pathSpecFile );
    m_pathSpecFile = QString::null;
  }

  emit finished( this );

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
	  m_error = K3b::SUCCESS;
	  emit infoMessage( "Burning successfully finished" );
	  break;
				
	default:
	  // no recording device and also other errors!! :-(
	  emit infoMessage( "Cdrecord returned some error!" );
	  emit infoMessage( "Sorry, no error handling yet! :-((" );
	  emit infoMessage( "Please send me a mail with the last output..." );
	  m_error = K3b::CDRECORD_ERROR;
	  break;
	}
    }
  else
    {
      m_error = K3b::CDRECORD_ERROR;
      emit infoMessage( "cdrecord did not exit cleanly!" );
    }

  // remove toc-file
  if( QFile::exists( m_pathSpecFile ) ) {
    QFile::remove( m_pathSpecFile );
    m_pathSpecFile = QString::null;
  }

  // remove image-file
  if( m_doc->deleteImage() ) {
    QFile::remove( m_doc->isoImage() );
    m_doc->setIsoImage("");
  }
		
  emit finished( this );

  m_process->disconnect();
}



void K3bDataJob::addMkisofsParameters()
{
  kapp->config()->setGroup( "External Programs" );
  *m_process << kapp->config()->readEntry( "mkisofs path" );
	
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
  if( m_doc->followSymbolicLinks()  )
    *m_process << "-f";	
  if( m_doc->hideRR_MOVED()  )
    *m_process << "-hide-rr-moved";	
  if( m_doc->createTRANS_TBL()  )
    *m_process << "-T";	
  if( m_doc->hideTRANS_TBL()  )
    *m_process << "-hide-joliet-trans-tbl";	
  if( m_doc->padding()  )
    *m_process << "-pad";	

  *m_process << "-path-list" << m_pathSpecFile;
	
  // additional parameters from config
  QStringList _params = kapp->config()->readListEntry( "mkisofs parameters" );
  for( QStringList::Iterator it = _params.begin(); it != _params.end(); ++it )
    *m_process << *it;
}



void K3bDataJob::slotParseMkisofsSize(KProcess*, char* output, int len)
{
  QString buffer = QString::fromLatin1( output, len ).stripWhiteSpace();
  qDebug("*** parsing line: " + buffer );

  // this seems to be the format for mkisofs verion < 1.14
  if( buffer.contains( "=" ) )
    m_isoSize = buffer.mid( buffer.find('=') + 1 ).stripWhiteSpace() + "s";

  // and mkisofs >= 1.14 prints only the number
  else {
    bool ok;
    buffer.toInt( &ok );
    if( ok )
      m_isoSize = buffer + "s";
  }

  qDebug("ISO-Size should be: " + m_isoSize );
}
