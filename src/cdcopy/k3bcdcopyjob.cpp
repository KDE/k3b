/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3bcdcopyjob.h"
#include "k3baudiosessionreadingjob.h"

#include <k3bexternalbinmanager.h>
#include <k3bemptydiscwaiter.h>
#include <k3bdevice.h>
#include <k3bdiskinfo.h>
#include <k3btoc.h>
#include <k3bglobals.h>
#include <k3bdevicehandler.h>
#include <k3breadcdreader.h>
#include <k3bdatatrackreader.h>
#include <k3bcdrecordwriter.h>
#include <k3bcdtext.h>
#include <cddb/k3bcddb.h>
#include <cddb/k3bcddbresult.h>
#include <cddb/k3bcddbquery.h>
#include <k3bcore.h>
#include <k3binffilewriter.h>

#include <kconfig.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kdebug.h>
#include <ktempfile.h>
#include <kio/netaccess.h>
#include <kmessagebox.h>

#include <qtimer.h>
#include <qstringlist.h>
#include <qfile.h>
#include <qregexp.h>
#include <qtextstream.h>
#include <qcstring.h>
#include <qfileinfo.h>
#include <qdir.h>
#include <qapplication.h>


class K3bCdCopyJob::Private
{
public:
  Private()
    : canceled(false),
      running(false),
      readcdReader(0),
      dataTrackReader(0),
      audioSessionReader(0),
      cdrecordWriter(0),
      infFileWriter(0),
      cddb(0),
      cdTextFile(0) {
  }

  bool canceled;
  bool error;
  bool readingSuccessful;
  bool running;

  unsigned int numSessions;
  bool doNotCloseLastSession;

  unsigned int doneCopies;
  unsigned int currentReadSession;
  unsigned int currentWrittenSession;

  K3bCdDevice::Toc toc;
  QByteArray cdTextRaw;

  K3bReadcdReader* readcdReader;
  K3bDataTrackReader* dataTrackReader;
  K3bAudioSessionReadingJob* audioSessionReader;
  K3bCdrecordWriter* cdrecordWriter;
  K3bInfFileWriter* infFileWriter;

  bool audioReaderRunning;
  bool dataReaderRunning;
  bool writerRunning;

  // image filenames, one for every track
  QStringList imageNames;

  // inf-filenames for writing audio tracks
  QStringList infNames;

  // indicates if we created a dir or not
  bool deleteTempDir;

  K3bCddb* cddb;
  K3bCddbResultEntry cddbInfo;

  bool haveCddb;
  bool haveCdText;

  KTempFile* cdTextFile;

  QValueVector<bool> dataSessionProbablyTAORecorded;

  // used to determine progress
  QValueVector<long> sessionSizes;
  long overallSize;
};


K3bCdCopyJob::K3bCdCopyJob( QObject* parent )
  : K3bBurnJob( parent ),
    m_simulate(false),
    m_copies(1),
    m_onlyCreateImages(false),
    m_onTheFly(true),
    m_ignoreReadErrors(false),
    m_readRetries(128),
    m_preferCdText(false),
    m_writingMode( K3b::WRITING_MODE_AUTO )
{
  d = new Private();
}


K3bCdCopyJob::~K3bCdCopyJob()
{
  delete d->cdTextFile;
  delete d->infFileWriter;
  delete d;
}


void K3bCdCopyJob::start()
{
  d->running = true;
  d->canceled = false;
  d->error = false;
  d->readingSuccessful = false;
  d->audioReaderRunning = d->dataReaderRunning = d->writerRunning = false;
  d->sessionSizes.clear();
  d->dataSessionProbablyTAORecorded.clear();
  d->deleteTempDir = false;

  emit started();

  emit newTask( i18n("Checking Source Disk") );

  // without this message the window looks quite empty and the user might think nothing is going on
  emit infoMessage( i18n("Checking source disk"), INFO );

  emit burning(false);

  // wait for a source disk
  if( K3bEmptyDiscWaiter::wait( m_readerDevice,
				K3bCdDevice::STATE_COMPLETE|K3bCdDevice::STATE_INCOMPLETE,
				K3bCdDevice::MEDIA_WRITABLE_CD|K3bCdDevice::MEDIA_CD_ROM ) 
      == K3bEmptyDiscWaiter::CANCELED ) {
    finishJob( true, false );
    return;
  }

  emit newSubTask( i18n("Reading Table of Contents") );

  connect( K3bCdDevice::sendCommand( K3bCdDevice::DeviceHandler::DISKINFO_ISRC_MCN, m_readerDevice ), 
	   SIGNAL(finished(K3bCdDevice::DeviceHandler*)),
	   this, 
	   SLOT(slotDiskInfoReady(K3bCdDevice::DeviceHandler*)) );
}


void K3bCdCopyJob::slotDiskInfoReady( K3bCdDevice::DeviceHandler* dh )
{
  if( dh->success() ) {
    d->toc = dh->toc();

    //
    // for now we copy audio, pure data (aka 1 data track), cd-extra (2 session, audio and data),
    // and data multisession wich one track per session.
    // Everything else will be rejected
    //
    bool canCopy = true;
    bool audio = false;
    d->numSessions = dh->ngDiskInfo().numSessions();
    d->doNotCloseLastSession = (dh->ngDiskInfo().diskState() == K3bCdDevice::STATE_INCOMPLETE);
    switch( dh->toc().contentType() ) {
    case K3bCdDevice::DATA:
      // check if every track is in it's own session
      // only then we copy the cd
      if( (int)dh->toc().count() != dh->ngDiskInfo().numSessions() ) {
	emit infoMessage( i18n("K3b does not copy CDs containing multiple data tracks."), ERROR );
	canCopy = false;
      }
      else if( dh->ngDiskInfo().numSessions() > 1 )
	emit infoMessage( i18n("Copying Multisession Data CD."), INFO );
      else
	emit infoMessage( i18n("Copying Data CD."), INFO );
      break;
      
    case K3bCdDevice::MIXED:
      audio = true;
      if( dh->ngDiskInfo().numSessions() != 2 || d->toc[0].type() != K3bCdDevice::Track::AUDIO ) {
	emit infoMessage( i18n("K3b can only copy CD-Extra mixed mode CDs."), ERROR );
	canCopy = false;
      }
      else
	emit infoMessage( i18n("Copying Enhanced Audio CD (CD-Extra)."), INFO );
      break;

    case K3bCdDevice::AUDIO:
      audio = true;
      emit infoMessage( i18n("Copying Audio CD."), INFO );
      break;

    case K3bCdDevice::NONE:
    default:
      emit infoMessage( i18n("The source disk is empty."), ERROR );
      canCopy = false;
      break;
    }

    //
    // A data track recorded in TAO mode has two run-out blocks which cannot be read and contain
    // zero data anyway. The problem is that I do not know of a valid method to determine if a track
    // was written in TAO (the control nibble does definitely not work, I never saw one which did not
    // equal 4).
    // So the solution for now is to simply try to read the last sector of a data track. If this is not
    // possible we assume it was written in TAO mode and reduce the length by 2 sectors
    //
    unsigned char buffer[2048];
    int i = 1;
    for( K3bCdDevice::Toc::iterator it = d->toc.begin(); it != d->toc.end(); ++it ) {
      if( (*it).type() == K3bCdDevice::Track::DATA ) {
	// we try twice just to be sure
	if( m_readerDevice->read10( buffer, 2048, (*it).lastSector().lba(), 1 ) ||
	    m_readerDevice->read10( buffer, 2048, (*it).lastSector().lba(), 1 ) ) {
	  d->dataSessionProbablyTAORecorded.append(false);
	  kdDebug() << "(K3bCdCopyJob) track " << i << " probably DAO recorded." << endl;
	}
	else {
	  d->dataSessionProbablyTAORecorded.append(true);
	  kdDebug() << "(K3bCdCopyJob) track " << i << " probably TAO recorded." << endl;
	}
      }

      ++i;
    }


    //
    // To copy mode2 data tracks we need cdrecord >= 2.01a12 which introduced the -xa1 and -xamix options
    //
    if( k3bcore->externalBinManager()->binObject("cdrecord") && 
	k3bcore->externalBinManager()->binObject("cdrecord")->version < K3bVersion( 2, 1, -1, "a12" ) ) {
      for( K3bCdDevice::Toc::const_iterator it = d->toc.begin(); it != d->toc.end(); ++it ) {
	if( (*it).type() == K3bCdDevice::Track::DATA &&
	    ( (*it).mode() == K3bCdDevice::Track::XA_FORM1 ||
	      (*it).mode() == K3bCdDevice::Track::XA_FORM2 ) ) {
	  emit infoMessage( i18n("K3b needs cdrecord 2.01a12 or newer to copy Mode2 data tracks."), ERROR );
	  finishJob( true, false );
	  return;
	}
      }
    }


    //
    // We already create the temp filenames here since we need them to check the free space
    //
    if( !m_onTheFly || m_onlyCreateImages ) {
      if( !prepareImageFiles() ) {
	finishJob( false, true );
	return;
      }

      //
      // check free temp space
      //
      KIO::filesize_t imageSpaceNeeded = 0;
      for( K3bCdDevice::Toc::const_iterator it = d->toc.begin(); it != d->toc.end(); ++it ) {
	if( (*it).type() == K3bCdDevice::Track::AUDIO )
	  imageSpaceNeeded += (*it).length().audioBytes() + 44;
	else
	  imageSpaceNeeded += (*it).length().mode1Bytes();
      }

      unsigned long avail, size;
      QString pathToTest = m_tempPath.left( m_tempPath.findRev( '/' ) );
      if( !K3b::kbFreeOnFs( pathToTest, size, avail ) ) {
	emit infoMessage( i18n("Unable to determine free space in temporary directory '%1'.").arg(pathToTest), ERROR );
	d->error = true;
	canCopy = false;
      }
      else {
	if( avail < imageSpaceNeeded/1024 ) {
	  emit infoMessage( i18n("Not enough space left in temporary directory."), ERROR );
	  d->error = true;
	  canCopy = false;
	}
      }
    }

    if( canCopy ) {
      d->overallSize = 0;

      // now create some progress helper values
      for( K3bCdDevice::Toc::const_iterator it = d->toc.begin(); it != d->toc.end(); ++it ) {
	d->overallSize += (*it).length().lba();
	if( d->sessionSizes.isEmpty() || (*it).type() == K3bCdDevice::Track::DATA )
	  d->sessionSizes.append( (*it).length().lba() );
	else
	  d->sessionSizes[0] += (*it).length().lba();
      }

      if( audio && !m_onlyCreateImages ) {
	emit newSubTask( i18n("Searching CD-TEXT") );
	
	connect( K3bCdDevice::sendCommand( K3bCdDevice::DeviceHandler::CD_TEXT_RAW, m_readerDevice ),
		 SIGNAL(finished(K3bCdDevice::DeviceHandler*)),
		 this, 
		 SLOT(slotCdTextReady(K3bCdDevice::DeviceHandler*)) );
      }
      else
	startCopy();
    }
    else {
      finishJob( false, true );
    }
  }
  else {
    emit infoMessage( i18n("Unable to read TOC"), ERROR );
    finishJob( false, true );
  }
}


void K3bCdCopyJob::slotCdTextReady( K3bCdDevice::DeviceHandler* dh )
{
  if( dh->success() ) {
    emit infoMessage( i18n("Found CD-TEXT."), SUCCESS );

    d->cdTextRaw = dh->cdTextRaw();
    d->haveCdText = true;

    if( m_preferCdText )
      startCopy();
    else
      queryCddb();  
  }
  else {
    emit infoMessage( i18n("No CD-TEXT found."), INFO );

    d->haveCdText = false;

    queryCddb();
  }
}


void K3bCdCopyJob::queryCddb()
{
  emit newSubTask( i18n("Querying Cddb") );

  d->haveCddb = false;

  if( !d->cddb ) {
    d->cddb = new K3bCddb( this );
    connect( d->cddb, SIGNAL(queryFinished(int)),
	     this, SLOT(slotCddbQueryFinished(int)) );
  }

  KConfig* c = k3bcore->config();
  c->setGroup("Cddb");

  d->cddb->readConfig( c );
  d->cddb->query( d->toc );
}


void K3bCdCopyJob::slotCddbQueryFinished( int error )
{
  if( error == K3bCddbQuery::SUCCESS ) {
    d->cddbInfo = d->cddb->result();
    d->haveCddb = true;

    emit infoMessage( i18n("Found Cddb entry (%1 - %2).").arg(d->cddbInfo.cdArtist).arg(d->cddbInfo.cdTitle), SUCCESS );

    // save the entry locally
    KConfig* c = k3bcore->config();
    c->setGroup( "Cddb" );
    if( c->readBoolEntry( "save cddb entries locally", true ) )
      d->cddb->saveEntry( d->cddbInfo );
  }
  else if( error == K3bCddbQuery::NO_ENTRY_FOUND ) {
    emit infoMessage( i18n("No Cddb entry found."), WARNING );
  }
  else {
    emit infoMessage( i18n("Cddb error (%1).").arg(d->cddb->errorString()), ERROR );
  }

  startCopy();
}


void K3bCdCopyJob::startCopy()
{
  d->currentWrittenSession = d->currentReadSession = 1;
  d->doneCopies = 0;
  
  if( m_onTheFly ) {
    emit newSubTask( i18n("Preparing write process...") );

    if( writeNextSession() )
      readNextSession();
    else {
      finishJob( d->canceled, d->error );
    }
  }
  else
    readNextSession();
}


void K3bCdCopyJob::cancel()
{
  d->canceled = true;

  if( d->writerRunning ) {
    //
    // we will handle cleanup in slotWriterFinished()
    // if we are writing onthefly the reader won't be able to write
    // anymore and will finish unsuccessfully, too
    //
    d->cdrecordWriter->cancel();
  }
  else if( d->audioReaderRunning )
    d->audioSessionReader->cancel();
  else if( d->dataReaderRunning )
    //    d->readcdReader->cancel();
    d->dataTrackReader->cancel();
}


bool K3bCdCopyJob::prepareImageFiles()
{
  kdDebug() << "(K3bCdCopyJob) prepareImageFiles()" << endl;

  d->imageNames.clear();
  d->infNames.clear();
  d->deleteTempDir = false;

  QFileInfo fi( m_tempPath );

  if( d->toc.count() > 1 || d->toc.contentType() == K3bCdDevice::AUDIO ) {
    // create a directory which contains all the images and inf and stuff
    // and save it in some cool structure

    if( !fi.isDir() ) {
      if( QFileInfo( m_tempPath.section( '/', 0, -1 ) ).isDir() )
	m_tempPath = m_tempPath.section( '/', 0, -1 );
      else {
	emit infoMessage( i18n("Specified an unusable temporary path. Using default."), WARNING );
	m_tempPath = K3b::defaultTempPath();
      }
    }

    // create temp dir
    QDir dir( m_tempPath );
    m_tempPath = K3b::findUniqueFilePrefix( "k3bCdCopy", m_tempPath );
    kdDebug() << "(K3bCdCopyJob) creating temp dir: " << m_tempPath << endl;
    if( !dir.mkdir( m_tempPath, true ) ) {
      emit infoMessage( i18n("Unable to create temporary directory '%1'.").arg(m_tempPath), ERROR );
      return false;
    }
    d->deleteTempDir = true;
    m_tempPath = K3b::prepareDir( m_tempPath );

    // create temp filenames
    int i = 1;
    for( K3bCdDevice::Toc::const_iterator it = d->toc.begin(); it != d->toc.end(); ++it ) {
      if( (*it).type() == K3bCdDevice::Track::AUDIO ) {
	d->imageNames.append( m_tempPath + QString("Track%1.wav").arg(QString::number(i).rightJustify(2, '0')) );
	d->infNames.append( m_tempPath + QString("Track%1.inf").arg(QString::number(i).rightJustify(2, '0')) );
      }
      else
	d->imageNames.append( m_tempPath + QString("Track%1.iso").arg(QString::number(i).rightJustify(2, '0')) );
      ++i;
    }

    kdDebug() << "(K3bCdCopyJob) created image filenames:" << endl;
    for( unsigned int i = 0; i < d->imageNames.count(); ++i )
      kdDebug() << "(K3bCdCopyJob) " << d->imageNames[i] << endl;

    return true;
  }
  else {
    // we only need a single image file
    if( !fi.isFile() || 
	KMessageBox::warningYesNo( qApp->activeWindow(),
				   i18n("Do you want to overwrite %1?").arg(m_tempPath),
				   i18n("File Exists") )
	!= KMessageBox::Yes) {
      if( fi.isDir() )
	m_tempPath = K3b::findTempFile( "iso", m_tempPath );
      else if( !QFileInfo( m_tempPath.section( '/', 0, -1 ) ).isDir() ) {
	emit infoMessage( i18n("Specified an unusable temporary path. Using default."), WARNING );
	m_tempPath = K3b::findTempFile( "iso" );
      }
      // else the user specified a file in an existing dir
    }

    d->imageNames.append( m_tempPath );

    return true;
  }
}


void K3bCdCopyJob::readNextSession()
{
  if( !m_onTheFly || m_onlyCreateImages ) {
    if( d->numSessions > 1 )
      emit newTask( i18n("Reading Session %1").arg(d->currentReadSession) );
    else
      emit newTask( i18n("Reading Source Disk") );

    if( d->currentReadSession == 1 )
      emit newSubTask( i18n("Reading track %1 of %2").arg(1).arg(d->toc.count()) );
  }

  // there is only one situation where we need the audiosessionreader:
  // if the first session is an audio session. That means the first track
  // is an audio track
  if( d->currentReadSession == 1 && d->toc[0].type() == K3bCdDevice::Track::AUDIO ) {
    if( !d->audioSessionReader ) {
      d->audioSessionReader = new K3bAudioSessionReadingJob( this );
      connect( d->audioSessionReader, SIGNAL(nextTrack(int, int)), 
	       this, SLOT(slotReadingNextTrack(int, int)) );
      connectSubJob( d->audioSessionReader, 
		     SLOT(slotSessionReaderFinished(bool)),
		     true,
		     SLOT(slotReaderProgress(int)),
		     SLOT(slotReaderSubProgress(int)) );
    }

    d->audioSessionReader->setDevice( m_readerDevice );
    d->audioSessionReader->setToc( d->toc );
    d->audioSessionReader->setParanoiaMode( m_paranoiaMode );
    d->audioSessionReader->setReadRetries( m_readRetries );
    d->audioSessionReader->setNeverSkip( !m_ignoreReadErrors );
    if( m_onTheFly )
      d->audioSessionReader->writeToFd( d->cdrecordWriter->fd() );
    else
      d->audioSessionReader->setImageNames( d->imageNames );  // the audio tracks are always the first tracks

    d->audioReaderRunning = true;
    d->audioSessionReader->start();
  }
  else {
//     if( !d->readcdReader ) {
//       d->readcdReader = new K3bReadcdReader( this );
//       connect( d->readcdReader, SIGNAL(percent(int)), this, SLOT(slotReaderProgress(int)) );
//       connect( d->readcdReader, SIGNAL(processedSize(int, int)), this, SLOT(slotReaderProcessedSize(int, int)) );
//       connect( d->readcdReader, SIGNAL(finished(bool)), this, SLOT(slotSessionReaderFinished(bool)) );
//       connect( d->readcdReader, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
//       connect( d->readcdReader, SIGNAL(debuggingOutput(const QString&, const QString&)), 
// 	       this, SIGNAL(debuggingOutput(const QString&, const QString&)) );
//     }

//     d->readcdReader->setReadDevice( m_readerDevice );
//     d->readcdReader->setAbortOnError( !m_ignoreReadErrors );
//     d->readcdReader->setClone( false );
//     d->readcdReader->setRetries( m_readRetries );

    if( !d->dataTrackReader ) {
      d->dataTrackReader = new K3bDataTrackReader( this );
      connect( d->dataTrackReader, SIGNAL(percent(int)), this, SLOT(slotReaderProgress(int)) );
      connect( d->dataTrackReader, SIGNAL(processedSize(int, int)), this, SLOT(slotReaderProcessedSize(int, int)) );
      connect( d->dataTrackReader, SIGNAL(finished(bool)), this, SLOT(slotSessionReaderFinished(bool)) );
      connect( d->dataTrackReader, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
      connect( d->dataTrackReader, SIGNAL(debuggingOutput(const QString&, const QString&)), 
	       this, SIGNAL(debuggingOutput(const QString&, const QString&)) );
    }

    d->dataTrackReader->setDevice( m_readerDevice );
    d->dataTrackReader->setIgnoreErrors( m_ignoreReadErrors );
    d->dataTrackReader->setRetries( m_readRetries );
    
    K3bTrack* track = 0;
    unsigned int dataTrackIndex = 0;
    if( d->toc.contentType() == K3bCdDevice::MIXED ) {
      track = &d->toc[d->toc.count()-1];
      dataTrackIndex = 0;
    }
    else {
      track = &d->toc[d->currentReadSession-1]; // only one track per session
      dataTrackIndex = d->currentReadSession-1;
    }

    // HACK: if the track is TAO recorded cut the two run-out sectors
    if( d->dataSessionProbablyTAORecorded.count() > dataTrackIndex &&
	d->dataSessionProbablyTAORecorded[dataTrackIndex] )
      //      d->readcdReader->setSectorRange( track->firstSector(), track->lastSector() - 2 );
      d->dataTrackReader->setSectorRange( track->firstSector(), track->lastSector() - 2 );
    else
      //      d->readcdReader->setSectorRange( track->firstSector(), track->lastSector() );
      d->dataTrackReader->setSectorRange( track->firstSector(), track->lastSector() );

    int trackNum = d->currentReadSession;
    if( d->toc.contentType() == K3bCdDevice::MIXED )
      trackNum = d->toc.count();

    if( m_onTheFly )
      //      d->readcdReader->writeToFd( d->cdrecordWriter->fd() );
      d->dataTrackReader->writeToFd( d->cdrecordWriter->fd() );
    else {
      //      d->readcdReader->setImagePath( d->imageNames[trackNum-1] );
      d->dataTrackReader->setImagePath( d->imageNames[trackNum-1] );
    }

    d->dataReaderRunning = true;
    if( !m_onTheFly || m_onlyCreateImages )
      slotReadingNextTrack( 1, 1 );
    //    d->readcdReader->start();
    d->dataTrackReader->start();
  }
}


bool K3bCdCopyJob::writeNextSession()
{
  // we emit our own task since the cdrecord task is way too simple
  if( d->numSessions > 1 ) {
    if( m_simulate )
      emit newTask( i18n("Simulating Session %1").arg(d->currentWrittenSession) );
    else
      emit newTask( i18n("Writing Copy %1 (Session %2)").arg(d->doneCopies+1).arg(d->currentWrittenSession) );
  }
  else {
    if( m_simulate )
      emit newTask( i18n("Simulating") );
    else
      emit newTask( i18n("Writing Copy %1").arg(d->doneCopies+1) );
  }

  emit newSubTask( i18n("Waiting for disk") );

  // if session > 1 we wait for an appendable CD
  if( K3bEmptyDiscWaiter::wait( m_writerDevice, d->currentWrittenSession > 1 ) == K3bEmptyDiscWaiter::CANCELED ) {
    d->canceled = true;
    return false;
  }

  if( !d->cdrecordWriter ) {
    d->cdrecordWriter = new K3bCdrecordWriter( m_writerDevice, this );
    connect( d->cdrecordWriter, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
    connect( d->cdrecordWriter, SIGNAL(percent(int)), this, SLOT(slotWriterProgress(int)) );
    connect( d->cdrecordWriter, SIGNAL(processedSize(int, int)), this, SIGNAL(processedSize(int, int)) );
    connect( d->cdrecordWriter, SIGNAL(subPercent(int)), this, SIGNAL(subPercent(int)) );
    connect( d->cdrecordWriter, SIGNAL(processedSubSize(int, int)), this, SIGNAL(processedSubSize(int, int)) );
    connect( d->cdrecordWriter, SIGNAL(nextTrack(int, int)), this, SLOT(slotWritingNextTrack(int, int)) );
    connect( d->cdrecordWriter, SIGNAL(buffer(int)), this, SIGNAL(bufferStatus(int)) );
    connect( d->cdrecordWriter, SIGNAL(writeSpeed(int, int)), this, SIGNAL(writeSpeed(int, int)) );
    connect( d->cdrecordWriter, SIGNAL(finished(bool)), this, SLOT(slotWriterFinished(bool)) );
    //    connect( d->cdrecordWriter, SIGNAL(newTask(const QString&)), this, SIGNAL(newTask(const QString&)) );
    connect( d->cdrecordWriter, SIGNAL(newSubTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
    connect( d->cdrecordWriter, SIGNAL(debuggingOutput(const QString&, const QString&)), 
	     this, SIGNAL(debuggingOutput(const QString&, const QString&)) );
  }

  d->cdrecordWriter->setBurnDevice( m_writerDevice );
  d->cdrecordWriter->clearArguments();
  d->cdrecordWriter->setSimulate( m_simulate );
  d->cdrecordWriter->setBurnproof( m_burnfree );
  d->cdrecordWriter->setBurnSpeed( m_speed );


  // create the cdrecord arguments
  if( d->currentWrittenSession == 1 && d->toc[0].type() == K3bCdDevice::Track::AUDIO ) {
    //
    // Audio session
    //


    if( !d->infFileWriter )
      d->infFileWriter = new K3bInfFileWriter();

    //
    // create the inf files if not already done
    //
    if( d->infNames.isEmpty() || !QFile::exists( d->infNames[0] ) ) {

      unsigned int trackNumber = 1;

      for( K3bCdDevice::Toc::const_iterator it = d->toc.begin(); it != d->toc.end(); ++it ) {
	const K3bCdDevice::Track& track = *it;

	if( track.type() == K3bCdDevice::Track::DATA )
	  break;

	d->infFileWriter->setTrack( track );
	d->infFileWriter->setTrackNumber( trackNumber );

	if( d->haveCddb ) {
	  d->infFileWriter->setTrackTitle( d->cddbInfo.titles[trackNumber-1] );
	  d->infFileWriter->setTrackPerformer( d->cddbInfo.artists[trackNumber-1] );
	  d->infFileWriter->setTrackMessage( d->cddbInfo.extInfos[trackNumber-1] );

	  d->infFileWriter->setAlbumTitle( d->cddbInfo.cdTitle );
	  d->infFileWriter->setAlbumPerformer( d->cddbInfo.cdArtist );
	}

	if( m_onTheFly ) {

	  d->infFileWriter->setBigEndian( true );

	  // we let KTempFile choose a temp file but delete it on our own
	  // the same way we delete them when writing with images
	  // It is important that the files have the ending inf because
	  // cdrecord only checks this

	  KTempFile tmp( QString::null, ".inf" );
	  d->infNames.append( tmp.name() );
	  bool success = d->infFileWriter->save( *tmp.textStream() );
	  tmp.close();
	  if( !success )
	    return false;
	}
	else {
	  d->infFileWriter->setBigEndian( false );

	  if( !d->infFileWriter->save( d->infNames[trackNumber-1] ) )
	    return false;
	}

	++trackNumber;
      }
    }

    //
    // the inf files are ready and named correctly when writing with images
    //
    int usedWritingMode = m_writingMode;
    if( usedWritingMode == K3b::WRITING_MODE_AUTO ) {
      // there are a lot of writers out there which produce coasters
      // in dao mode if the CD contains pregaps of length 0 (or maybe already != 2 secs?)
      bool zeroPregap = false;
      if( d->numSessions == 1 ) {
	for( K3bCdDevice::Toc::const_iterator it = d->toc.begin(); it != d->toc.end(); ++it ) {
	  const K3bCdDevice::Track& track = *it;
	  if( track.index( 0, false ) == -1 ) {
	    ++it;
	    if( it != d->toc.end() )
	      zeroPregap = true;
	    --it;
	  }
	}
      }

      if( zeroPregap && m_writerDevice->supportsRawWriting() )
	usedWritingMode = K3b::RAW;
      else if( m_writerDevice->dao() )
	usedWritingMode = K3b::DAO;
      else
	usedWritingMode = K3b::TAO;
    }
    d->cdrecordWriter->setWritingMode( usedWritingMode  );

    if( d->numSessions > 1 )
      d->cdrecordWriter->addArgument( "-multi" );

    if( d->haveCddb || d->haveCdText ) {
      // TODO:       if( usedWritingMode == K3b::TAO ) {
      //     emit infoMessage( "no cd-text in TAO mode." )
      //    }
      if( d->haveCdText && ( !d->haveCddb || m_preferCdText ) ) {

	// create a temp file containing the raw cdtext data
	// TODO: move this to K3bCdrecordWriter

	delete d->cdTextFile;
	d->cdTextFile = new KTempFile( QString::null, ".dat" );
	d->cdTextFile->setAutoDelete(true);
	d->cdTextFile->file()->writeBlock( d->cdTextRaw );
	d->cdTextFile->close();

	// use the raw CDTEXT data
	d->cdrecordWriter->addArgument( "textfile=" + d->cdTextFile->name() );
      }
      else {
	// cdrecord will use the cdtext data in the inf files	
	d->cdrecordWriter->addArgument( "-text" );
      }
    }

    d->cdrecordWriter->addArgument( "-useinfo" );

    //
    // add all the audio tracks
    //
    d->cdrecordWriter->addArgument( "-audio" )->addArgument( "-shorttrack" );

    for( unsigned int i = 0; i < d->infNames.count(); ++i ) {
      if( m_onTheFly )
	d->cdrecordWriter->addArgument( d->infNames[i] );
      else
	d->cdrecordWriter->addArgument( d->imageNames[i] );
    }
  }
  else {
    //
    // Data Session
    //
    bool multi = d->doNotCloseLastSession || (d->numSessions > 1 && d->currentWrittenSession < d->toc.count());
    int usedWritingMode = m_writingMode;
    if( usedWritingMode == K3b::WRITING_MODE_AUTO ) {
      // a lot of writers have problems to write pure Data CDs in DAO mode with cdrecord.
      // So we simply always use TAO. I don't like it that much but maybe this will finally
      // get rid of the bug reports...
      usedWritingMode = K3b::TAO;
    }
    d->cdrecordWriter->setWritingMode( usedWritingMode );

    //
    // all but the last session of a multisession disk are written in multi mode
    // and every data track has it's own session which we forced above
    //
    if( multi )
      d->cdrecordWriter->addArgument( "-multi" );

    // just to let the reader init
    if( m_onTheFly )
      d->cdrecordWriter->addArgument( "-waiti" );

    K3bTrack* track = 0;
    unsigned int dataTrackIndex = 0;
    if( d->toc.contentType() == K3bCdDevice::MIXED ) {
      track = &d->toc[d->toc.count()-1];
      dataTrackIndex = 0;
    }
    else {
      track = &d->toc[d->currentWrittenSession-1];
      dataTrackIndex = d->currentWrittenSession-1;
    }

    if( track->mode() == K3bCdDevice::Track::MODE1 )
      d->cdrecordWriter->addArgument( "-data" );
    else if( track->mode() == K3bCdDevice::Track::XA_FORM1 )
      d->cdrecordWriter->addArgument( "-xa1" );
    else
      d->cdrecordWriter->addArgument( "-xamix" );

    if( m_onTheFly ) {
      // HACK: if the track is TAO recorded cut the two run-out sectors
      unsigned long trackLen = track->length().lba();
      if( d->dataSessionProbablyTAORecorded.count() > dataTrackIndex &&
	  d->dataSessionProbablyTAORecorded[dataTrackIndex] )
	trackLen -= 2;

      if( track->mode() == K3bCdDevice::Track::MODE1 )
	trackLen = trackLen * 2048;
      else if( track->mode() == K3bCdDevice::Track::XA_FORM1 )
	trackLen = trackLen * 2056; // see k3bdatatrackreader.h
      else
	trackLen = trackLen * 2332; // see k3bdatatrackreader.h
      d->cdrecordWriter->addArgument( QString("-tsize=%1").arg(trackLen) )->addArgument("-");
    }
    else if( d->toc.contentType() == K3bCdDevice::MIXED )
      d->cdrecordWriter->addArgument( d->imageNames[d->toc.count()-1] );
    else
      d->cdrecordWriter->addArgument( d->imageNames[d->currentWrittenSession-1] );
  }


  //
  // Finally start the writer
  //
  emit burning(true);
  d->writerRunning = true;
  d->cdrecordWriter->start();

  return true;
}


// both the readcdreader and the audiosessionreader are connected to this slot
void K3bCdCopyJob::slotSessionReaderFinished( bool success )
{
  d->audioReaderRunning = d->dataReaderRunning = false;

  if( success ) {
    if( d->numSessions > 1 )
      emit infoMessage( i18n("Successfully read session %1.").arg(d->currentReadSession), SUCCESS );
    else
      emit infoMessage( i18n("Successfully read source disk."), SUCCESS );

    if( !m_onTheFly ) {
      if( d->numSessions > d->currentReadSession ) {
	d->currentReadSession++;
	readNextSession();
      }
      else {
	d->readingSuccessful = true;
	if( !m_onlyCreateImages ) {
	  if( m_readerDevice == m_writerDevice ) {
	    // eject the media
	    m_readerDevice->eject();
	  }
	  
	  if( !writeNextSession() ) {
	    // nothing is running here...
	    finishJob( d->canceled, d->error );
	  }
	}
	else {
	  finishJob( false, false );
	}
      }
    }
  }
  else {
    if( !d->canceled )
      emit infoMessage( i18n("Error while reading session %1.").arg(d->currentReadSession), ERROR );

    finishJob( d->canceled, !d->canceled );
  }
}


void K3bCdCopyJob::slotWriterFinished( bool success )
{
  emit burning(false);

  d->writerRunning = false;

  if( success ) {
    //
    // if this was the last written session we need to reset d->currentWrittenSession
    // and start a new writing if more copies are wanted
    //

    if( d->currentWrittenSession < d->numSessions ) {
      d->currentWrittenSession++;
      d->currentReadSession++;

      // reload the media
      emit newSubTask( i18n("Reloading the media") );
      connect( K3bCdDevice::reload( m_writerDevice ), SIGNAL(finished(K3bCdDevice::DeviceHandler*)),
	       this, SLOT(slotMediaReloadedForNextSession(K3bCdDevice::DeviceHandler*)) );
    }
    else {
      d->doneCopies++;

      if( !m_simulate && d->doneCopies < m_copies ) {
	// start next copy
	K3bCdDevice::eject( m_writerDevice );
	d->currentWrittenSession = 1;
	d->currentReadSession = 1;
	if( writeNextSession() ) {
	  if( m_onTheFly )
	    readNextSession();
	}
	else {
	  // nothing running here...
	  finishJob( d->canceled, d->error );
	}
      }
      else {
	finishJob( false, false );
      }
    }
  }
  else {
    //
    // If we are writing on the fly the reader will also stop when it is not able to write anymore
    // The error handling will be done only here in that case
    //

    // the K3bCdrecordWriter emitted an error message

    finishJob( d->canceled, !d->canceled );
  }
}


void K3bCdCopyJob::slotMediaReloadedForNextSession( K3bCdDevice::DeviceHandler* dh )
{
  if( !dh->success() )
    KMessageBox::information( qApp->activeWindow(), i18n("Please reload the medium and press 'ok'"),
			      i18n("Unable to close the tray") );

  if( !writeNextSession() ) {
    // nothing is running here...
    finishJob( d->canceled, d->error );
  }
  else if( m_onTheFly )
    readNextSession();
}


void K3bCdCopyJob::cleanup()
{
  delete d->cdTextFile;
  d->cdTextFile = 0;

  if( m_onTheFly || !m_keepImage || ((d->canceled || d->error) && !d->readingSuccessful) ) {
    emit infoMessage( i18n("Removing temporary files."), INFO );
    for( QStringList::iterator it = d->infNames.begin(); it != d->infNames.end(); ++it )
      QFile::remove( *it );
  }

  if( !m_onTheFly && (!m_keepImage || ((d->canceled || d->error) && !d->readingSuccessful)) ) {
    emit infoMessage( i18n("Removing image files."), INFO );
    for( QStringList::iterator it = d->imageNames.begin(); it != d->imageNames.end(); ++it )
      QFile::remove( *it );

    // remove the tempdir created in prepareImageFiles()
    if( d->deleteTempDir ) {
      KIO::NetAccess::del( m_tempPath );
      d->deleteTempDir = false;
    }
  }
}


void K3bCdCopyJob::slotReaderProgress( int p )
{
  if( !m_onTheFly || m_onlyCreateImages ) {
    int bigParts = ( m_onlyCreateImages ? 1 : (m_simulate ? 2 : m_copies + 1 ) );
    double done = (double)p * (double)d->sessionSizes[d->currentReadSession-1] / 100.0;
    for( unsigned int i = 0; i < d->currentReadSession-1; ++i )
      done += (double)d->sessionSizes[i];
    emit percent( (int)(100.0*done/(double)d->overallSize/(double)bigParts) );

    if( d->dataReaderRunning )
      emit subPercent(p);
  }
}


void K3bCdCopyJob::slotReaderSubProgress( int p )
{
  // only if reading an audiosession
  if( !m_onTheFly || m_onlyCreateImages ) {
    emit subPercent( p );
  }
}


void K3bCdCopyJob::slotReaderProcessedSize( int p, int pp )
{
  if( !m_onTheFly )
    emit processedSubSize( p, pp );
}


void K3bCdCopyJob::slotWriterProgress( int p )
{
  int bigParts = ( m_simulate ? 1 : m_copies ) + ( m_onTheFly ? 0 : 1 );
  long done = ( m_onTheFly ? d->doneCopies : d->doneCopies+1 ) * d->overallSize
    + (p * d->sessionSizes[d->currentWrittenSession-1] / 100);
  for( unsigned int i = 0; i < d->currentWrittenSession-1; ++i )
    done += d->sessionSizes[i];
  emit percent( 100*done/d->overallSize/bigParts );
}


void K3bCdCopyJob::slotWritingNextTrack( int t, int tt )
{
  if( d->toc.contentType() == K3bCdDevice::MIXED ) {
    if( d->currentWrittenSession == 1 )
      emit newSubTask( i18n("Writing track %1 of %2").arg(t).arg(d->toc.count()) );
    else
      emit newSubTask( i18n("Writing track %1 of %2").arg(d->toc.count()).arg(d->toc.count()) );
  }
  else if( d->numSessions > 1 )
    emit newSubTask( i18n("Writing track %1 of %2").arg(d->currentWrittenSession).arg(d->toc.count()) );
  else
    emit newSubTask( i18n("Writing track %1 of %2").arg(t).arg(tt) );
}


void K3bCdCopyJob::slotReadingNextTrack( int t, int )
{
  if( !m_onTheFly || m_onlyCreateImages ) {
    int track = t;
    if( d->audioReaderRunning )
      track = t;
    else if( d->toc.contentType() == K3bCdDevice::MIXED )
      track = d->toc.count();
    else
      track = d->currentReadSession;

    emit newSubTask( i18n("Reading track %1 of %2").arg(track).arg(d->toc.count()) );
  }
}


QString K3bCdCopyJob::jobDescription() const
{
  if( m_onlyCreateImages ) {
    return i18n("Creating CD Image");
  }
  else if( m_simulate ) {
    if( m_onTheFly )
      return i18n("Simulating CD Copy on-the-fly");
    else
      return i18n("Simulating CD Copy");
  }
  else {
    if( m_onTheFly )
      return i18n("Copying CD on-the-fly");
    else
      return i18n("Copying CD");
  }
}


QString K3bCdCopyJob::jobDetails() const
{
  return i18n("1 copy", "%n copies", m_copies );
}


void K3bCdCopyJob::finishJob( bool c, bool e )
{
  if( d->running ) {
    if( c ) {
      d->canceled = true;
      emit canceled();
    }
    if( e )
      d->error = true;
    
    cleanup();
    
    d->running = false;
    
    emit finished( !(c||e) );
  }
}

#include "k3bcdcopyjob.moc"
