/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */




#include "k3bmixedjob.h"
#include "k3bmixeddoc.h"

#include <data/k3bdatadoc.h>
#include <data/k3bisoimager.h>
#include <data/k3bmsinfofetcher.h>
#include <audio/k3baudiodecoder.h>
#include <audio/k3baudiodoc.h>
#include <audio/k3baudiotrack.h>
#include <audio/k3baudiotocfilewriter.h>
#include <device/k3bdevicemanager.h>
#include <device/k3bdevice.h>
#include <device/k3bmsf.h>
#include <tools/k3bwavefilewriter.h>
#include <tools/k3bglobals.h>
#include <tools/k3bexternalbinmanager.h>
#include <k3bemptydiscwaiter.h>
#include <k3b.h>
#include <k3bcdrecordwriter.h>
#include <k3bcdrdaowriter.h>

#include <qfile.h>
#include <qdatastream.h>

#include <kdebug.h>
#include <klocale.h>
#include <ktempfile.h>
#include <kio/netaccess.h>


// for the fifo
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>


K3bMixedJob::K3bMixedJob( K3bMixedDoc* doc, QObject* parent )
  : K3bBurnJob( parent ),
    m_doc( doc )
{
  m_isoImager = new K3bIsoImager( doc->dataDoc(), this );
  connect( m_isoImager, SIGNAL(sizeCalculated(int, int)), this, SLOT(slotSizeCalculationFinished(int, int)) );
  connect( m_isoImager, SIGNAL(infoMessage(const QString&, int)), this, SIGNAL(infoMessage(const QString&, int)) );
  connect( m_isoImager, SIGNAL(data(char*, int)), this, SLOT(slotReceivedIsoImagerData(char*, int)) );
  connect( m_isoImager, SIGNAL(percent(int)), this, SLOT(slotIsoImagerPercent(int)) );
  connect( m_isoImager, SIGNAL(finished(bool)), this, SLOT(slotIsoImagerFinished(bool)) );

  m_audioDecoder = new K3bAudioDecoder( doc->audioDoc(), this );
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
  m_isoImageFile = 0;
}


K3bMixedJob::~K3bMixedJob()
{
  delete m_waveFileWriter;
  if( m_tocFile ) delete m_tocFile;
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

  // calculate percentage of audio and data
  double ds = (double)m_doc->dataDoc()->length().totalFrames();
  double as = (double)m_doc->audioDoc()->length().totalFrames();
  m_audioDocPartOfProcess = as/(ds+as);


  // set some flags that are needed
  m_doc->audioDoc()->setOnTheFly( m_doc->onTheFly() );  // for the toc writer
  m_doc->dataDoc()->setBurner( m_doc->burner() );  // so the isoImager can read ms data


  if( writingApp() == K3b::CDRECORD ||
      ( writingApp() == K3b::DEFAULT && !m_doc->dao() ) )
    m_usedWritingApp = K3b::CDRECORD;
  else
    m_usedWritingApp = K3b::CDRDAO;


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
      
      if( startWriting() )
	m_audioDecoder->start();
    }
  }
  else {
    emit newTask( i18n("Creating image files") );

    if( m_doc->mixedType() != K3bMixedDoc::DATA_SECOND_SESSION ) {
      createIsoImage();
    }
    else {
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
  emit finished(false);
}


void K3bMixedJob::slotMsInfoFetched( bool success )
{
  if( m_canceled || m_errorOccuredAndAlreadyReported )
    return;

  if( success ) {
    if( m_usedWritingApp == K3b::CDRECORD )
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


void K3bMixedJob::slotReceivedIsoImagerData( char* data, int len )
{
  if( m_doc->onTheFly() ) {
    if( !m_writer->write( data, len ) )
      kdDebug() << "(K3bMixedJob) Error while writing data to Writer" << endl;
  }
  else {
    m_isoImageFileStream->writeRawBytes( data, len );
    m_isoImager->resume();
  }
}


void K3bMixedJob::slotDataWritten()
{
  if( m_currentAction == WRITING_ISO_IMAGE )
    m_isoImager->resume();
  else
    m_audioDecoder->resume();
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
    m_isoImageFile->close();
    emit infoMessage( i18n("Iso image successfully created."), INFO );

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

  if( m_doc->mixedType() == K3bMixedDoc::DATA_SECOND_SESSION && m_currentAction == WRITING_AUDIO_IMAGE ) {
    // reload the media
    emit infoMessage( i18n("Reloading the media"), INFO );
    m_doc->burner()->eject();
    qApp->processEvents();
    m_doc->burner()->load();

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
  else {
    if( !m_doc->onTheFly() && m_doc->removeBufferFiles() )
      removeBufferFiles();

    emit finished(true);
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

    // TODO: enable me after message freeze
    //    emit infoMessage( i18n("Audio images successfully created."), STATUS );

    if( m_doc->mixedType() == K3bMixedDoc::DATA_FIRST_TRACK )
      m_currentAction = WRITING_ISO_IMAGE;
    else
      m_currentAction = WRITING_AUDIO_IMAGE;
    
    if( !prepareWriter() ) {
      cleanupAfterError();
      emit finished(false);
      return;
    }
    
    startWriting();
  }
}


void K3bMixedJob::slotReceivedAudioDecoderData( const char* data, int len )
{
  if( m_doc->onTheFly() ) {
    if( m_usingFifo )
      ::write( m_fifo, data, len );
    else if( !m_writer->write( (char*)data, len ) ) {
      kdDebug() << "(K3bMixedJob) Error while writing data to Writer" << endl;
      emit infoMessage( i18n("IO error"), ERROR );
      cleanupAfterError();
      emit finished(false);
      return;
    }
  }
  else {
    m_waveFileWriter->write( data, len );
    m_audioDecoder->resume();
  }
}


void K3bMixedJob::slotAudioDecoderNextTrack( int t, int tt )
{
  if( !m_doc->onTheFly() ) {
    emit newSubTask( i18n("Decoding audiotrack %1 of %2 (%3)").arg(t).arg(tt).arg(m_doc->audioDoc()->at(t-1)->fileName()) );
    //emit infoMessage( i18n("Decoding audiotrack %1 (%2)").arg(t).arg(m_doc->audioDoc()->at(t-1)->fileName()), INFO );
    QString bf = k3bMain()->findTempFile( "wav", m_doc->imagePath() );
    if( !m_waveFileWriter->open( bf ) ) {
      emit infoMessage( i18n("Could not open file %1 for writing.").arg(m_waveFileWriter->filename()), ERROR );
      cleanupAfterError();
      emit finished( false );
      return;
    }

    m_doc->audioDoc()->at(t-1)->setBufferFile( bf );
  }
  else if( m_usingFifo ) {
    if( t > 1 )
      ::close( m_fifo );

    m_fifo = ::open( m_doc->audioDoc()->at(t-1)->bufferFile().latin1(), O_WRONLY | O_NONBLOCK );
    if( m_fifo < 0 ) {
      kdDebug() << "(K3bMixedJob) Could not open fifo " << strerror(errno) << endl;
    }
  }
}


bool K3bMixedJob::prepareWriter()
{
  if( m_writer ) delete m_writer;

  if( m_usedWritingApp == K3b::CDRECORD ) {

    K3bCdrecordWriter* writer = new K3bCdrecordWriter( m_doc->burner(), this );

    // only write the audio tracks in DAO mode
    writer->setDao( m_doc->dao() && (m_doc->mixedType() != K3bMixedDoc::DATA_SECOND_SESSION 
				     || m_currentAction == WRITING_AUDIO_IMAGE) );
    writer->setSimulate( m_doc->dummy() );
    writer->setBurnproof( m_doc->burnproof() );
    writer->setBurnSpeed( m_doc->speed() );
    writer->setProvideStdin( m_doc->onTheFly() );
    writer->prepareArgumentList();

    m_usingFifo = m_doc->onTheFly();

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
    m_usingFifo = false;

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

    if( m_doc->onTheFly() ) {
      writer->setProvideStdin(true);
    }

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
  connect( m_writer, SIGNAL(writeSpeed(int)), this, SIGNAL(writeSpeed(int)) );
  connect( m_writer, SIGNAL(finished(bool)), this, SLOT(slotWriterFinished(bool)) );
  connect( m_writer, SIGNAL(dataWritten()), this, SLOT(slotDataWritten()) );
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
    if( ( m_doc->dataDoc()->dataMode() == K3b::AUTO && m_doc->mixedType() == K3bMixedDoc::DATA_SECOND_SESSION ) ||
	m_doc->dataDoc()->dataMode() == K3b::MODE2 ) {
      *s << "CD_ROM_XA" << endl;
    }
    else {
      *s << "CD_ROM" << endl;
    }
    *s << endl;


    if( ( m_doc->mixedType() == K3bMixedDoc::DATA_SECOND_SESSION &&
	  m_currentAction == WRITING_AUDIO_IMAGE ) ||
	m_doc->mixedType() == K3bMixedDoc::DATA_LAST_TRACK ) 
      K3bAudioTocfileWriter::writeAudioToc( m_doc->audioDoc(), *s );

    if( m_doc->mixedType() != K3bMixedDoc::DATA_SECOND_SESSION ||
	m_currentAction == WRITING_ISO_IMAGE ) {

      if( ( m_doc->dataDoc()->dataMode() == K3b::AUTO && m_doc->mixedType() == K3bMixedDoc::DATA_SECOND_SESSION ) ||
	  m_doc->dataDoc()->dataMode() == K3b::MODE2 )
	*s << "TRACK MODE2_FORM1" << endl;
      else
	*s << "TRACK MODE1" << endl;
      if( m_doc->onTheFly() )
	*s << "DATAFILE \"-\" " << m_isoImager->size()*2048 << endl;
      else
	*s << "DATAFILE \"" << m_isoImageFile->name() << "\"" << endl;
      *s << endl;
    }
      
    if( m_doc->mixedType() == K3bMixedDoc::DATA_FIRST_TRACK )
      K3bAudioTocfileWriter::writeAudioToc( m_doc->audioDoc(), *s );
  
    m_tocFile->close();


    // debugging
    //    KIO::NetAccess::copy( m_tocFile->name(), k3bMain()->findTempFile( "toc" ) );

    return true;
  }
  else 
    return false;
}


void K3bMixedJob::addAudioTracks( K3bCdrecordWriter* writer )
{
  // add all the audio tracks
  writer->addArgument( "-audio" );

  QListIterator<K3bAudioTrack> it( *m_doc->audioDoc()->tracks() );
  for( ; it.current(); ++it ) {
    K3bAudioTrack* track = it.current();

    if( !track->copyProtection() )
      writer->addArgument( "-copy" );
    else
      writer->addArgument( "-nocopy" );
    if( track->preEmp() )
      writer->addArgument( "-preemp" );
    else
     writer->addArgument( "-nopreemp" );

    writer->addArgument( QString("-pregap=%1").arg(track->pregap().totalFrames()) );
    if( m_doc->onTheFly() ) {
      QString fifoname = QString("/home/trueg/tmp/fifo_track%1").arg(track->index());
      if( ::mkfifo( fifoname.latin1(), S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH ) == -1 ) {
	kdDebug() << "(K3bMixedJob) could not create fifo" << endl;
      }
      track->setBufferFile( fifoname );  // just a temp solution
      writer->addArgument( QString("-tsize=%1f").arg(track->length().totalFrames()) )->addArgument(fifoname);
    }
    else
      writer->addArgument( QFile::encodeName(track->bufferFile()) );
  }
}

void K3bMixedJob::addDataTrack( K3bCdrecordWriter* writer )
{
  // add data track
  if( ( m_doc->dataDoc()->dataMode() == K3b::AUTO && m_doc->mixedType() == K3bMixedDoc::DATA_SECOND_SESSION ) ||
      m_doc->dataDoc()->dataMode() == K3b::MODE2 )
    writer->addArgument( "-xa1" );
  else
    writer->addArgument( "-data" );

  if( m_doc->onTheFly() )
    writer->addArgument( QString("-tsize=%1s").arg(m_isoImager->size()) )->addArgument("-");
  else
    writer->addArgument( m_isoImageFile->name() );
}


void K3bMixedJob::slotWriterNextTrack( int t, int tt )
{
  if( m_doc->mixedType() == K3bMixedDoc::DATA_FIRST_TRACK ) {
    if( t == 1 )
      emit newSubTask( i18n("Writing track %1 of %2 (%3)").arg(t).arg(tt).arg(i18n("Iso9660 data")) );
    else
      emit newSubTask( i18n("Writing track %1 of %2 (%3)").arg(t).arg(tt).arg(m_doc->audioDoc()->at(t-2)->fileName()) );
  }
  else if( m_doc->mixedType() == K3bMixedDoc::DATA_LAST_TRACK ) {
    if( t == m_doc->audioDoc()->numberOfTracks()+1 )
      emit newSubTask( i18n("Writing track %1 of %2 (%3)").arg(t).arg(tt).arg(i18n("Iso9660 data")) );
    else
      emit newSubTask( i18n("Writing track %1 of %2 (%3)").arg(t).arg(tt).arg(m_doc->audioDoc()->at(t-1)->fileName()) );
  }
  else {
    if( m_currentAction == WRITING_AUDIO_IMAGE )
      emit newSubTask( i18n("Writing track %1 of %2 (%3)").arg(t).arg(tt).arg(m_doc->audioDoc()->at(t-1)->fileName()) );
    else
      emit newSubTask( i18n("Writing track %1 of %2 (%3)").arg(1).arg(1).arg(i18n("Iso9660 data")) );
  }
}


void K3bMixedJob::slotWriterJobPercent( int p )
{
  if( m_doc->onTheFly() ) {
    if( m_doc->mixedType() == K3bMixedDoc::DATA_SECOND_SESSION ) {
      if( m_currentAction == WRITING_AUDIO_IMAGE ) {
	emit percent( (int)((double)p * m_audioDocPartOfProcess) );
      }
      else {
	emit percent( (int)( 100.0*m_audioDocPartOfProcess + (1.0-m_audioDocPartOfProcess)*(double)p ) );
      }
    }
    else
      emit percent(p);
  }
  else {
    if( m_doc->mixedType() == K3bMixedDoc::DATA_SECOND_SESSION ) {
      if( m_currentAction == WRITING_AUDIO_IMAGE )
	emit percent( (int)(50.0*m_audioDocPartOfProcess) + (int)((double)p*m_audioDocPartOfProcess*0.5) );
      else
	emit percent( (int)( 100.0*m_audioDocPartOfProcess + 50.0*(1.0-m_audioDocPartOfProcess)
			     + (double)p*(1.0-m_audioDocPartOfProcess)*0.5 ) );
    }
    else {
      emit percent( 50 + p/2 );
    }
  }
}


void K3bMixedJob::slotAudioDecoderPercent( int p )
{
  if( !m_doc->onTheFly() ) {
    if( m_doc->mixedType() == K3bMixedDoc::DATA_SECOND_SESSION ) {
      emit percent( (int)( (double)p*m_audioDocPartOfProcess*0.5 ) );
    }
    else {
      // the iso imager already finished
      emit percent( (int)((1.0-m_audioDocPartOfProcess)*50.0 + (double)p*m_audioDocPartOfProcess*0.5) );
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
      emit percent( (int)( 100.0*m_audioDocPartOfProcess
			   + (double)p*(1.0-m_audioDocPartOfProcess)*0.5 ) );
    }
    else {
      emit percent( (int)((double)p*(1.0-m_audioDocPartOfProcess)*0.5) );
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

    K3bEmptyDiscWaiter waiter( m_doc->burner(), k3bMain() );
    if( waiter.waitForEmptyDisc() == K3bEmptyDiscWaiter::CANCELED ) {
      cancel();
      return false;
    }
  }
	
  m_writer->start();
  return true;
}


void K3bMixedJob::createIsoImage()
{
  m_currentAction = CREATING_ISO_IMAGE;

  // prepare iso image file
  m_isoImageFile = new QFile( k3bMain()->findTempFile( "iso", m_doc->imagePath() ) );
  if( !m_isoImageFile->open( IO_WriteOnly ) ) {
    emit infoMessage( i18n("Could not open file %1 for writing.").arg(m_isoImageFile->name()), ERROR );
    cleanupAfterError();
    emit finished( false );
    return;
  }
  
  m_isoImageFileStream = new QDataStream( m_isoImageFile );

  emit newSubTask( i18n("Creating iso image in %1").arg(m_isoImageFile->name()) );
  emit infoMessage( i18n("Creating iso image in %1").arg(m_isoImageFile->name()), INFO );
  
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

  if( m_isoImageFile )
    if( m_isoImageFile->exists() )
      if( !m_isoImageFile->remove() )
	emit infoMessage( i18n("Could not delete file %1.").arg(m_isoImageFile->name()), ERROR );

  QListIterator<K3bAudioTrack> it( *m_doc->audioDoc()->tracks() );
  for( ; it.current(); ++it ) {
    K3bAudioTrack* track = it.current();
    if( QFile::exists( track->bufferFile() ) )
      if( !QFile::remove( track->bufferFile() ) )
	emit infoMessage( i18n("Could not delete file %1.").arg(track->bufferFile()), ERROR );
  }
}


#include "k3bmixedjob.moc"
