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




#include "k3bmixedjob.h"
#include "k3bmixeddoc.h"

#include <k3bdatadoc.h>
#include <k3bisoimager.h>
#include <k3bmsinfofetcher.h>
#include <k3baudiostreamer.h>
#include <k3baudiodoc.h>
#include <k3baudiotrack.h>
#include <k3baudionormalizejob.h>
#include <k3baudiojobtempdata.h>
#include <k3bdevicemanager.h>
#include <k3bdevice.h>
#include <k3bdevicehandler.h>
#include <k3bmsf.h>
#include <k3bwavefilewriter.h>
#include <k3bglobals.h>
#include <k3bexternalbinmanager.h>
#include <k3bversion.h>
#include <k3bemptydiscwaiter.h>
#include <k3bcore.h>
#include <k3bcdrecordwriter.h>
#include <k3bcdrdaowriter.h>

#include <qfile.h>
#include <qdatastream.h>
#include <qapplication.h>

#include <kdebug.h>
#include <klocale.h>
#include <ktempfile.h>
#include <kio/netaccess.h>
#include <kio/global.h>
#include <kmessagebox.h>


K3bMixedJob::K3bMixedJob( K3bMixedDoc* doc, QObject* parent )
  : K3bBurnJob( parent ),
    m_doc( doc ),
    m_normalizeJob(0)
{
  m_isoImager = new K3bIsoImager( doc->dataDoc(), this );
  connect( m_isoImager, SIGNAL(sizeCalculated(int, int)), this, SLOT(slotSizeCalculationFinished(int, int)) );
  connect( m_isoImager, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
  connect( m_isoImager, SIGNAL(percent(int)), this, SLOT(slotIsoImagerPercent(int)) );
  connect( m_isoImager, SIGNAL(finished(bool)), this, SLOT(slotIsoImagerFinished(bool)) );
  connect( m_isoImager, SIGNAL(debuggingOutput(const QString&, const QString&)),
	   this, SIGNAL(debuggingOutput(const QString&, const QString&)) );

  m_audioDecoder = new K3bAudioStreamer( doc->audioDoc(), this );
  connect( m_audioDecoder, SIGNAL(data(const char*, int)), this, SLOT(slotReceivedAudioDecoderData(const char*, int)) );
  connect( m_audioDecoder, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
  connect( m_audioDecoder, SIGNAL(percent(int)), this, SLOT(slotAudioDecoderPercent(int)) );
  connect( m_audioDecoder, SIGNAL(subPercent(int)), this, SLOT(slotAudioDecoderSubPercent(int)) );
  connect( m_audioDecoder, SIGNAL(finished(bool)), this, SLOT(slotAudioDecoderFinished(bool)) );
  connect( m_audioDecoder, SIGNAL(nextTrack(int, int)), this, SLOT(slotAudioDecoderNextTrack(int, int)) );

  m_msInfoFetcher = new K3bMsInfoFetcher( this );
  connect( m_msInfoFetcher, SIGNAL(finished(bool)), this, SLOT(slotMsInfoFetched(bool)) );
  connect( m_msInfoFetcher, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );

  m_waveFileWriter = new K3bWaveFileWriter();

  m_writer = 0;
  m_tocFile = 0;
  m_tempData = new K3bAudioJobTempData( m_doc->audioDoc(), this );
}


K3bMixedJob::~K3bMixedJob()
{
  delete m_waveFileWriter;
  delete m_tocFile;
}


K3bDevice* K3bMixedJob::writer() const
{
  return m_doc->burner();
}


K3bDoc* K3bMixedJob::doc() const
{
  return m_doc;
}


void K3bMixedJob::start()
{
  emit started();

  m_canceled = false;
  m_errorOccuredAndAlreadyReported = false;

  prepareProgressInformation();

  // set some flags that are needed
  m_doc->audioDoc()->setOnTheFly( m_doc->onTheFly() );  // for the toc writer
  m_doc->audioDoc()->setHideFirstTrack( false );   // unsupported
  m_doc->dataDoc()->setBurner( m_doc->burner() );  // so the isoImager can read ms data


  determineWritingMode();


  // depending on the mixed type and if it's onthefly or not we
  // decide what to do first

  // if not onthefly create the iso image and then the wavs
  // and write then
  // if onthefly calculate the iso size
  if( m_doc->onTheFly() ) {
    emit newTask( i18n("Preparing write process") );

    if( m_doc->mixedType() != K3bMixedDoc::DATA_SECOND_SESSION ) {
      m_isoImager->calculateSize();
    }
    else {
      // we cannot calculate the size since we don't have the msinfo yet
      // so first write the audio session
      m_currentAction = WRITING_AUDIO_IMAGE;
      if( !prepareWriter() ) {
	// no cleanup necessary since nothing has been started yet
	emit finished(false);
	return;
      }

      if( startWriting() ) {
	m_audioDecoder->start();
      }
    }
  }
  else {
    emit burning(false);

    m_tempFilePrefix = K3b::findUniqueFilePrefix( ( !m_doc->audioDoc()->title().isEmpty()
						    ? m_doc->audioDoc()->title()
						    : m_doc->dataDoc()->isoOptions().volumeID() ),
						  m_doc->tempDir() );

    if( m_doc->mixedType() != K3bMixedDoc::DATA_SECOND_SESSION ) {
      createIsoImage();
    }
    else {
      emit newTask( i18n("Creating audio image files") );
      m_currentAction = CREATING_AUDIO_IMAGE;
      m_audioDecoder->start();
    }
  }
}


void K3bMixedJob::cancel()
{
  m_canceled = true;

  if( m_writer )
    m_writer->cancel();
  m_isoImager->cancel();
  m_audioDecoder->cancel();
  m_msInfoFetcher->cancel();
  emit infoMessage( i18n("Writing canceled."), K3bJob::ERROR );
  removeBufferFiles();
  emit canceled();
  emit finished(false);
}


void K3bMixedJob::slotMsInfoFetched( bool success )
{
  if( m_canceled || m_errorOccuredAndAlreadyReported )
    return;

  if( success ) {
    if( m_usedDataWritingApp == K3b::CDRECORD )
      m_isoImager->setMultiSessionInfo( m_msInfoFetcher->msInfo() );
    else  // cdrdao seems to write a 150 blocks pregap that is not used by cdrecord
      m_isoImager->setMultiSessionInfo( QString("%1,%2").arg(m_msInfoFetcher->lastSessionStart()).arg(m_msInfoFetcher->nextSessionStart()+150) );

    if( m_doc->onTheFly() ) {
      m_isoImager->calculateSize();
    }
    else {
      createIsoImage();
    }
  }
  else {
    // the MsInfoFetcher already emitted failure info
    cleanupAfterError();
    emit finished(false);
  }
}


void K3bMixedJob::slotSizeCalculationFinished( int status, int size )
{
  emit infoMessage( i18n("Size calculated:") + i18n("%1 (1 Byte)", "%1 (%n bytes)", size*2048).arg(size), INFO );
  if( status != ERROR ) {

    // 1. data in first track:
    //    start isoimager and writer
    //    when isoimager finishes start audiodecoder

    // 2. data in last track
    //    start audiodecoder and writer
    //    when audiodecoder finishes start isoimager

    // 3. data in second session
    //    start audiodecoder and writer
    //    start isoimager and writer


    // the prepareWriter method needs the action to be set
    if( m_doc->mixedType() == K3bMixedDoc::DATA_LAST_TRACK ) {
      m_currentAction = WRITING_AUDIO_IMAGE;
    }
    else {
      m_currentAction = WRITING_ISO_IMAGE;
    }

    if( !prepareWriter() ) {
      // no cleanup necessary since nothing has been started yet
      emit finished(false);
      return;
    }

    if( !startWriting() )
      return;

    if( m_doc->mixedType() == K3bMixedDoc::DATA_LAST_TRACK ) {
      m_audioDecoder->start();
    }
    else {
      m_isoImager->start();
    }
  }
  else {
    // no cleanup necessary since nothing has been started yet
    emit finished(false);
  }
}


void K3bMixedJob::slotIsoImagerFinished( bool success )
{
  if( m_canceled || m_errorOccuredAndAlreadyReported )
    return;

  if( !success ) {
    emit infoMessage( i18n("Error while creating iso image."), ERROR );
    cleanupAfterError();

    emit finished( false );
    return;
  }

  if( m_doc->onTheFly() ) {
    if( m_doc->mixedType() == K3bMixedDoc::DATA_FIRST_TRACK ) {
      m_currentAction = WRITING_AUDIO_IMAGE;
      m_audioDecoder->start();
    }
  }
  else {
    emit infoMessage( i18n("Iso image successfully created."), SUCCESS );

    if( m_doc->mixedType() == K3bMixedDoc::DATA_SECOND_SESSION ) {
      m_currentAction = WRITING_ISO_IMAGE;

      if( !prepareWriter() ) {
	cleanupAfterError();
	emit finished(false);
	return;
      }

      startWriting();
    }
    else {
      emit newTask( i18n("Creating audio image files") );
      m_currentAction = CREATING_AUDIO_IMAGE;
      m_audioDecoder->start();
    }
  }
}


void K3bMixedJob::slotWriterFinished( bool success )
{
  if( m_canceled || m_errorOccuredAndAlreadyReported )
    return;

  if( !success ) {
    cleanupAfterError();
    emit finished(false);
    return;
  }

  emit burning(false);

  if( m_doc->mixedType() == K3bMixedDoc::DATA_SECOND_SESSION && m_currentAction == WRITING_AUDIO_IMAGE ) {
    // reload the media (as a subtask so the user does not see the "Flushing cache" or "Fixating" messages while
    // doing so
    emit newSubTask( i18n("Reloading the media") );
    connect( K3bCdDevice::reload( m_doc->burner() ), SIGNAL(finished(bool)),
	     this, SLOT(slotMediaReloadedForSecondSession(bool)) );
  }
  else {
    if( !m_doc->onTheFly() && m_doc->removeImages() )
      removeBufferFiles();

    emit finished(true);
  }
}


void K3bMixedJob::slotMediaReloadedForSecondSession( bool success )
{
  if( !success )
    KMessageBox::information( 0, i18n("Please reload the medium and press 'ok'"),
			      i18n("Unable to close the tray") );

  // start the next session
  if( m_doc->dummy() ) {
    // do not try to get ms info in simulation mode since the cd is empty!
    if( m_doc->onTheFly() ) {
      m_isoImager->calculateSize();
    }
    else {
      createIsoImage();
    }
  }
  else {
    m_msInfoFetcher->setDevice( m_doc->burner() );
    m_msInfoFetcher->start();
  }
}


void K3bMixedJob::slotAudioDecoderFinished( bool success )
{
  if( m_canceled || m_errorOccuredAndAlreadyReported )
    return;

  if( !success ) {
    emit infoMessage( i18n("Error while decoding audio tracks."), ERROR );
    cleanupAfterError();
    emit finished(false);
    return;
  }

  if( m_doc->onTheFly() ) {
    if( m_doc->mixedType() == K3bMixedDoc::DATA_LAST_TRACK ) {
      m_currentAction = WRITING_ISO_IMAGE;
      m_isoImager->start();
    }
  }
  else {
    m_waveFileWriter->close();

    emit infoMessage( i18n("Audio images successfully created."), SUCCESS );

    if( m_doc->audioDoc()->normalize() ) {
      normalizeFiles();
    }
    else {
      if( m_doc->mixedType() == K3bMixedDoc::DATA_FIRST_TRACK )
	m_currentAction = WRITING_ISO_IMAGE;
      else
	m_currentAction = WRITING_AUDIO_IMAGE;

      if( !prepareWriter() ) {
	cleanupAfterError();
	emit finished(false);
	return;
      }
      else
	startWriting();
    }
  }
}


void K3bMixedJob::slotReceivedAudioDecoderData( const char* data, int len )
{
  m_waveFileWriter->write( data, len );
  m_audioDecoder->resume();
}


void K3bMixedJob::slotAudioDecoderNextTrack( int t, int tt )
{
  if( !m_doc->onTheFly() ) {
    K3bAudioTrack* track = m_doc->audioDoc()->at(t-1);
    emit newSubTask( i18n("Decoding audio track %1 of %2 (%3)").arg(t).arg(tt).arg(track->filename()) );

    if( !m_waveFileWriter->open( m_tempData->bufferFileName(track) ) ) {
      emit infoMessage( i18n("Could not open file %1 for writing.").arg(m_waveFileWriter->filename()), ERROR );
      cleanupAfterError();
      emit finished( false );
      return;
    }
  }
}


bool K3bMixedJob::prepareWriter()
{
  if( m_writer ) delete m_writer;

  if( ( m_currentAction == WRITING_ISO_IMAGE && m_usedDataWritingApp == K3b::CDRECORD ) ||
      ( m_currentAction == WRITING_AUDIO_IMAGE && m_usedAudioWritingApp == K3b::CDRECORD ) )  {

    if( !m_tempData->writeInfFiles() ) {
      kdDebug() << "(K3bMixedJob) could not write inf-files." << endl;
      emit infoMessage( i18n("IO Error"), ERROR );

      return false;
    }

    K3bCdrecordWriter* writer = new K3bCdrecordWriter( m_doc->burner(), this );

    // only write the audio tracks in DAO mode
    if( m_currentAction == WRITING_ISO_IMAGE )
      writer->setWritingMode( m_usedDataWritingMode );
    else
      writer->setWritingMode( m_usedAudioWritingMode );

    writer->setSimulate( m_doc->dummy() );
    writer->setBurnproof( m_doc->burnproof() );
    writer->setBurnSpeed( m_doc->speed() );

    if( m_doc->mixedType() == K3bMixedDoc::DATA_SECOND_SESSION ) {
      if( m_currentAction == WRITING_ISO_IMAGE ) {
	if( m_doc->onTheFly() )
	  writer->addArgument("-waiti");

	addDataTrack( writer );
      }
      else {
	writer->addArgument("-multi");
	addAudioTracks( writer );
      }
    }
    else {
      if( m_doc->mixedType() == K3bMixedDoc::DATA_FIRST_TRACK )
	addDataTrack( writer );
      addAudioTracks( writer );
      if( m_doc->mixedType() == K3bMixedDoc::DATA_LAST_TRACK )
	addDataTrack( writer );
    }

    m_writer = writer;
  }
  else {  // DEFAULT
    if( !writeTocFile() ) {
      kdDebug() << "(K3bDataJob) could not write tocfile." << endl;
      emit infoMessage( i18n("IO Error"), ERROR );

      return false;
    }

    // create the writer
    // create cdrdao job
    K3bCdrdaoWriter* writer = new K3bCdrdaoWriter( m_doc->burner(), this );
    writer->setSimulate( m_doc->dummy() );
    writer->setBurnSpeed( m_doc->speed() );

    // multisession only for the first session
    writer->setMulti( m_doc->mixedType() == K3bMixedDoc::DATA_SECOND_SESSION
		      && m_currentAction == WRITING_AUDIO_IMAGE );

    writer->setTocFile( m_tocFile->name() );

    m_writer = writer;
  }

  connect( m_writer, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
  connect( m_writer, SIGNAL(percent(int)), this, SLOT(slotWriterJobPercent(int)) );
  connect( m_writer, SIGNAL(processedSize(int, int)), this, SIGNAL(processedSize(int, int)) );
  connect( m_writer, SIGNAL(subPercent(int)), this, SIGNAL(subPercent(int)) );
  connect( m_writer, SIGNAL(processedSubSize(int, int)), this, SIGNAL(processedSubSize(int, int)) );
  connect( m_writer, SIGNAL(nextTrack(int, int)), this, SLOT(slotWriterNextTrack(int, int)) );
  connect( m_writer, SIGNAL(buffer(int)), this, SIGNAL(bufferStatus(int)) );
  connect( m_writer, SIGNAL(writeSpeed(int, int)), this, SIGNAL(writeSpeed(int, int)) );
  connect( m_writer, SIGNAL(finished(bool)), this, SLOT(slotWriterFinished(bool)) );
  //  connect( m_writer, SIGNAL(newTask(const QString&)), this, SIGNAL(newTask(const QString&)) );
  connect( m_writer, SIGNAL(newSubTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
  connect( m_writer, SIGNAL(debuggingOutput(const QString&, const QString&)),
	   this, SIGNAL(debuggingOutput(const QString&, const QString&)) );

  return true;
}


bool K3bMixedJob::writeTocFile()
{
  if( m_tocFile ) delete m_tocFile;
  m_tocFile = new KTempFile( QString::null, "toc" );
  m_tocFile->setAutoDelete(true);

  // write the toc-file
  if( QTextStream* s = m_tocFile->textStream() ) {

    // TocType
    if( m_usedDataMode == K3b::MODE2 ) {
      *s << "CD_ROM_XA" << endl;
    }
    else {
      *s << "CD_ROM" << endl;
    }
    *s << endl;

    // we do not need a cd-text header when writing the second session
    // which only contains the iso image
    if( !(m_doc->mixedType() == K3bMixedDoc::DATA_SECOND_SESSION &&
	  m_currentAction == WRITING_ISO_IMAGE ) )
      m_tempData->writeAudioTocCdTextHeader( *s );

    if( ( m_doc->mixedType() == K3bMixedDoc::DATA_SECOND_SESSION &&
	  m_currentAction == WRITING_AUDIO_IMAGE ) ||
	m_doc->mixedType() == K3bMixedDoc::DATA_LAST_TRACK )
      m_tempData->writeAudioTocFilePart( *s );

    if( m_doc->mixedType() != K3bMixedDoc::DATA_SECOND_SESSION ||
	m_currentAction == WRITING_ISO_IMAGE ) {

      if( m_usedDataMode == K3b::MODE2 )
	*s << "TRACK MODE2_FORM1" << endl;
      else
	*s << "TRACK MODE1" << endl;

      if( m_doc->mixedType() != K3bMixedDoc::DATA_SECOND_SESSION &&
	  m_doc->audioDoc()->cdText() ) {
	// insert fake cdtext
	// cdrdao does not work without it and it seems not to do any harm.
	*s << "CD_TEXT {" << endl
	   << "  LANGUAGE 0 {" << endl
	   << "    TITLE " << "\"\"" << endl
	   << "    PERFORMER " << "\"\"" << endl
	   << "    ISRC " << "\"\"" << endl
	   << "    ARRANGER " << "\"\"" << endl
	   << "    SONGWRITER " << "\"\"" << endl
	   << "    COMPOSER " << "\"\"" << endl
	   << "    MESSAGE " << "\"\"" << endl
	   << "  }" << endl
	   << "}" << endl;
      }

      if( m_doc->onTheFly() )
	*s << "DATAFILE \"-\" " << m_isoImager->size()*2048 << endl;
      else
	*s << "DATAFILE \"" << m_isoImageFilePath << "\"" << endl;
      *s << endl;
    }

    if( m_doc->mixedType() == K3bMixedDoc::DATA_FIRST_TRACK )
      m_tempData->writeAudioTocFilePart( *s, m_doc->onTheFly() ? K3b::Msf(m_isoImager->size()) : K3b::Msf() );

    m_tocFile->close();

    // backup for debugging
//     KIO::NetAccess::del("/home/trueg/tmp/tocfile_debug_backup.toc");
//     KIO::NetAccess::copy( m_tocFile->name(), "/home/trueg/tmp/tocfile_debug_backup.toc" );

    return true;
  }
  else
    return false;
}


void K3bMixedJob::addAudioTracks( K3bCdrecordWriter* writer )
{
  writer->addArgument( "-useinfo" );
  if( m_doc->audioDoc()->cdText() )
    writer->addArgument( "-text" );
  writer->addArgument( "-audio" );

  // we always pad because although K3b makes sure all tracks' lenght are multible of 2352
  // it seems that normalize sometimes corrupts these lengths
  writer->addArgument( "-pad" );

  // Allow tracks shorter than 4 seconds
  writer->addArgument( "-shorttrack" );

  // add all the audio tracks
  QPtrListIterator<K3bAudioTrack> it( *m_doc->audioDoc()->tracks() );
  for( ; it.current(); ++it ) {
    K3bAudioTrack* track = it.current();

    if( m_doc->onTheFly() ) {
      // this is only supported by cdrecord versions >= 2.01a13
      writer->addArgument( QFile::encodeName( m_tempData->infFileName( track ) ) );
    }
    else {
      writer->addArgument( QFile::encodeName( m_tempData->bufferFileName( track ) ) );
    }
  }
}

void K3bMixedJob::addDataTrack( K3bCdrecordWriter* writer )
{
  // add data track
  if(  m_usedDataMode == K3b::MODE2 ) {
    if( k3bcore->externalBinManager()->binObject("cdrecord") && 
	k3bcore->externalBinManager()->binObject("cdrecord")->hasFeature( "xamix" ) )
      writer->addArgument( "-xa" );
    else
      writer->addArgument( "-xa1" );
  }
  else
    writer->addArgument( "-data" );

  if( m_doc->onTheFly() )
    writer->addArgument( QString("-tsize=%1s").arg(m_isoImager->size()) )->addArgument("-");
  else
    writer->addArgument( m_isoImageFilePath );
}


void K3bMixedJob::slotWriterNextTrack( int t, int tt )
{
  if( m_doc->mixedType() == K3bMixedDoc::DATA_FIRST_TRACK ) {
    if( t == 1 )
      emit newSubTask( i18n("Writing track %1 of %2 (%3)").arg(t).arg(tt).arg(i18n("Iso9660 data")) );
    else
      emit newSubTask( i18n("Writing track %1 of %2 (%3)").arg(t).arg(tt).arg(m_doc->audioDoc()->at(t-2)->filename()) );
  }
  else if( m_doc->mixedType() == K3bMixedDoc::DATA_LAST_TRACK ) {
    if( t == m_doc->audioDoc()->numberOfTracks()+1 )
      emit newSubTask( i18n("Writing track %1 of %2 (%3)").arg(t).arg(tt).arg(i18n("Iso9660 data")) );
    else
      emit newSubTask( i18n("Writing track %1 of %2 (%3)").arg(t).arg(tt).arg(m_doc->audioDoc()->at(t-1)->filename()) );
  }
  else {
    if( m_currentAction == WRITING_AUDIO_IMAGE )
      emit newSubTask( i18n("Writing track %1 of %2 (%3)").arg(t).arg(tt).arg(m_doc->audioDoc()->at(t-1)->filename()) );
    else
      emit newSubTask( i18n("Writing track %1 of %2 (%3)").arg(1).arg(1).arg(i18n("Iso9660 data")) );
  }
}


void K3bMixedJob::slotWriterJobPercent( int p )
{
  if( m_doc->mixedType() == K3bMixedDoc::DATA_SECOND_SESSION ) {
    if( m_currentAction == WRITING_AUDIO_IMAGE )
      emit percent( (int)( 100.0*(1.0-m_writingPartOfProcess)*m_audioDocPartOfProcess +
			   (double)p*m_audioDocPartOfProcess*m_writingPartOfProcess) );
    else
      emit percent( (int)( 100.0*m_audioDocPartOfProcess +
			   100.0*(1.0-m_writingPartOfProcess)*(1.0-m_audioDocPartOfProcess) +
			   + (double)p*(1.0-m_audioDocPartOfProcess)*m_writingPartOfProcess ) );
  }
  else {
    emit percent( (int)( 100.0*(1.0-m_writingPartOfProcess) + (double)p*m_writingPartOfProcess ) );
  }
}


void K3bMixedJob::slotAudioDecoderPercent( int p )
{
  if( !m_doc->onTheFly() ) {
    if( m_doc->mixedType() == K3bMixedDoc::DATA_SECOND_SESSION ) {
      emit percent( (int)( (double)p*m_audioDecoderPartOfProgress*(1.0-m_writingPartOfProcess) ) );
    }
    else {
      // the iso imager already finished
      // plus the current progress of the audiodecoder * audiopart * imagepart
      emit percent( (int)( 100.0*m_isoImagerPartOfProgress*(1.0-m_writingPartOfProcess) +
			   (double)p*m_audioDecoderPartOfProgress*(1.0-m_writingPartOfProcess) ) );
    }
  }
}


void K3bMixedJob::slotAudioDecoderSubPercent( int p )
{
  if( !m_doc->onTheFly() ) {
    emit subPercent( p );
  }
}


void K3bMixedJob::slotIsoImagerPercent( int p )
{
  if( !m_doc->onTheFly() ) {
    emit subPercent( p );
    if( m_doc->mixedType() == K3bMixedDoc::DATA_SECOND_SESSION ) {
      // the audio decoder finished
      // the normalizer finished
      // the writing of the audio part finished
      emit percent( (int)( 100.0*m_audioDocPartOfProcess*(1.0-m_writingPartOfProcess) +
			   100.0*m_audioDocPartOfProcess*m_writingPartOfProcess +
			   (double)p*(m_isoImagerPartOfProgress)*(1.0-m_writingPartOfProcess) ) );
    }
    else {
      emit percent( (int)((double)p*(m_isoImagerPartOfProgress)*(1.0-m_writingPartOfProcess)) );
    }
  }
}


bool K3bMixedJob::startWriting()
{
  if( m_doc->mixedType() == K3bMixedDoc::DATA_SECOND_SESSION ) {
    if( m_currentAction == WRITING_ISO_IMAGE) {
      if( m_doc->dummy() )
	emit newTask( i18n("Simulating second session") );
      else
	emit newTask( i18n("Writing second session") );
    }
    else {
      if( m_doc->dummy() )
	emit newTask( i18n("Simulating first session") );
      else
	emit newTask( i18n("Writing first session") );
    }
  }
  else if( m_doc->dummy() )
    emit newTask( i18n("Simulating") );
  else
    emit newTask( i18n("Writing") );


  // if we append the second session the cd is already in the drive
  if( !(m_doc->mixedType() == K3bMixedDoc::DATA_SECOND_SESSION
	&& m_currentAction == WRITING_ISO_IMAGE) ) {

    if( K3bEmptyDiscWaiter::wait( m_doc->burner() ) == K3bEmptyDiscWaiter::CANCELED ) {
      cancel();
      return false;
    }

    // just to be sure we did not get canceled during the async discWaiting
    if( m_canceled )
      return false;
  }

  emit burning(true);
  m_writer->start();

  if( m_doc->onTheFly() ) {
    // now the writer is running and we can get it's stdin
    // we only use this method when writing on-the-fly since
    // we cannot easily change the audioDecode fd while it's working
    // which we would need to do since we write into several
    // image files.
    m_audioDecoder->writeToFd( m_writer->fd() );
    m_isoImager->writeToFd( m_writer->fd() );
  }

  return true;
}


void K3bMixedJob::createIsoImage()
{
  m_currentAction = CREATING_ISO_IMAGE;

  // prepare iso image file
  m_isoImageFilePath = m_tempFilePrefix + "_datatrack.iso";

  if( !m_doc->onTheFly() )
    emit newTask( i18n("Creating iso image file") );
  emit newSubTask( i18n("Creating iso image in %1").arg(m_isoImageFilePath) );
  emit infoMessage( i18n("Creating iso image in %1").arg(m_isoImageFilePath), INFO );

  m_isoImager->writeToImageFile( m_isoImageFilePath );
  m_isoImager->start();
}


void K3bMixedJob::cleanupAfterError()
{
  m_errorOccuredAndAlreadyReported = true;
  m_audioDecoder->cancel();
  m_isoImager->cancel();
  if( m_writer )
    m_writer->cancel();

  if( m_tocFile ) delete m_tocFile;
  m_tocFile = 0;

  // remove the temp files
  removeBufferFiles();
}


void K3bMixedJob::removeBufferFiles()
{
  emit infoMessage( i18n("Removing buffer files."), INFO );

  if( QFile::exists( m_isoImageFilePath ) )
      if( !QFile::remove( m_isoImageFilePath ) )
	emit infoMessage( i18n("Could not delete file %1.").arg(m_isoImageFilePath), ERROR );

  QPtrListIterator<K3bAudioTrack> it( *m_doc->audioDoc()->tracks() );
  for( ; it.current(); ++it ) {
    K3bAudioTrack* track = it.current();
    const QString& f = m_tempData->bufferFileName(track);
    if( QFile::exists( f ) )
      if( !QFile::remove( f ) )
	emit infoMessage( i18n("Could not delete file %1.").arg(f), ERROR );
  }
}


void K3bMixedJob::determineWritingMode()
{
  // we don't need this when only creating image and it is possible
  // that the burn device is null
  if( m_doc->onlyCreateImages() )
    return;

  // at first we determine the data mode
  // --------------------------------------------------------------
  if( m_doc->dataDoc()->dataMode() == K3b::DATA_MODE_AUTO ) {
    if( m_doc->mixedType() == K3bMixedDoc::DATA_SECOND_SESSION )
      m_usedDataMode = K3b::MODE2;
    else
      m_usedDataMode = K3b::MODE1;
  }
  else
    m_usedDataMode = m_doc->dataDoc()->dataMode();


  // we try to use cdrecord if possible
  bool cdrecordOnTheFly = false;
  bool cdrecordCdText = false;
  bool cdrecordUsable = false;
  
  if( k3bcore->externalBinManager()->binObject("cdrecord") ) {
    cdrecordOnTheFly =
      k3bcore->externalBinManager()->binObject("cdrecord")->version
      >= K3bVersion( 2, 1, -1, "a13" );
    cdrecordCdText =
      k3bcore->externalBinManager()->binObject("cdrecord")->hasFeature( "cdtext" );
    cdrecordUsable =
      !( !cdrecordOnTheFly && m_doc->onTheFly() ) &&
      !( m_doc->audioDoc()->cdText() &&
	 // the inf-files we use do only support artist and title in the global section
	 ( !m_doc->audioDoc()->arranger().isEmpty() ||
	   !m_doc->audioDoc()->songwriter().isEmpty() ||
	   !m_doc->audioDoc()->composer().isEmpty() ||
	   !m_doc->audioDoc()->cdTextMessage().isEmpty() ||
	   !cdrecordCdText )
	 );
  }

  // Writing Application
  // --------------------------------------------------------------
  // cdrecord seems to have problems writing xa 1 disks in dao mode? At least on my system!
  if( writingApp() == K3b::DEFAULT ) {
    if( m_doc->mixedType() == K3bMixedDoc::DATA_SECOND_SESSION ) {
      if( m_doc->writingMode() == K3b::DAO ||
	  ( m_doc->writingMode() == K3b::WRITING_MODE_AUTO && !cdrecordUsable ) ) {
	m_usedAudioWritingApp = K3b::CDRDAO;
	m_usedDataWritingApp = K3b::CDRDAO;
      }
      else {
	m_usedAudioWritingApp = K3b::CDRECORD;
	m_usedDataWritingApp = K3b::CDRECORD;
      }
    }
    else {
      if( cdrecordUsable ) {
	m_usedAudioWritingApp = K3b::CDRECORD;
	m_usedDataWritingApp = K3b::CDRECORD;
      }
      else {
	m_usedAudioWritingApp = K3b::CDRDAO;
	m_usedDataWritingApp = K3b::CDRDAO;
      }
    }
  }
  else {
    m_usedAudioWritingApp = writingApp();
    m_usedDataWritingApp = writingApp();
  }


  // Writing Mode (TAO/DAO/RAW)
  // --------------------------------------------------------------
  if( m_doc->writingMode() == K3b::WRITING_MODE_AUTO ) {

    if( m_doc->mixedType() == K3bMixedDoc::DATA_SECOND_SESSION ) {
      if( m_usedDataWritingApp == K3b::CDRECORD )
	m_usedDataWritingMode = K3b::TAO;
      else
	m_usedDataWritingMode = K3b::DAO;

      // default to Session at once for the audio part
      m_usedAudioWritingMode = K3b::DAO;
    }
    else if( writer()->dao() ) {
      m_usedDataWritingMode = K3b::DAO;
      m_usedAudioWritingMode = K3b::DAO;
    }
    else {
      m_usedDataWritingMode = K3b::TAO;
      m_usedAudioWritingMode = K3b::TAO;
    }
  }
  else {
    m_usedAudioWritingMode = m_doc->writingMode();
    m_usedDataWritingMode = m_doc->writingMode();
  }


  if( m_usedDataWritingApp == K3b::CDRECORD ) {
    if( !cdrecordOnTheFly && m_doc->onTheFly() ) {
      m_doc->setOnTheFly( false );
      emit infoMessage( i18n("On-the-fly writing with cdrecord < 2.01a13 not supported."), ERROR );
    }

    if( m_doc->audioDoc()->cdText() ) {
      if( !cdrecordCdText ) {
	m_doc->audioDoc()->writeCdText( false );
	emit infoMessage( i18n("Cdrecord %1 does not support CD-Text writing.").arg(k3bcore->externalBinManager()->binObject("cdrecord")->version), ERROR );
      }
      else if( m_usedAudioWritingMode == K3b::TAO ) {
	emit infoMessage( i18n("It is not possible to write CD-Text in TAO mode. Try DAO or RAW."), WARNING );
      }
      else {
	if( !m_doc->audioDoc()->arranger().isEmpty() )
	  emit infoMessage( i18n("K3b does not support Album arranger CD-Text with cdrecord."), ERROR );
	if( !m_doc->audioDoc()->songwriter().isEmpty() )
	  emit infoMessage( i18n("K3b does not support Album songwriter CD-Text with cdrecord."), ERROR );
	if( !m_doc->audioDoc()->composer().isEmpty() )
	  emit infoMessage( i18n("K3b does not support Album composer CD-Text with cdrecord."), ERROR );
	if( !m_doc->audioDoc()->cdTextMessage().isEmpty() )
	  emit infoMessage( i18n("K3b does not support Album comment CD-Text with cdrecord."), ERROR );
      }
    }
  }
}


void K3bMixedJob::normalizeFiles()
{
  if( !m_normalizeJob ) {
    m_normalizeJob = new K3bAudioNormalizeJob( this );

    connect( m_normalizeJob, SIGNAL(infoMessage(const QString&, int)),
	     this, SIGNAL(infoMessage(const QString&, int)) );
    connect( m_normalizeJob, SIGNAL(percent(int)), this, SLOT(slotNormalizeProgress(int)) );
    connect( m_normalizeJob, SIGNAL(subPercent(int)), this, SLOT(slotNormalizeSubProgress(int)) );
    connect( m_normalizeJob, SIGNAL(finished(bool)), this, SLOT(slotNormalizeJobFinished(bool)) );
    connect( m_normalizeJob, SIGNAL(newTask(const QString&)), this, SIGNAL(newSubTask(const QString&)) );
    connect( m_normalizeJob, SIGNAL(debuggingOutput(const QString&, const QString&)),
	     this, SIGNAL(debuggingOutput(const QString&, const QString&)) );
  }

  // add all the files
  QValueVector<QString> files;
  QPtrListIterator<K3bAudioTrack> it( *m_doc->audioDoc()->tracks() );
  for( ; it.current(); ++it ) {
    K3bAudioTrack* track = it.current();
    files.append( m_tempData->bufferFileName(track) );
  }

  m_normalizeJob->setFilesToNormalize( files );

  emit newTask( i18n("Normalizing volume levels") );
  m_normalizeJob->start();
}

void K3bMixedJob::slotNormalizeJobFinished( bool success )
{
  if( m_canceled || m_errorOccuredAndAlreadyReported )
    return;

  if( success ) {
    if( m_doc->mixedType() == K3bMixedDoc::DATA_FIRST_TRACK )
      m_currentAction = WRITING_ISO_IMAGE;
    else
      m_currentAction = WRITING_AUDIO_IMAGE;

    if( !prepareWriter() ) {
      cleanupAfterError();
      emit finished(false);
      return;
    }
    else
      startWriting();
  }
  else {
    cleanupAfterError();
    emit finished(false);
  }
}

void K3bMixedJob::slotNormalizeProgress( int p )
{
  if( m_doc->mixedType() == K3bMixedDoc::DATA_SECOND_SESSION ) {
    // the decoder finished
    // plus the normalize progress

    emit percent( (int)( 100.0*m_audioDecoderPartOfProgress*(1.0-m_writingPartOfProcess) +
			 (double)p*m_normalizerPartOfProgress*(1.0-m_writingPartOfProcess) ) );
  }
  else {
    // the iso imager already finished
    // the decoder finished
    // plus the normalize progress

    emit percent( (int)( 100.0*m_isoImagerPartOfProgress*(1.0-m_writingPartOfProcess) +
			 100.0*m_audioDecoderPartOfProgress*(1.0-m_writingPartOfProcess) +
			 (double)p*m_normalizerPartOfProgress*(1.0-m_writingPartOfProcess) ) );
  }
}


void K3bMixedJob::slotNormalizeSubProgress( int p )
{
  emit subPercent( p );
}


void K3bMixedJob::prepareProgressInformation()
{
  // all these values are basically useful when not writing on the fly
  // since with on-the-fly the writer gets the full percentage

  // calculate percentage of audio and data
  // this is also used in on-the-fly mode
  double ds = (double)m_doc->dataDoc()->length().totalFrames();
  double as = (double)m_doc->audioDoc()->length().totalFrames();
  m_audioDocPartOfProcess = as/(ds+as);

  if( m_doc->onTheFly() )
    m_writingPartOfProcess = 1.0;
  else {
    m_writingPartOfProcess = 0.5;
  }

  m_audioDecoderPartOfProgress = ( m_doc->audioDoc()->normalize() ?
				   m_audioDocPartOfProcess/2 :
				   m_audioDocPartOfProcess );

  m_isoImagerPartOfProgress = 1.0 - m_audioDocPartOfProcess;

  m_normalizerPartOfProgress = ( m_doc->audioDoc()->normalize() ?
				 m_audioDecoderPartOfProgress :
				 0 );
}


QString K3bMixedJob::jobDescription() const
{
  if( m_doc->audioDoc()->title().isEmpty() )
    return i18n("Writing mixed mode cd");
  else
    return i18n("Writing mixed mode cd (%1)").arg(m_doc->audioDoc()->title());
}


QString K3bMixedJob::jobDetails() const
{
  return i18n("%1 tracks (%2 minutes audio data, %3 Iso9660 data)").arg(m_doc->numOfTracks()).arg(m_doc->audioDoc()->length().toString()).arg(KIO::convertSize(m_doc->dataDoc()->size()));
}

#include "k3bmixedjob.moc"
