/* 
 *
 * $Id: $
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


#include <config.h>
#include <kdebug.h>


#ifdef OGG_VORBIS

#include "k3boggvorbismodule.h"
#include "../../k3baudiotrack.h"

#include <qtimer.h>
#include <qfile.h>
#include <qstringlist.h>

#include <kurl.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>


K3bOggVorbisModule::K3bOggVorbisModule( QObject* parent, const char* name )
  : K3bAudioModule( parent, name )
{
  m_oggVorbisFile = new OggVorbis_File;
  m_outputBuffer  = new char[OUTPUT_BUFFER_SIZE];

  m_decodingTimer = new QTimer( this );
  connect( m_decodingTimer, SIGNAL(timeout()), this, SLOT(decode()) );


  m_bDecodingInProgress = false;
}


K3bOggVorbisModule::~K3bOggVorbisModule()
{
  delete m_oggVorbisFile;
  delete [] m_outputBuffer;
}


void K3bOggVorbisModule::startDecoding()
{
  // open the file
  FILE* file = fopen( QFile::encodeName(audioTrack()->absPath()), "r" );
  if( !file ) {
    kdDebug() << "(K3bOggVorbisModule) Could not open file " << audioTrack()->absPath() << endl;
    emit finished( false );
  }
  else {
    if( ov_open( file, m_oggVorbisFile, 0, 0 ) ) {
      kdDebug() << "(K3bOggVorbisModule) " << audioTrack()->absPath() << " seems not to to be an ogg vorbis file." << endl;
      fclose( file );
      emit finished( false );
    }
    else {
      kdDebug() << "(K3bOggVorbisModule) start decoding of file " << audioTrack()->absPath() << endl;

      m_rawDataLengthToStream = audioTrack()->length()*2352;
      m_rawDataAlreadyStreamed = 0;

      m_bDecodingInProgress = true;
      m_decodingTimer->start(0);
    }
  }
}


void K3bOggVorbisModule::cancel()
{
  if( m_bDecodingInProgress ) {
    m_bDecodingInProgress = false;
    m_decodingTimer->stop();

    ov_clear( m_oggVorbisFile );

    emit canceled();
    emit finished( false );
  }
}


void K3bOggVorbisModule::decode()
{
  if( m_bDecodingInProgress ) {
 
    long bytesRead = ov_read( m_oggVorbisFile, m_outputBuffer, OUTPUT_BUFFER_SIZE, 1, 2, 1, &m_currentOggVorbisSection );

    if( bytesRead == OV_HOLE ) {
      // TODO: I think we can go on here?

      kdDebug() << "(K3bOggVorbisModule) OV_HOLE" << endl;
      
      ov_clear( m_oggVorbisFile );

      m_bDecodingInProgress = false;
      m_decodingTimer->stop();
      emit finished( false );
    }

    else if( bytesRead == OV_EBADLINK ) {
      kdDebug() << "(K3bOggVorbisModule) OV_EBADLINK" << endl;

      ov_clear( m_oggVorbisFile );

      m_bDecodingInProgress = false;
      m_decodingTimer->stop();
      emit finished( false );
    }

    else if( bytesRead < 0 ) {

      // TODO: add a method in the upcoming vorbisLib class that returnes
      // an error string
      // #define OV_FALSE      -1
      // #define OV_EOF        -2
      // #define OV_HOLE       -3

      // #define OV_EREAD      -128
      // #define OV_EFAULT     -129
      // #define OV_EIMPL      -130
      // #define OV_EINVAL     -131
      // #define OV_ENOTVORBIS -132
      // #define OV_EBADHEADER -133
      // #define OV_EVERSION   -134
      // #define OV_ENOTAUDIO  -135
      // #define OV_EBADPACKET -136
      // #define OV_EBADLINK   -137
      // #define OV_ENOSEEK    -138

      kdDebug() << "(K3bOggVorbisModule) Error: " << bytesRead << endl;

      ov_clear( m_oggVorbisFile );

      m_bDecodingInProgress = false;
      m_decodingTimer->stop();
      emit finished( false );
    }

    else if( bytesRead == 0 ) {
      // eof
      // pad if necessary
      if( m_rawDataAlreadyStreamed < m_rawDataLengthToStream ) {
	unsigned long bytesToPad = m_rawDataLengthToStream - m_rawDataAlreadyStreamed;
	kdDebug() << "(K3bOggVorbisModule) we have to pad by " << bytesToPad << "i bytes" << endl;

	memset( m_outputBuffer, 0, OUTPUT_BUFFER_SIZE );

	long bytesToOutput = ( bytesToPad < OUTPUT_BUFFER_SIZE ? bytesToPad : OUTPUT_BUFFER_SIZE );

	emit output( (const unsigned char*)m_outputBuffer, bytesToOutput );
	m_rawDataAlreadyStreamed += bytesToOutput;
	emit percent( (int)(100.0 * ((double)m_rawDataAlreadyStreamed / (double)m_rawDataLengthToStream)) );
      }
      else {
	kdDebug() << "(K3bOggVorbisModule) successfully finished decoding file " << audioTrack()->absPath() << endl;
	// finished with success
	m_bDecodingInProgress = false;
	m_decodingTimer->stop();

	ov_clear( m_oggVorbisFile );

	emit percent( 100 );
	emit finished( true );
      }
    }

    else {
      // correct data
      // TODO: check if too much output

      emit output( (const unsigned char*)m_outputBuffer, bytesRead );
      m_rawDataAlreadyStreamed += bytesRead;
      emit percent( (int)(100.0 * ((double)m_rawDataAlreadyStreamed / (double)m_rawDataLengthToStream)) );
    }

    m_decodingTimer->stop();
  }
}


void K3bOggVorbisModule::slotConsumerReady()
{
  if( m_bDecodingInProgress )
    m_decodingTimer->start(0);
}


bool K3bOggVorbisModule::canDecode( const KURL& url )
{
  FILE* file = fopen( QFile::encodeName(url.path()), "r" );
  if( !file ) {
    kdDebug() << "(K3bOggVorbisModule) Could not open file " << url.path() << endl;
    return false;
  }

  OggVorbis_File of;

  if( ov_open( file, &of, 0, 0 ) ) {
    fclose( file );
    return false;
  }

  ov_clear( &of );

  return true;
}


void K3bOggVorbisModule::analyseTrack()
{
  // do some initialization
  FILE* file = fopen( QFile::encodeName(audioTrack()->absPath()), "r" );
  if( !file ) {
    kdDebug() << "(K3bOggVorbisModule) Could not open file " << audioTrack()->absPath() << endl;
    audioTrack()->setStatus( K3bAudioTrack::CORRUPT );
  }
  else {
    if( ov_open( file, m_oggVorbisFile, 0, 0 ) ) {
      kdDebug() << "(K3bOggVorbisModule) " << audioTrack()->absPath() << " seems to to be an ogg vorbis file." << endl;
      audioTrack()->setStatus( K3bAudioTrack::CORRUPT );
      fclose( file );
    }
    else {
      // check length of track
      double seconds = ov_time_total( m_oggVorbisFile, -1 );
      if( seconds == OV_EINVAL ) {
	kdDebug() << "(K3bOggVorbisModule) Could not determine length of file " << audioTrack()->absPath() << endl;
	audioTrack()->setStatus( K3bAudioTrack::CORRUPT );
      }
      else {
	audioTrack()->setLength( (unsigned long)ceil(seconds * 75.0) );
	audioTrack()->setStatus( K3bAudioTrack::OK );
      }

      // search for artist,title information
      vorbis_comment* vComment = ov_comment( m_oggVorbisFile, -1 );
      if( !vComment ) {
	kdDebug() << "(K3bOggVorbisModule) Could not open OggVorbis comment of file " << audioTrack()->absPath() << endl;
      }
      else {
	for( int i = 0; i < vComment->comments; ++i ) {
	  QString comment( vComment->user_comments[i] );
	  QStringList values = QStringList::split( "=", comment );
	  if( values.count() > 1 ) {
	    if( values[0] == "title" )
	      audioTrack()->setTitle( values[1] );
	    else if( values[0] == "artist" )
	      audioTrack()->setArtist( values[1] );
	    else if( values[0] == "album" )
	      audioTrack()->setAlbum( values[1] );
	  }
	}
      }
    }
  
    ov_clear( m_oggVorbisFile );
  }

  QTimer::singleShot( 0, this, SLOT(slotEmitTrackAnalysed()) );
}


void K3bOggVorbisModule::slotEmitTrackAnalysed()
{
  emit trackAnalysed( audioTrack() );
}


void K3bOggVorbisModule::stopAnalysingTrack()
{
  // do nothing since all is done syncronously
}


#include "k3boggvorbismodule.moc"

#endif
