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
#include "../tools/k3bglobals.h"
#include "../device/k3bdevice.h"
#include "../device/k3bemptydiscwaiter.h"
#include "k3bdiritem.h"
#include "../tools/k3bexternalbinmanager.h"


#include <kprocess.h>
#include <kapplication.h>
#include <kconfig.h>
#include <klocale.h>
#include <kstandarddirs.h>

#include <qstring.h>
#include <qstringlist.h>
#include <qdatetime.h>
#include <qtextstream.h>
#include <qfile.h>
#include <qregexp.h>
#include <kdebug.h>




K3bDataJob::K3bDataJob( K3bDataDoc* doc )
  : K3bBurnJob()
{
  m_doc = doc;
  m_process = 0;
  m_noDeepDirectoryRelocation = false;

  m_imageFinished = true;
}

K3bDataJob::~K3bDataJob()
{
  delete m_process;
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
  // write path spec file
  // ----------------------------------------------------
  m_pathSpecFile = locateLocal( "appdata", "temp/k3b_path_spec.mkisofs" );
  if( !writePathSpec( m_pathSpecFile ) ) {
    emit infoMessage( i18n("Could not write to temporary file %1").arg( m_pathSpecFile ), K3bJob::ERROR );
    cancelAll();
    return;
  }

  if( m_doc->isoOptions().createRockRidge() ) {
    m_rrHideFile = locateLocal( "appdata", "temp/k3b_rr_hide.mkisofs" );
    if( !writeRRHideFile( m_rrHideFile ) ) {
      emit infoMessage( i18n("Could not write to temporary file %1").arg( m_rrHideFile ), K3bJob::ERROR );
      cancelAll();
      return;
    }
  }

  if( m_doc->isoOptions().createJoliet() ) {
    m_jolietHideFile = locateLocal( "appdata", "temp/k3b_joliet_hide.mkisofs" );
    if( !writeJolietHideFile( m_jolietHideFile ) ) {
      emit infoMessage( i18n("Could not write to temporary file %1").arg( m_rrHideFile ), K3bJob::ERROR );
      cancelAll();
      return;
    }
  }

  if( m_doc->multiSessionMode() == K3bDataDoc::CONTINUE ||
      m_doc->multiSessionMode() == K3bDataDoc::FINISH ) {
    fetchMultiSessionInfo();
  }
  else if( m_doc->onTheFly() ) {
    fetchIsoSize();
  }
  else {
    writeImage();
  }
}


void K3bDataJob::fetchMultiSessionInfo()
{
  K3bEmptyDiscWaiter waiter( m_doc->burner(), k3bMain() );
  if( waiter.waitForEmptyDisc( true ) == K3bEmptyDiscWaiter::CANCELED ) {
    emit infoMessage( i18n("Writing canceled."), K3bJob::ERROR );
    emit canceled();
    cancelAll();
    return;
  }

  emit infoMessage( i18n("Searching previous session"), K3bJob::PROCESS );

  // check msinfo
  delete m_process;
  m_process = new KProcess();

  if( !k3bMain()->externalBinManager()->foundBin( "cdrecord" ) ) {
    kdDebug() << "(K3bAudioJob) could not find cdrecord executable" << endl;
    emit infoMessage( i18n("cdrecord executable not found."), K3bJob::ERROR );
    cancelAll();
    return;
  }

  *m_process << k3bMain()->externalBinManager()->binPath( "cdrecord" );
  *m_process << "-msinfo";

  // add the device (e.g. /dev/sg1)
  *m_process << QString("dev=%1").arg( m_doc->burner()->busTargetLun() );//genericDevice() );

  connect( m_process, SIGNAL(receivedStderr(KProcess*, char*, int)),
	   this, SLOT(slotCollectOutput(KProcess*, char*, int)) );
  connect( m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
	   this, SLOT(slotCollectOutput(KProcess*, char*, int)) );
  connect( m_process, SIGNAL(processExited(KProcess*)),
	   this, SLOT(slotMsInfoFetched()) );

  m_msInfo = QString::null;
  m_collectedOutput = QString::null;

  if( !m_process->start( KProcess::NotifyOnExit, KProcess::AllOutput ) ) {
    kdDebug() << "(K3bDataJob) could not start cdrecord" << endl;
    emit infoMessage( i18n("Could not start cdrecord!"), K3bJob::ERROR );
    cancelAll();
    return;
  }
}


void K3bDataJob::slotMsInfoFetched()
{
  kdDebug() << "(K3bDataJob) msinfo fetched" << endl;

  // now parse the output
  QStringList list = QStringList::split( ",", m_collectedOutput );
  if( list.count() == 2 ) {
    bool ok1, ok2;
    list.first().toInt( &ok1 );
    list[1].toInt( &ok2 );
    if( ok1 && ok2 )
      m_msInfo = m_collectedOutput.stripWhiteSpace();
    else
      m_msInfo = QString::null;
  }
  else {
    m_msInfo = QString::null;
  }

  kdDebug() << "(K3bDataJob) msinfo parsed: " << m_msInfo << endl;

  if( m_msInfo.isEmpty() ) {
    emit infoMessage( i18n("Could not retrieve multisession information from disk."), K3bJob::ERROR );
    emit infoMessage( i18n("The disk is either empty or not appendable."), K3bJob::ERROR );
    cancelAll();
    return;
  }
  else if( m_doc->onTheFly() ) {
    fetchIsoSize();
  }
  else {
    writeImage();
  }
}


void K3bDataJob::fetchIsoSize()
{
  // determine iso-size
  delete m_process;
  m_process = new KProcess();

  kdDebug() << "(K3bDataJob) process cleared" << endl;

  if( !addMkisofsParameters() )
    return;

  kdDebug() << "(K3bDataJob) mkisofs arguments set" << endl;

  *m_process << "-print-size" << "-quiet";
  // add empty dummy dir since one path-spec is needed
  *m_process << m_doc->dummyDir();

  kdDebug() << "***** mkisofs parameters:\n";
  const QValueList<QCString>& args = m_process->args();
  QString s;
  for( QValueList<QCString>::const_iterator it = args.begin(); it != args.end(); ++it ) {
    s += *it + " ";
  }
  kdDebug() << s << endl << flush;


  connect( m_process, SIGNAL(receivedStderr(KProcess*, char*, int)),
	   this, SLOT(slotCollectOutput(KProcess*, char*, int)) );
  connect( m_process, SIGNAL(receivedStdout(KProcess*, char*, int)),
	   this, SLOT(slotCollectOutput(KProcess*, char*, int)) );
  connect( m_process, SIGNAL(processExited(KProcess*)),
	   this, SLOT(slotIsoSizeFetched()) );

  m_collectedOutput = QString::null;
  m_isoSize = QString::null;

  if( !m_process->start( KProcess::NotifyOnExit, KProcess::AllOutput ) ) {
    kdDebug() << "(K3bDataJob) could not start mkisofs: " << kapp->config()->readEntry( "mkisofs path" ) << endl;
    emit infoMessage( i18n("Could not start mkisofs!"), K3bJob::ERROR );
    cancelAll();
    return;
  }
}


void K3bDataJob::slotIsoSizeFetched()
{
  kdDebug() << "(K3bDataJob) iso size fetched: " << m_collectedOutput << endl;

  // now parse the output
  // this seems to be the format for mkisofs version < 1.14 (to stdout)
  if( m_collectedOutput.contains( "=" ) ) {
    int pos = m_collectedOutput.find('=') + 1;
    int pos2 = m_collectedOutput.find( "\n", pos );
    m_isoSize = m_collectedOutput.mid( pos, (pos2>0?pos2:-1) ).stripWhiteSpace() + "s";
  }
  // and mkisofs >= 1.14 prints out only the number (to stderr)
  // (could be followed or follow other warnings)
  else {
    QStringList lines = QStringList::split( "\n", m_collectedOutput );
    for( QStringList::Iterator str = lines.begin(); str != lines.end(); str++ ) {
      if( QRegExp("\\d*").exactMatch( *str ) ) {
	bool ok;
	int size = (*str).toInt( &ok );
	if( ok ) {
	  m_isoSize = QString("%1s").arg(size);
	  break;
	}
      }
    }
  }

  kdDebug() << "(K3bDataJob) iso size parsed: " << m_isoSize << endl;

  if( m_isoSize.isEmpty() ) {
    emit infoMessage( i18n("Could not retrieve size of data. On-the-fly writing did not work."), K3bJob::ERROR );
    emit infoMessage( i18n("Please create an image first!"), K3bJob::ERROR );
    cancelAll();
  }
  else {
    writeCD();
  }
}


void K3bDataJob::writeCD()
{
  // if we append a new session we asked for an appendable cd already
  if( m_doc->multiSessionMode() == K3bDataDoc::NONE ||
      m_doc->multiSessionMode() == K3bDataDoc::START ) {

    K3bEmptyDiscWaiter waiter( m_doc->burner(), k3bMain() );
    if( waiter.waitForEmptyDisc() == K3bEmptyDiscWaiter::CANCELED ) {
      emit infoMessage( i18n("Writing canceled."), K3bJob::ERROR );
      emit canceled();
      cancelAll();
      return;
    }
  }

  emit newSubTask( i18n("Preparing write process...") );

  // start the writing process -------------------------------------------------------------
  // create a kshellprocess and do it on the fly!

  delete m_process;
  m_process = new KShellProcess();

  if( m_doc->onTheFly() ) {
    if( !addMkisofsParameters() ) {
      cancelAll();
      return;
    }

    // add empty dummy dir since one path-spec is needed
    *m_process << m_doc->dummyDir();
    *m_process << "|";
  }

  // use cdrecord to burn the cd
  if( !k3bMain()->externalBinManager()->foundBin( "cdrecord" ) ) {
    kdDebug() << "(K3bAudioJob) could not find cdrecord executable" << endl;
    emit infoMessage( i18n("cdrecord executable not found."), K3bJob::ERROR );
    cancelAll();
    return;
  }

  *m_process << k3bMain()->externalBinManager()->binPath( "cdrecord" );

  // and now we add the needed arguments...
  // display progress
  *m_process << "-v";

  if( m_doc->multiSessionMode() == K3bDataDoc::CONTINUE ||
      m_doc->multiSessionMode() == K3bDataDoc::FINISH ) {
    if( m_doc->onTheFly() )
      *m_process << "-waiti";  // wait for data on stdin before opening SCSI
  }

  k3bMain()->config()->setGroup( "General Options" );
  bool manualBufferSize = k3bMain()->config()->readBoolEntry( "Manual buffer size", false );
  if( manualBufferSize ) {
    *m_process << QString("fs=%1m").arg( k3bMain()->config()->readNumEntry( "Cdrecord buffer", 4 ) );
  }
  bool overburn = k3bMain()->config()->readBoolEntry( "Allow overburning", false );

  if( m_doc->dummy() )
    *m_process << "-dummy";
  if( overburn )
    *m_process << "-overburn";

  // multisession
  if( m_doc->multiSessionMode() == K3bDataDoc::START ||
      m_doc->multiSessionMode() == K3bDataDoc::CONTINUE )
    *m_process << "-multi";
  else if( m_doc->multiSessionMode() == K3bDataDoc::NONE && m_doc->dao() )
    *m_process << "-dao";


  if( k3bMain()->eject() )
    *m_process << "-eject";
  if( m_doc->burnproof() && m_doc->burner()->burnproof() )
    *m_process << "driveropts=burnfree";  // with cdrecord 1.11a02 burnproof was renamed to burnfree (the new cdrecord writing class should check the version.)

  // add speed
  QString s = QString("-speed=%1").arg( m_doc->speed() );
  *m_process << s;

  // add the device (bus,target,lun)
  s = QString("dev=%1").arg( m_doc->burner()->busTargetLun() ); //genericDevice() );
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
      kdDebug() << "(K3bDataJob) could not start mkisofs/cdrecord" << endl;
      emit infoMessage( i18n("Could not start mkisofs/cdrecord!"), K3bJob::ERROR );
      cancelAll();
    }
  else
    {
      if( m_doc->multiSessionMode() == K3bDataDoc::START )
	emit infoMessage( i18n("Starting multi-session disk."), K3bJob::PROCESS );
      else if( m_doc->multiSessionMode() == K3bDataDoc::CONTINUE )
	emit infoMessage( i18n("Appending session"), K3bJob::PROCESS );
      else if( m_doc->multiSessionMode() == K3bDataDoc::FINISH )
	emit infoMessage( i18n("Finishing multi-session disk"), K3bJob::PROCESS );


      if( m_doc->dummy() )
	emit infoMessage( i18n("Starting simulation at %1x speed...").arg(m_doc->speed()), K3bJob::STATUS );
      else
	emit infoMessage( i18n("Starting recording at %1x speed...").arg(m_doc->speed()), K3bJob::STATUS );

      emit newTask( i18n("Writing data") );
      emit started();
    }
}


void K3bDataJob::writeImage()
{
  // get image file path
  if( m_doc->isoImage().isEmpty() )
    m_doc->setIsoImage( k3bMain()->findTempFile( "iso" ) );

  delete m_process;
  m_process = new KProcess();

  if( !addMkisofsParameters() ) {
    cancelAll();
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
      kdDebug() << "(K3bDataJob) could not start mkisofs" << endl;

      emit infoMessage( i18n("Could not start mkisofs!"), K3bJob::ERROR );
      cancelAll();
    }
  else
    {
      m_imageFinished = false;
      emit infoMessage( i18n("Creating image in %1").arg(m_doc->isoImage()), K3bJob::STATUS );
      emit newSubTask( i18n("Creating image") );
      emit started();
    }
}


void K3bDataJob::cancel()
{
  emit infoMessage( i18n("Writing canceled."), K3bJob::ERROR );
  emit canceled();

  cancelAll();
}


void K3bDataJob::slotParseMkisofsOutput( KProcess*, char* output, int len )
{
  QString buffer = QString::fromLocal8Bit( output, len );


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
	  kdDebug() << "Parsing did not work for " << _perStr << endl;
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
	kdDebug() << "(mkisofs) " << (*str) << endl;
      }
    }
}


void K3bDataJob::slotParseCdrecordOutput( KProcess*, char* output, int len )
{
  QString buffer = QString::fromLocal8Bit( output, len );


  emit debuggingOutput( "cdrecord", buffer );


  // split to lines
  QStringList lines = QStringList::split( "\n", buffer );

  // do every line
  for( QStringList::Iterator str = lines.begin(); str != lines.end(); str++ )
    {
      *str = (*str).stripWhiteSpace();
      if( (*str).startsWith( "Track" ) )
	{
	  //			kdDebug() << "Parsing line [[" << *str << "]]"endl;

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
	  emit newSubTask( i18n("Writing ISO data") );
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
	  kdDebug() << "Parsing did not work for " << _perStr << endl;
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
	kdDebug() << "(cdrecord) " << (*str) << endl;
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
	  emit infoMessage( i18n("mkisofs returned an error. (code %1)").arg(m_process->exitStatus()), K3bJob::ERROR );
	  emit infoMessage( i18n("No error handling yet!"), K3bJob::ERROR );
	  emit infoMessage( i18n("Please send me an email with the last output..."), K3bJob::ERROR );
	  cancelAll();
	  break;
	}
    }
  else
    {
      emit infoMessage( i18n("mkisofs did not exit cleanly."), K3bJob::ERROR );
      cancelAll();
    }

  // remove toc-file
  if( QFile::exists( m_pathSpecFile ) ) {
    //    QFile::remove( m_pathSpecFile );
    m_pathSpecFile = QString::null;
  }
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
	  emit infoMessage( i18n("cdrecord returned an error! (code %1)").arg(m_process->exitStatus()), K3bJob::ERROR );
	  emit infoMessage( i18n("No error handling yet!"), K3bJob::ERROR );
	  emit infoMessage( i18n("Please send me an email with the last output..."), K3bJob::ERROR );
	  emit finished( false );
	  break;
	}
    }
  else
    {
      emit infoMessage( i18n("cdrecord did not exit cleanly."), K3bJob::ERROR );
      emit finished( false );
    }

  // remove path-spec-file
  if( QFile::exists( m_pathSpecFile ) ) {
    //    QFile::remove( m_pathSpecFile );
    m_pathSpecFile = QString::null;
  }

  if( !m_doc->onTheFly() && m_doc->deleteImage() ) {
    QFile::remove( m_doc->isoImage() );
    m_doc->setIsoImage("");
  }
}



bool K3bDataJob::addMkisofsParameters()
{
  if( !k3bMain()->externalBinManager()->foundBin( "mkisofs" ) ) {
    kdDebug() << "(K3bAudioJob) could not find mkisofs executable" << endl;
    emit infoMessage( i18n("Mkisofs executable not found."), K3bJob::ERROR );
    return false;
  }

  *m_process << k3bMain()->externalBinManager()->binPath( "mkisofs" );

  // add multisession info
  if( m_doc->multiSessionMode() == K3bDataDoc::CONTINUE ||
      m_doc->multiSessionMode() == K3bDataDoc::FINISH ) {

    // it has to be the device we are writing to cause only this makes sense
    *m_process << "-M" << m_doc->burner()->busTargetLun();//genericDevice();
    *m_process << "-C" << m_msInfo;
  }

  // add the arguments
  *m_process << "-gui";
  *m_process << "-graft-points";

  if( !m_doc->isoOptions().volumeID().isEmpty() )
    *m_process << "-V" << "\"" + m_doc->isoOptions().volumeID() + "\"";
  if( !m_doc->isoOptions().volumeSetId().isEmpty() )
    *m_process << "-volset" << "\"" + m_doc->isoOptions().volumeSetId() + "\"";
  if( !m_doc->isoOptions().applicationID().isEmpty() )
    *m_process << "-A" << "\"" + m_doc->isoOptions().applicationID() + "\"";
  if( !m_doc->isoOptions().publisher().isEmpty() )
    *m_process << "-P" << "\"" + m_doc->isoOptions().publisher() + "\"";
  if( !m_doc->isoOptions().preparer().isEmpty() )
    *m_process << "-p" << "\"" + m_doc->isoOptions().preparer() + "\"";
  if( !m_doc->isoOptions().systemId().isEmpty() )
    *m_process << "-sysid" << "\"" + m_doc->isoOptions().systemId() + "\"";

  if( m_doc->isoOptions().createRockRidge() ) {
    if( m_doc->isoOptions().preserveFilePermissions() )
      *m_process << "-R";
    else
      *m_process << "-r";
    *m_process << "-hide-list" << m_rrHideFile;
  }

  if( m_doc->isoOptions().createJoliet() ) {
    *m_process << "-J";
    *m_process << "-hide-joliet-list" << m_jolietHideFile;
  }

  if( m_doc->isoOptions().ISOuntranslatedFilenames()  ) {
    *m_process << "-U";
  }
  else {
    if( m_doc->isoOptions().ISOallowPeriodAtBegin()  )
      *m_process << "-L";
    if( m_doc->isoOptions().ISOallow31charFilenames()  )
      *m_process << "-l";
    if( m_doc->isoOptions().ISOomitVersionNumbers() && !m_doc->isoOptions().ISOmaxFilenameLength() )
      *m_process << "-N";
    if( m_doc->isoOptions().ISOrelaxedFilenames()  )
      *m_process << "-relaxed-filenames";
    if( m_doc->isoOptions().ISOallowLowercase()  )
      *m_process << "-allow-lowercase";
    if( m_doc->isoOptions().ISOnoIsoTranslate()  )
      *m_process << "-no-iso-translate";
    if( m_doc->isoOptions().ISOallowMultiDot()  )
      *m_process << "-allow-multidot";
    if( m_doc->isoOptions().ISOomitTrailingPeriod() )
      *m_process << "-d";
  }

  if( m_doc->isoOptions().ISOmaxFilenameLength()  )
    *m_process << "-max-iso9660-filenames";
  if( m_noDeepDirectoryRelocation  )
    *m_process << "-D";


  if( m_doc->isoOptions().followSymbolicLinks() )
    *m_process << "-f";

  if( m_doc->isoOptions().createTRANS_TBL()  )
    *m_process << "-T";
  if( m_doc->isoOptions().hideTRANS_TBL()  )
    *m_process << "-hide-joliet-trans-tbl";

  *m_process << "-iso-level" << QString::number(m_doc->isoOptions().ISOLevel());

  if( m_doc->isoOptions().forceInputCharset() )
    *m_process << "-input-charset" << m_doc->isoOptions().inputCharset();

  *m_process << "-path-list" << QFile::encodeName(m_pathSpecFile);


  // additional parameters from config
  QStringList params = kapp->config()->readListEntry( "mkisofs parameters" );
  for( QStringList::Iterator it = params.begin(); it != params.end(); ++it )
    *m_process << *it;

  return true;
}


void K3bDataJob::slotCollectOutput( KProcess*, char* output, int len )
{
  emit debuggingOutput( "misc", QString::fromLocal8Bit( output, len ) );

  m_collectedOutput += QString::fromLocal8Bit( output, len );
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
//   K3bDataItem* item = m_doc->root()->nextSibling();


//   while( item ) {
//     t << escapeGraftPoint( m_doc->treatWhitespace(item->k3bPath()) )
//       << "=" << escapeGraftPoint( item->localPath() ) << "\n";

//     item = item->nextSibling();
//   }

  writePathSpecForDir( m_doc->root(), t );

  file.close();

  return true;
}


void K3bDataJob::writePathSpecForDir( K3bDirItem* dirItem, QTextStream& stream )
{
  if( dirItem->depth() > 7 ) {
    kdDebug() << "(K3bDataJob) found directory depth > 7. Enabling no deep directory relocation." << endl;
    m_noDeepDirectoryRelocation = true;
  }

  // if joliet is enabled we need to cut long names since mkisofs is not able to do it

  if( m_doc->isoOptions().createJoliet() ) {
    // create new joliet names and use jolietPath for graftpoints
    // sort dirItem->children entries and rename all to fit joliet
    // which is about x characters

    kdDebug() << "(K3bDataJob) creating joliet names for directory: " << dirItem->k3bName() << endl;

    QPtrList<K3bDataItem> sortedChildren;

    // insertion sort
    for( QPtrListIterator<K3bDataItem> it( *dirItem->children() ); it.current(); ++it ) {
      K3bDataItem* item = it.current();

      unsigned int i = 0;
      while( i < sortedChildren.count() && item->k3bName() > sortedChildren.at(i)->k3bName() )
	++i;

      sortedChildren.insert( i, item );
    }

    unsigned int begin = 0;
    unsigned int sameNameCount = 0;
    unsigned int jolietMaxLength = 64;
    while( begin < sortedChildren.count() ) {
      if( sortedChildren.at(begin)->k3bName().length() > jolietMaxLength ) {
	kdDebug() << "(K3bDataJob) filename to long for joliet: "
		  << sortedChildren.at(begin)->k3bName() << endl;
	sameNameCount = 1;

	while( begin + sameNameCount < sortedChildren.count() &&
	       sortedChildren.at( begin + sameNameCount )->k3bName().left(jolietMaxLength)
	       == sortedChildren.at(begin)->k3bName().left(jolietMaxLength) )
	  sameNameCount++;

	kdDebug() << "K3bDataJob) found " << sameNameCount << " files with same joliet name" << endl;

	unsigned int charsForNumber = QString::number(sameNameCount).length();
	for( unsigned int i = begin; i < begin + sameNameCount; i++ ) {
	  // we always reserve 5 chars for the extension
	  QString extension = sortedChildren.at(i)->k3bName().right(5);
	  if( !extension.contains(".") )
	    extension = "";
	  else
	    extension = extension.mid( extension.find(".") );
	  QString jolietName = sortedChildren.at(i)->k3bName().left(jolietMaxLength-charsForNumber-extension.length()-1);
	  jolietName.append( " " );
	  jolietName.append( QString::number( i-begin ).rightJustify( charsForNumber, '0') );
	  jolietName.append( extension );
	  sortedChildren.at(i)->setJolietName( jolietName );

	  kdDebug() << "(K3bDataJob) set joliet name for "
		    << sortedChildren.at(i)->k3bName() << " to "
		    << jolietName << endl;
	}

	begin += sameNameCount;
      }
      else {
	sortedChildren.at(begin)->setJolietName( sortedChildren.at(begin)->k3bName() );
	begin++;
      }
    }

    // now create the graft points
    for( QPtrListIterator<K3bDataItem> it( *dirItem->children() ); it.current(); ++it ) {
      K3bDataItem* item = it.current();
      if( item->isSymLink() ) {
	if( m_doc->isoOptions().discardSymlinks() )
	  continue;
	else if( m_doc->isoOptions().discardBrokenSymlinks() && !item->isValid() )
	  continue;
      }

      stream << escapeGraftPoint( m_doc->treatWhitespace(item->jolietPath()) )
	     << "=" << escapeGraftPoint( item->localPath() ) << "\n";
    }
  }
  else {
    // use k3bPath as normal for graftpoints
    // if rr is enabled all will be cool
    // if neither rr nor joliet are enabled we get very awful names but mkisofs
    // takes care of it
    for( QPtrListIterator<K3bDataItem> it( *dirItem->children() ); it.current(); ++it ) {
      K3bDataItem* item = it.current();
      if( m_doc->isoOptions().discardSymlinks() && item->isSymLink() )
	continue;

      stream << escapeGraftPoint( m_doc->treatWhitespace(item->k3bPath()) )
	     << "=" << escapeGraftPoint( item->localPath() ) << "\n";
    }
  }

  // recursively write graft points for all subdirs
  for( QPtrListIterator<K3bDataItem> it( *dirItem->children() ); it.current(); ++it ) {
    if( K3bDirItem* item = dynamic_cast<K3bDirItem*>(it.current()) )
      writePathSpecForDir( item, stream );
  }
}



bool K3bDataJob::writeRRHideFile( const QString& filename )
{
  QFile file( filename );
  if( !file.open( IO_WriteOnly ) )
    return false;

  QTextStream stream( &file );

  K3bDataItem* item = m_doc->root();
  while( item ) {
    if( item->hideOnRockRidge() ) {
      if( !item->isDir() )  // hiding directories does not work (all dirs point to the dummy-dir)
	stream << escapeGraftPoint( item->localPath() ) << "\n";
//       if( item->isDir() ) {
// 	K3bDirItem* parent = item->parent();
// 	if( parent )
// 	  item = parent->nextChild( item );
// 	else
// 	  item = 0;
//       }
//       else
//	item = item->nextSibling();
    }
    //    else
      item = item->nextSibling();
  }

  file.close();
  return true;
}


bool K3bDataJob::writeJolietHideFile( const QString& filename )
{
  QFile file( filename );
  if( !file.open( IO_WriteOnly ) )
    return false;

  QTextStream stream( &file );

  K3bDataItem* item = m_doc->root();
  while( item ) {
    if( item->hideOnRockRidge() ) {
      if( !item->isDir() )  // hiding directories does not work (all dirs point to the dummy-dir)
	stream << escapeGraftPoint( item->localPath() ) << "\n";
    }
    item = item->nextSibling();
  }

  file.close();
  return true;
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

      if( !m_doc->onlyCreateImage() ) {
	// we need to unlock the writer because cdrdao/cdrecord locked it while writing
	bool block = m_doc->burner()->block( false );
	if( !block )
	  emit infoMessage( i18n("Could not unlock CD drive."), K3bJob::ERROR );
	else if( k3bMain()->eject() )
	  m_doc->burner()->eject();
      }
    }

  // remove path-spec-file
  if( QFile::exists( m_pathSpecFile ) ) {
    //    QFile::remove( m_pathSpecFile );
    m_pathSpecFile = QString::null;
  }

  // remove iso-image if it is unfinished or the user selected to remove image
  if( QFile::exists( m_doc->isoImage() ) ) {
    if( !m_doc->onTheFly() && m_doc->deleteImage() || !m_imageFinished ) {
      emit infoMessage( i18n("Removing ISO image %1").arg(m_doc->isoImage()), K3bJob::STATUS );
      QFile::remove( m_doc->isoImage() );
      m_doc->setIsoImage("");
    }
  }

  emit finished( false );
}


QString K3bDataJob::escapeGraftPoint( const QString& str )
{
  QString newStr( str );

  newStr.replace( QRegExp( "\\\\\\\\" ), "\\\\\\\\\\\\\\\\" );
  newStr.replace( QRegExp( "=" ), "\\=" );

  return newStr;
}


#include "k3bdatajob.moc"
