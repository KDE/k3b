/*
 *
 * $Id.cpp,v 1.82 2005/02/04 09:27:19 trueg Exp $
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
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
#include <k3bdevice.h>
#include <k3bdiskinfo.h>
#include <k3btoc.h>
#include <k3bglobals.h>
#include <k3bdevicehandler.h>
#include <k3breadcdreader.h>
#include <k3bdatatrackreader.h>
#include <k3bcdrecordwriter.h>
#include <k3bcdtext.h>
#include <k3bcddb.h>
#include <k3bcddbresult.h>
#include <k3bcddbquery.h>
#include <k3bcore.h>
#include <k3binffilewriter.h>

#include <kconfig.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kdebug.h>
#include <ktempfile.h>
#include <kio/netaccess.h>
#include <kio/job.h>
#include <kio/global.h>

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
      cddb(0) {
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

  K3bDevice::Toc toc;
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

  QValueVector<bool> dataSessionProbablyTAORecorded;

  // used to determine progress
  QValueVector<long> sessionSizes;
  long overallSize;
};


K3bCdCopyJob::K3bCdCopyJob( K3bJobHandler* hdl, QObject* parent )
  : K3bBurnJob( hdl, parent ),
    m_simulate(false),
    m_copies(1),
    m_onlyCreateImages(false),
    m_onTheFly(true),
    m_ignoreDataReadErrors(false),
    m_ignoreAudioReadErrors(true),
    m_noCorrection(false),
    m_dataReadRetries(128),
    m_audioReadRetries(5),
    m_preferCdText(false),
    m_copyCdText(true),
    m_writingMode( K3b::WRITING_MODE_AUTO )
{
  d = new Private();
}


K3bCdCopyJob::~K3bCdCopyJob()
{
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
  d->haveCdText = false;
  d->haveCddb = false;

  jobStarted();

  emit newTask( i18n("Checking Source Medium") );

  emit burning(false);
  emit newSubTask( i18n("Waiting for source medium") );

  // wait for a source disk
  if( waitForMedia( m_readerDevice,
		    K3bDevice::STATE_COMPLETE|K3bDevice::STATE_INCOMPLETE,
		    K3bDevice::MEDIA_WRITABLE_CD|K3bDevice::MEDIA_CD_ROM ) < 0 ) {
    finishJob( true, false );
    return;
  }

  emit newSubTask( i18n("Checking source medium") );

  // FIXME: read ISRCs and MCN

  connect( K3bDevice::diskInfo( m_readerDevice ), SIGNAL(finished(K3bDevice::DeviceHandler*)),
	   this, SLOT(slotDiskInfoReady(K3bDevice::DeviceHandler*)) );
}


void K3bCdCopyJob::slotDiskInfoReady( K3bDevice::DeviceHandler* dh )
{
  if( dh->success() ) {
    d->toc = dh->toc();

    //
    // for now we copy audio, pure data (aka 1 data track), cd-extra (2 session, audio and data),
    // and data multisession which one track per session.
    // Everything else will be rejected
    //
    bool canCopy = true;
    bool audio = false;
    d->numSessions = dh->diskInfo().numSessions();
    d->doNotCloseLastSession = (dh->diskInfo().diskState() == K3bDevice::STATE_INCOMPLETE);
    switch( dh->toc().contentType() ) {
    case K3bDevice::DATA:
      // check if every track is in it's own session
      // only then we copy the cd
      if( (int)dh->toc().count() != dh->diskInfo().numSessions() ) {
	emit infoMessage( i18n("K3b does not copy CDs containing multiple data tracks."), ERROR );
	canCopy = false;
      }
      else if( dh->diskInfo().numSessions() > 1 )
	emit infoMessage( i18n("Copying Multisession Data CD."), INFO );
      else
	emit infoMessage( i18n("Copying Data CD."), INFO );
      break;

    case K3bDevice::MIXED:
      audio = true;
      if( dh->diskInfo().numSessions() != 2 || d->toc[0].type() != K3bDevice::Track::AUDIO ) {
	emit infoMessage( i18n("K3b can only copy CD-Extra mixed mode CDs."), ERROR );
	canCopy = false;
      }
      else
	emit infoMessage( i18n("Copying Enhanced Audio CD (CD-Extra)."), INFO );
      break;

    case K3bDevice::AUDIO:
      audio = true;
      emit infoMessage( i18n("Copying Audio CD."), INFO );
      break;

    case K3bDevice::NONE:
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
    for( K3bDevice::Toc::iterator it = d->toc.begin(); it != d->toc.end(); ++it ) {
      if( (*it).type() == K3bDevice::Track::DATA ) {
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
	!k3bcore->externalBinManager()->binObject("cdrecord")->hasFeature( "xamix" ) ) {
      for( K3bDevice::Toc::const_iterator it = d->toc.begin(); it != d->toc.end(); ++it ) {
	if( (*it).type() == K3bDevice::Track::DATA &&
	    ( (*it).mode() == K3bDevice::Track::XA_FORM1 ||
	      (*it).mode() == K3bDevice::Track::XA_FORM2 ) ) {
	  emit infoMessage( i18n("K3b needs cdrecord 2.01a12 or newer to copy Mode2 data tracks."), ERROR );
	  finishJob( true, false );
	  return;
	}
      }
    }


    //
    // It is not possible to create multisession cds in raw writing mode
    //
    if( d->numSessions > 1 && m_writingMode == K3b::RAW ) {
      if( !questionYesNo( i18n("You will only be able to copy the first session in raw writing mode. "
			       "Continue anyway?"),
			  i18n("Multisession CD") ) ) {
	finishJob( true, false );
	return;
      }
      else {
	emit infoMessage( i18n("Only copying first session."), WARNING );
	// TODO: remove the second session from the progress stuff
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
      for( K3bDevice::Toc::const_iterator it = d->toc.begin(); it != d->toc.end(); ++it ) {
	if( (*it).type() == K3bDevice::Track::AUDIO )
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
      if( K3b::isMounted( m_readerDevice ) ) {
	emit infoMessage( i18n("Unmounting source medium"), INFO );
	K3b::unmount( m_readerDevice );
      }

      d->overallSize = 0;

      // now create some progress helper values
      for( K3bDevice::Toc::const_iterator it = d->toc.begin(); it != d->toc.end(); ++it ) {
	d->overallSize += (*it).length().lba();
	if( d->sessionSizes.isEmpty() || (*it).type() == K3bDevice::Track::DATA )
	  d->sessionSizes.append( (*it).length().lba() );
	else
	  d->sessionSizes[0] += (*it).length().lba();
      }

      if( audio && !m_onlyCreateImages ) {
	if( m_copyCdText )
	  searchCdText();
	else
	  queryCddb();
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


void K3bCdCopyJob::searchCdText()
{
  emit newSubTask( i18n("Searching CD-TEXT") );

  connect( K3bDevice::sendCommand( K3bDevice::DeviceHandler::CD_TEXT_RAW, m_readerDevice ),
	   SIGNAL(finished(K3bDevice::DeviceHandler*)),
	   this,
	   SLOT(slotCdTextReady(K3bDevice::DeviceHandler*)) );
}


void K3bCdCopyJob::slotCdTextReady( K3bDevice::DeviceHandler* dh )
{
  if( dh->success() ) {
    if( K3bDevice::CdText::checkCrc( dh->cdTextRaw() ) ) {
      K3bDevice::CdText cdt( dh->cdTextRaw() );
      emit infoMessage( i18n("Found CD-TEXT (%1 - %2).").arg(cdt.performer()).arg(cdt.title()), SUCCESS );
      d->haveCdText = true;
      d->cdTextRaw = dh->cdTextRaw();
    }
    else {
      emit infoMessage( i18n("Found corrupted CD-TEXT. Ignoring it."), WARNING );
      d->haveCdText = false;
    }

    if( d->haveCdText && m_preferCdText )
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

  if( d->toc.count() > 1 || d->toc.contentType() == K3bDevice::AUDIO ) {
    // create a directory which contains all the images and inf and stuff
    // and save it in some cool structure

    bool tempDirReady = false;
    if( !fi.isDir() ) {
      if( QFileInfo( m_tempPath.section( '/', 0, -2 ) ).isDir() ) {
	if( !QFile::exists( m_tempPath ) ) {
	  QDir dir( m_tempPath.section( '/', 0, -2 ) );
	  dir.mkdir( m_tempPath.section( '/', -1 ) );
	  tempDirReady = true;
	}
	else
	  m_tempPath = m_tempPath.section( '/', 0, -2 );
      }
      else {
	emit infoMessage( i18n("Specified an unusable temporary path. Using default."), WARNING );
	m_tempPath = K3b::defaultTempPath();
      }
    }

    // create temp dir
    if( !tempDirReady ) {
      QDir dir( m_tempPath );
      m_tempPath = K3b::findUniqueFilePrefix( "k3bCdCopy", m_tempPath );
      kdDebug() << "(K3bCdCopyJob) creating temp dir: " << m_tempPath << endl;
      if( !dir.mkdir( m_tempPath, true ) ) {
	emit infoMessage( i18n("Unable to create temporary directory '%1'.").arg(m_tempPath), ERROR );
	return false;
      }
      d->deleteTempDir = true;
    }

    m_tempPath = K3b::prepareDir( m_tempPath );
    emit infoMessage( i18n("Using temporary directory %1.").arg(m_tempPath), INFO );

    // create temp filenames
    int i = 1;
    for( K3bDevice::Toc::const_iterator it = d->toc.begin(); it != d->toc.end(); ++it ) {
      if( (*it).type() == K3bDevice::Track::AUDIO ) {
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
	questionYesNo( i18n("Do you want to overwrite %1?").arg(m_tempPath),
		       i18n("File Exists") ) ) {
      if( fi.isDir() )
	m_tempPath = K3b::findTempFile( "iso", m_tempPath );
      else if( !QFileInfo( m_tempPath.section( '/', 0, -2 ) ).isDir() ) {
	emit infoMessage( i18n("Specified an unusable temporary path. Using default."), WARNING );
	m_tempPath = K3b::findTempFile( "iso" );
      }
      // else the user specified a file in an existing dir

      emit infoMessage( i18n("Writing image file to %1.").arg(m_tempPath), INFO );
    }
    else
      return false;

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
      emit newTask( i18n("Reading Source Medium") );

    if( d->currentReadSession == 1 )
      emit newSubTask( i18n("Reading track %1 of %2").arg(1).arg(d->toc.count()) );
  }

  // there is only one situation where we need the audiosessionreader:
  // if the first session is an audio session. That means the first track
  // is an audio track
  if( d->currentReadSession == 1 && d->toc[0].type() == K3bDevice::Track::AUDIO ) {
    if( !d->audioSessionReader ) {
      d->audioSessionReader = new K3bAudioSessionReadingJob( this, this );
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
    d->audioSessionReader->setReadRetries( m_audioReadRetries );
    d->audioSessionReader->setNeverSkip( !m_ignoreAudioReadErrors );
    if( m_onTheFly )
      d->audioSessionReader->writeToFd( d->cdrecordWriter->fd() );
    else
      d->audioSessionReader->setImageNames( d->imageNames );  // the audio tracks are always the first tracks

    d->audioReaderRunning = true;
    d->audioSessionReader->start();
  }
  else {
    if( !d->dataTrackReader ) {
      d->dataTrackReader = new K3bDataTrackReader( this, this );
      connect( d->dataTrackReader, SIGNAL(percent(int)), this, SLOT(slotReaderProgress(int)) );
      connect( d->dataTrackReader, SIGNAL(processedSize(int, int)), this, SLOT(slotReaderProcessedSize(int, int)) );
      connect( d->dataTrackReader, SIGNAL(finished(bool)), this, SLOT(slotSessionReaderFinished(bool)) );
      connect( d->dataTrackReader, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
      connect( d->dataTrackReader, SIGNAL(debuggingOutput(const QString&, const QString&)),
	       this, SIGNAL(debuggingOutput(const QString&, const QString&)) );
    }

    d->dataTrackReader->setDevice( m_readerDevice );
    d->dataTrackReader->setIgnoreErrors( m_ignoreDataReadErrors );
    d->dataTrackReader->setNoCorrection( m_noCorrection );
    d->dataTrackReader->setRetries( m_dataReadRetries );
    if( m_onlyCreateImages )
      d->dataTrackReader->setSectorSize( K3bDataTrackReader::MODE1 );
    else
      d->dataTrackReader->setSectorSize( K3bDataTrackReader::AUTO );

    K3bTrack* track = 0;
    unsigned int dataTrackIndex = 0;
    if( d->toc.contentType() == K3bDevice::MIXED ) {
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
      d->dataTrackReader->setSectorRange( track->firstSector(), track->lastSector() - 2 );
    else
      d->dataTrackReader->setSectorRange( track->firstSector(), track->lastSector() );

    int trackNum = d->currentReadSession;
    if( d->toc.contentType() == K3bDevice::MIXED )
      trackNum = d->toc.count();

    if( m_onTheFly )
      d->dataTrackReader->writeToFd( d->cdrecordWriter->fd() );
    else
      d->dataTrackReader->setImagePath( d->imageNames[trackNum-1] );

    d->dataReaderRunning = true;
    if( !m_onTheFly || m_onlyCreateImages )
      slotReadingNextTrack( 1, 1 );

    d->dataTrackReader->start();
  }
}


bool K3bCdCopyJob::writeNextSession()
{
  // we emit our own task since the cdrecord task is way too simple
  if( d->numSessions > 1 ) {
    if( m_simulate )
      emit newTask( i18n("Simulating Session %1").arg(d->currentWrittenSession) );
    else if( m_copies > 1 )
      emit newTask( i18n("Writing Copy %1 (Session %2)").arg(d->doneCopies+1).arg(d->currentWrittenSession) );
    else
      emit newTask( i18n("Writing Copy (Session %2)").arg(d->currentWrittenSession) );
  }
  else {
    if( m_simulate )
      emit newTask( i18n("Simulating") );
    else if( m_copies > 1 )
      emit newTask( i18n("Writing Copy %1").arg(d->doneCopies+1) );
    else
      emit newTask( i18n("Writing Copy") );
  }

  emit newSubTask( i18n("Waiting for media") );

  // if session > 1 we wait for an appendable CD
  if( waitForMedia( m_writerDevice,
		    d->currentWrittenSession > 1 && !m_simulate
		    ? K3bDevice::STATE_INCOMPLETE
		    : K3bDevice::STATE_EMPTY,
		    K3bDevice::MEDIA_WRITABLE_CD ) < 0 ) {

    finishJob( true, false );
    return false;
  }

  if( !d->cdrecordWriter ) {
    d->cdrecordWriter = new K3bCdrecordWriter( m_writerDevice, this, this );
    connect( d->cdrecordWriter, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
    connect( d->cdrecordWriter, SIGNAL(percent(int)), this, SLOT(slotWriterProgress(int)) );
    connect( d->cdrecordWriter, SIGNAL(processedSize(int, int)), this, SIGNAL(processedSize(int, int)) );
    connect( d->cdrecordWriter, SIGNAL(subPercent(int)), this, SIGNAL(subPercent(int)) );
    connect( d->cdrecordWriter, SIGNAL(processedSubSize(int, int)), this, SIGNAL(processedSubSize(int, int)) );
    connect( d->cdrecordWriter, SIGNAL(nextTrack(int, int)), this, SLOT(slotWritingNextTrack(int, int)) );
    connect( d->cdrecordWriter, SIGNAL(buffer(int)), this, SIGNAL(bufferStatus(int)) );
    connect( d->cdrecordWriter, SIGNAL(deviceBuffer(int)), this, SIGNAL(deviceBuffer(int)) );
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
  d->cdrecordWriter->setBurnSpeed( m_speed );


  // create the cdrecord arguments
  if( d->currentWrittenSession == 1 && d->toc[0].type() == K3bDevice::Track::AUDIO ) {
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

      for( K3bDevice::Toc::const_iterator it = d->toc.begin(); it != d->toc.end(); ++it ) {
	const K3bDevice::Track& track = *it;

	if( track.type() == K3bDevice::Track::DATA )
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
      //
      // there are a lot of writers out there which produce coasters
      // in dao mode if the CD contains pregaps of length 0 (or maybe already != 2 secs?)
      //
      bool zeroPregap = false;
      if( d->numSessions == 1 ) {
        for( K3bDevice::Toc::const_iterator it = d->toc.begin(); it != d->toc.end(); ++it ) {
          const K3bDevice::Track& track = *it;
          if( track.index0() == 0 ) {
            ++it;
            if( it != d->toc.end() )
              zeroPregap = true;
            --it;
          }
        }
      }

      if( zeroPregap && m_writerDevice->supportsRawWriting() ) {
	if( d->numSessions == 1 )
	  usedWritingMode = K3b::RAW;
	else
	  usedWritingMode = K3b::TAO;
      }
      else if( m_writerDevice->dao() )
	usedWritingMode = K3b::DAO;
      else if( m_writerDevice->supportsRawWriting() )
	usedWritingMode = K3b::RAW;
      else
	usedWritingMode = K3b::TAO;
    }
    d->cdrecordWriter->setWritingMode( usedWritingMode  );

    if( d->numSessions > 1 )
      d->cdrecordWriter->addArgument( "-multi" );

    if( d->haveCddb || d->haveCdText ) {
      if( usedWritingMode == K3b::TAO ) {
	emit infoMessage( i18n("It is not possible to write CD-Text in TAO mode."), WARNING );
      }
      else if( d->haveCdText && ( !d->haveCddb || m_preferCdText ) ) {
	// use the raw CDTEXT data
	d->cdrecordWriter->setRawCdText( d->cdTextRaw );
      }
      else {
	// make sure the writer job does not create raw cdtext
	d->cdrecordWriter->setRawCdText( QByteArray() );
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
    K3bTrack* track = 0;
    unsigned int dataTrackIndex = 0;
    if( d->toc.contentType() == K3bDevice::MIXED ) {
      track = &d->toc[d->toc.count()-1];
      dataTrackIndex = 0;
    }
    else {
      track = &d->toc[d->currentWrittenSession-1];
      dataTrackIndex = d->currentWrittenSession-1;
    }

    bool multi = d->doNotCloseLastSession || (d->numSessions > 1 && d->currentWrittenSession < d->toc.count());
    int usedWritingMode = m_writingMode;
    if( usedWritingMode == K3b::WRITING_MODE_AUTO ) {
      // at least the NEC3540a does write 2056 byte sectors only in tao mode. Same for LG4040b
      // since writing data tracks in TAO mode is no loss let's default to TAO in the case of 2056 byte
      // sectors (which is when writing xa form1 sectors here)
      if( m_writerDevice->dao() &&
	  d->toc.count() == 1 &&
	  !multi &&
	  track->mode() == K3bDevice::Track::MODE1 )
 	usedWritingMode = K3b::DAO;
      else
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

    if( track->mode() == K3bDevice::Track::MODE1 )
      d->cdrecordWriter->addArgument( "-data" );
    else if( track->mode() == K3bDevice::Track::XA_FORM1 )
      d->cdrecordWriter->addArgument( "-xa1" );
    else
      d->cdrecordWriter->addArgument( "-xamix" );

    if( m_onTheFly ) {
      // HACK: if the track is TAO recorded cut the two run-out sectors
      unsigned long trackLen = track->length().lba();
      if( d->dataSessionProbablyTAORecorded.count() > dataTrackIndex &&
	  d->dataSessionProbablyTAORecorded[dataTrackIndex] )
	trackLen -= 2;

      if( track->mode() == K3bDevice::Track::MODE1 )
	trackLen = trackLen * 2048;
      else if( track->mode() == K3bDevice::Track::XA_FORM1 )
	trackLen = trackLen * 2056; // see k3bdatatrackreader.h
      else
	trackLen = trackLen * 2332; // see k3bdatatrackreader.h
      d->cdrecordWriter->addArgument( QString("-tsize=%1").arg(trackLen) )->addArgument("-");
    }
    else if( d->toc.contentType() == K3bDevice::MIXED )
      d->cdrecordWriter->addArgument( d->imageNames[d->toc.count()-1] );
    else
      d->cdrecordWriter->addArgument( d->imageNames[d->currentWrittenSession-1] );

    // clear cd text from previous sessions
    d->cdrecordWriter->setRawCdText( QByteArray() );
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
	    // eject the media (we do this blocking to know if it worked
	    // becasue if it did not it might happen that k3b overwrites a CD-RW
	    // source)
	    if( !m_readerDevice->eject() ) {
	      blockingInformation( i18n("K3b was unable to eject the source disk. Please do so manually.") );
	    }
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
    if( !d->canceled ) {
      emit infoMessage( i18n("Error while reading session %1.").arg(d->currentReadSession), ERROR );
      if( m_onTheFly )
	d->cdrecordWriter->setSourceUnreadable(true);
    }

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
      emit newSubTask( i18n("Reloading the medium") );
      connect( K3bDevice::reload( m_writerDevice ), SIGNAL(finished(K3bDevice::DeviceHandler*)),
	       this, SLOT(slotMediaReloadedForNextSession(K3bDevice::DeviceHandler*)) );
    }
    else {
      d->doneCopies++;

      if( !m_simulate && d->doneCopies < m_copies ) {
	// start next copy
	K3bDevice::eject( m_writerDevice );

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


void K3bCdCopyJob::slotMediaReloadedForNextSession( K3bDevice::DeviceHandler* dh )
{
  if( !dh->success() )
    blockingInformation( i18n("Please reload the medium and press 'ok'"),
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
      KIO::NetAccess::del( KURL::fromPathOrURL(m_tempPath), 0 );
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
  if( d->toc.contentType() == K3bDevice::MIXED ) {
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
    else if( d->toc.contentType() == K3bDevice::MIXED )
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
      return i18n("Simulating CD Copy On-The-Fly");
    else
      return i18n("Simulating CD Copy");
  }
  else {
    if( m_onTheFly )
      return i18n("Copying CD On-The-Fly");
    else
      return i18n("Copying CD");
  }
}


QString K3bCdCopyJob::jobDetails() const
{
  return i18n("Creating 1 copy",
	      "Creating %n copies",
	      (m_simulate||m_onlyCreateImages) ? 1 : m_copies );
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

    jobFinished( !(c||e) );
  }
}

#include "k3bcdcopyjob.moc"
