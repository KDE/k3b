/* 
 *
 * $Id$
 * Copyright (C) 2004 Sebastian Trueg <trueg@k3b.org>
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

#include "k3baudioconverterjob.h"
#include "k3baudioconverterviewitem.h"

#include <k3bwavefilewriter.h>
#include <k3bthread.h>
#include <k3baudioencoder.h>
#include <k3baudiodecoder.h>
#include <k3bglobals.h>

#include <klocale.h>


class K3bAudioConverterJob::WorkThread : public K3bThread
{
public:
  WorkThread( QListView* view, K3bAudioEncoderFactory* f, const QString& type, const QString& dest )
    : K3bThread(),
      m_view(view),
      m_encoderFactory(f),
      m_encoder(0),
      m_waveFileWriter(0),
      m_encoderExtension(type),
      m_destDir(dest) {
  }


  ~WorkThread() {
    delete m_waveFileWriter;
    delete m_encoder;
  }


  void run() {
    m_canceled = false;
    int failedCnt = 0;

    emitStarted();
    emitNewTask( i18n("Converting") );

    //
    // Get the encoder
    //
    if( m_encoderFactory )
      m_encoder = static_cast<K3bAudioEncoder*>(m_encoderFactory->createPlugin());
    else
      m_waveFileWriter = new K3bWaveFileWriter();


    long long readOverall = 0;
    K3b::Msf lengthOverall;
    for( QListViewItemIterator it( m_view ); it.current(); ++it )
      lengthOverall += static_cast<K3bAudioConverterViewItem*>( *it )->decoder()->length();

    for( QListViewItemIterator it( m_view ); it.current(); ++it ) {
      K3bAudioConverterViewItem* converterItem = static_cast<K3bAudioConverterViewItem*>( *it );
      QString sourceFilename = converterItem->text(0);

      emitNewSubTask( i18n("Converting file %1").arg(sourceFilename) );

      //
      // initialize the decoder
      //
      if( !converterItem->decoder()->initDecoder() ) {
	emitInfoMessage( i18n("Error while initializing decoding."), K3bJob::ERROR );
	failedCnt++;
	continue;
      }


      //
      // initialize encoder
      //
      QString destFilename = K3b::prepareDir(m_destDir) + converterItem->path().section( "/", -1 );
      destFilename.truncate( destFilename.findRev( "." )+1 );
      if( m_encoder )
	destFilename += m_encoderExtension;
      else
	destFilename += "wav";

      emitInfoMessage( i18n("Writing file %1.").arg(destFilename), K3bJob::INFO );

      bool isOpen = true;
      if( m_encoder ) {
	isOpen = m_encoder->openFile( m_encoderExtension, destFilename, converterItem->decoder()->length() );
      
 	m_encoder->setMetaData( K3bAudioEncoder::META_TRACK_ARTIST,
				converterItem->decoder()->metaInfo(K3bAudioDecoder::META_TITLE) );
 	m_encoder->setMetaData( K3bAudioEncoder::META_TRACK_TITLE,
				converterItem->decoder()->metaInfo(K3bAudioDecoder::META_ARTIST) );
 	m_encoder->setMetaData( K3bAudioEncoder::META_TRACK_COMMENT,
				converterItem->decoder()->metaInfo(K3bAudioDecoder::META_COMMENT) );
      }
      else {
	isOpen = m_waveFileWriter->open( destFilename );
      }

      if( !isOpen ) {
	emitInfoMessage( i18n("Unable to open '%1' for writing.").arg(destFilename), K3bJob::ERROR );
	failedCnt++;
	converterItem->decoder()->cleanup();
	continue;
      }

      //
      // Now do the conversion
      //
      char buffer[10*1024];
      const int bufferLength = 10*1024;
      bool success = true;
      int readLength = 0;
      long long readFile = 0;
      while( !m_canceled && ( readLength = converterItem->decoder()->decode( buffer, bufferLength ) ) > 0 ) {

	if( m_encoder ) {
	  // the modules produce big endian samples
	  // so we need to swap the bytes here
	  char b;
	  for( int i = 0; i < bufferLength-1; i+=2 ) {
	    b = buffer[i];
	    buffer[i] = buffer[i+1];
	    buffer[i+1] = b;
	  }

	  if( m_encoder->encode( buffer, readLength ) < 0 ) {
	    kdDebug() << "(K3bAudioConverterJob) error while encoding." << endl;
	    emitInfoMessage( i18n("Error while encoding."), K3bJob::ERROR );
	    success = false;
	    break;
	  }
	}
	else {
	  m_waveFileWriter->write( buffer, 
				   readLength, 
				   K3bWaveFileWriter::BigEndian );
	}

	readOverall += readLength;
	readFile += readLength;
	emitSubPercent( 100*readFile/converterItem->decoder()->length().audioBytes() );
	emitPercent( 100*readOverall/lengthOverall.audioBytes() );
      }


      //
      // Close the encoder
      //
      if( m_encoder )
	m_encoder->closeFile();
      else
	m_waveFileWriter->close();

      //
      // Cleanup the decoder
      //
      converterItem->decoder()->cleanup();


      if( m_canceled ) {
	emitCanceled();
	break;
      }
      else {
	if( readLength < 0 || !success ) {
	  // TODO: do stuff like deleting the file
	  failedCnt++;
	  emitInfoMessage( i18n("Converting file %1 failed.").arg(sourceFilename), K3bJob::ERROR );
	}
	else
	  emitInfoMessage( i18n("Successfully converted file %1.").arg(sourceFilename), K3bJob::SUCCESS );
      }
    }

    emitFinished(failedCnt == 0 && !m_canceled );
  }


  void cancel() {
    m_canceled = true;
  }

  QString jobDescription() const {
    return i18n("Converting 1 audio file", "Converting %n audio files", m_view->childCount());
  }

  QString jobDetails() const {
    return i18n("Encoding to %1").arg( m_encoderFactory 
				       ? m_encoderFactory->fileTypeComment(m_encoderExtension)
				       : i18n("Wave") );
  }


  bool m_canceled;
  QListView* m_view;
  K3bAudioEncoderFactory* m_encoderFactory;
  K3bAudioEncoder* m_encoder;
  K3bWaveFileWriter* m_waveFileWriter;
  QString m_encoderExtension;
  QString m_destDir;
};


K3bAudioConverterJob::K3bAudioConverterJob( QListView* view, 
					    K3bAudioEncoderFactory* f, 
					    const QString& type,
					    const QString& dest,
					    K3bJobHandler* jh, 
					    QObject* parent, const char* name )
  : K3bThreadJob( jh, parent, name )
{
  m_thread = new WorkThread( view, f, type, dest );
  setThread( m_thread );
}


K3bAudioConverterJob::~K3bAudioConverterJob()
{
  delete m_thread;
}
