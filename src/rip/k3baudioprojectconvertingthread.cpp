/*
 *
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
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
 

#include "k3baudioprojectconvertingthread.h"
#include "k3bpatternparser.h"

#include <k3bjob.h>
#include <k3baudiodoc.h>
#include <k3baudiotrack.h>
#include <k3baudioencoder.h>
#include <k3bwavefilewriter.h>
#include "k3bcuefilewriter.h"

#include <k3bglobals.h>

#include <qfile.h>
#include <qtimer.h>

#include <kdebug.h>
#include <klocale.h>
#include <kstandarddirs.h>




class K3bAudioProjectConvertingThread::Private
{
public:
  Private()
    : encoder(0),
      waveFileWriter(0),
      canceled(false) {
  }

  // the index of the currently ripped track in m_tracks
  int currentTrackIndex;
  long long overallBytesRead;
  long long overallBytesToRead;

  K3bAudioEncoder* encoder;
  K3bWaveFileWriter* waveFileWriter;

  bool canceled;

  QString fileType;
};


K3bAudioProjectConvertingThread::K3bAudioProjectConvertingThread( K3bAudioDoc* doc )
  : K3bThread(),
    m_doc(doc)
{
  d = new Private();
}


K3bAudioProjectConvertingThread::~K3bAudioProjectConvertingThread()
{
  delete d->waveFileWriter;
  delete d;
}


void K3bAudioProjectConvertingThread::setFileType( const QString& t )
{
  d->fileType = t;
}


void K3bAudioProjectConvertingThread::setEncoder( K3bAudioEncoder* f )
{
  d->encoder = f;
}


void K3bAudioProjectConvertingThread::init()
{
  d->canceled = false;
}


void K3bAudioProjectConvertingThread::run()
{
  emitStarted();
  emitNewTask( i18n("Converting Audio Tracks")  );

  if( !d->encoder )
    if( !d->waveFileWriter )
      d->waveFileWriter = new K3bWaveFileWriter();
    

  d->overallBytesRead = 0;
  d->overallBytesToRead = m_doc->length().audioBytes();

  if( m_singleFile ) {
    QString& filename = m_tracks[0].second;

    QString dir = filename.left( filename.findRev("/") );
    if( !KStandardDirs::makeDir( dir ) ) {
      emitInfoMessage( i18n("Unable to create directory %1",dir), K3bJob::ERROR );
      emitFinished(false);
      return;
    }

    // initialize
    bool isOpen = true;
    if( d->encoder ) {
      if( isOpen = d->encoder->openFile( d->fileType, filename, m_doc->length() ) ) {
	// here we use cd Title and Artist
	d->encoder->setMetaData( K3bAudioEncoder::META_TRACK_ARTIST, m_cddbEntry.cdArtist );
	d->encoder->setMetaData( K3bAudioEncoder::META_TRACK_TITLE, m_cddbEntry.cdTitle );
	d->encoder->setMetaData( K3bAudioEncoder::META_TRACK_COMMENT, m_cddbEntry.cdExtInfo );
	d->encoder->setMetaData( K3bAudioEncoder::META_ALBUM_ARTIST, m_cddbEntry.cdArtist );
	d->encoder->setMetaData( K3bAudioEncoder::META_ALBUM_TITLE, m_cddbEntry.cdTitle );
	d->encoder->setMetaData( K3bAudioEncoder::META_ALBUM_COMMENT, m_cddbEntry.cdExtInfo );
	d->encoder->setMetaData( K3bAudioEncoder::META_YEAR, QString::number(m_cddbEntry.year) );
	d->encoder->setMetaData( K3bAudioEncoder::META_GENRE, m_cddbEntry.genre );
      }
      else
	emitInfoMessage( d->encoder->lastErrorString(), K3bJob::ERROR );
    }
    else {
      isOpen = d->waveFileWriter->open( filename );
    }

    if( !isOpen ) {
      emitInfoMessage( i18n("Unable to open '%1' for writing.",filename), K3bJob::ERROR );
      emitFinished(false);
      return;
    }

    emitInfoMessage( i18n("Converting to single file '%1'.",filename), K3bJob::INFO );
  }

  bool success = true;
  K3bAudioTrack* track = m_doc->firstTrack();
  unsigned int i = 0;
  while( track ) {
    d->currentTrackIndex = i;
    if( !convertTrack( track, m_singleFile ? m_tracks[0].second : m_tracks[i].second ) ) {
      success = false;
      break;
    }

    emitInfoMessage( i18n("Successfully converted track %1.",QString::number(i+1)), K3bJob::INFO );

    track = track->next();
    ++i;
  }

  if( m_singleFile ) {
    if( d->encoder )
      d->encoder->closeFile();
    else
      d->waveFileWriter->close();
  }
  
  if( !d->canceled && success && m_writePlaylist ) {
    success = success && writePlaylist();
  }

  if( !d->canceled && success && m_writeCueFile && m_singleFile ) {
    success = success && writeCueFile();
  }

  if( d->canceled ) {
    if( d->currentTrackIndex >= 0 && d->currentTrackIndex < (int)m_tracks.count() ) {
      if( QFile::exists( m_tracks[d->currentTrackIndex].second ) ) {
	QFile::remove( m_tracks[d->currentTrackIndex].second );
	emitInfoMessage( i18n("Removed partial file '%1'.",m_tracks[d->currentTrackIndex].second), K3bJob::INFO );
      }
    }

    emitCanceled();
    emitFinished(false);
  }
  else
    emitFinished(success);
}


bool K3bAudioProjectConvertingThread::convertTrack( K3bAudioTrack* track, const QString& filename )
{
  QString dir = filename.left( filename.findRev("/") );
  if( !KStandardDirs::makeDir( dir ) ) {
    emitInfoMessage( i18n("Unable to create directory %1",dir), K3bJob::ERROR );
    return false;
  }

  // initialize
  bool isOpen = true;
  if( !m_singleFile ) {
    if( d->encoder ) {
      if( isOpen = d->encoder->openFile( d->fileType, 
					 filename, 
					 track->length() ) ) {
	
	d->encoder->setMetaData( K3bAudioEncoder::META_TRACK_ARTIST, m_cddbEntry.artists[d->currentTrackIndex] );
	d->encoder->setMetaData( K3bAudioEncoder::META_TRACK_TITLE, m_cddbEntry.titles[d->currentTrackIndex] );
	d->encoder->setMetaData( K3bAudioEncoder::META_TRACK_COMMENT, m_cddbEntry.extInfos[d->currentTrackIndex] );
	d->encoder->setMetaData( K3bAudioEncoder::META_TRACK_NUMBER, QString::number(d->currentTrackIndex+1).rightJustified( 2, '0' ) );
	d->encoder->setMetaData( K3bAudioEncoder::META_ALBUM_ARTIST, m_cddbEntry.cdArtist );
	d->encoder->setMetaData( K3bAudioEncoder::META_ALBUM_TITLE, m_cddbEntry.cdTitle );
	d->encoder->setMetaData( K3bAudioEncoder::META_ALBUM_COMMENT, m_cddbEntry.cdExtInfo );
	d->encoder->setMetaData( K3bAudioEncoder::META_YEAR, QString::number(m_cddbEntry.year) );
	d->encoder->setMetaData( K3bAudioEncoder::META_GENRE, m_cddbEntry.genre );
      }
      else
	emitInfoMessage( d->encoder->lastErrorString(), K3bJob::ERROR );
    }
    else {
      isOpen = d->waveFileWriter->open( filename );
    }

    if( !isOpen ) {
      emitInfoMessage( i18n("Unable to open '%1' for writing.",filename), K3bJob::ERROR );
      return false;
    }
  }


  if( !m_cddbEntry.artists[d->currentTrackIndex].isEmpty() &&
      !m_cddbEntry.titles[d->currentTrackIndex].isEmpty() )
    emitNewSubTask( i18n("Converting track %1 (%2 - %3)")
		    .arg(d->currentTrackIndex+1)
		    .arg(m_cddbEntry.artists[d->currentTrackIndex])
		    .arg(m_cddbEntry.titles[d->currentTrackIndex]) );
  else
    emitNewSubTask( i18n("Converting track %1",d->currentTrackIndex+1) );


  // do the conversion
  // ----------------------

  char buffer[10*1024];
  const int bufferLength = 10*1024;
  int readLength = 0;
  long long readFile = 0;
  track->seek(0);
  while( !d->canceled && ( readLength = track->read( buffer, bufferLength ) ) > 0 ) {

    if( d->encoder ) {
      // the tracks produce big endian samples
      // so we need to swap the bytes here
      char b;
      for( int i = 0; i < bufferLength-1; i+=2 ) {
	b = buffer[i];
	buffer[i] = buffer[i+1];
	buffer[i+1] = b;
      }

      if( d->encoder->encode( buffer, readLength ) < 0 ) {
	kDebug() << "(K3bAudioProjectConvertingThread) error while encoding.";
	emitInfoMessage( d->encoder->lastErrorString(), K3bJob::ERROR );
	emitInfoMessage( i18n("Error while encoding track %1.",d->currentTrackIndex+1), K3bJob::ERROR );
	return false;
      }
    }
    else {
      d->waveFileWriter->write( buffer, 
				readLength, 
				K3bWaveFileWriter::BigEndian );
    }

    d->overallBytesRead += readLength;
    readFile += readLength;
    emitSubPercent( 100*readFile/track->size() );
    emitPercent( 100*d->overallBytesRead/d->overallBytesToRead );
  }

  if( !m_singleFile ) {
    if( d->encoder )
      d->encoder->closeFile();
    else
      d->waveFileWriter->close();
  }

  return ( readLength == 0 );
}


void K3bAudioProjectConvertingThread::cancel()
{
  d->canceled = true;
}


bool K3bAudioProjectConvertingThread::writePlaylist()
{
  // this is an absolut path so there is always a "/"
  QString playlistDir = m_playlistFilename.left( m_playlistFilename.findRev( "/" ) );

  if( !KStandardDirs::makeDir( playlistDir ) ) {
    emitInfoMessage( i18n("Unable to create directory %1",playlistDir), K3bJob::ERROR );
    return false;
  }

  emitInfoMessage( i18n("Writing playlist to %1.", m_playlistFilename ), K3bJob::INFO );

  QFile f( m_playlistFilename );
  if( f.open( QIODevice::WriteOnly ) ) {
    QTextStream t( &f );

    // format descriptor
    t << "#EXTM3U" << endl;

    // now write the entries (or the entry if m_singleFile)
    if( m_singleFile ) {
      // extra info
      t << "#EXTINF:" << m_doc->length().lba() << ",";
      if( !m_cddbEntry.cdArtist.isEmpty() && !m_cddbEntry.cdTitle.isEmpty() )
	t << m_cddbEntry.cdArtist << " - " << m_cddbEntry.cdTitle << endl;
      else
	t << m_tracks[0].second.mid(m_tracks[0].second.findRev("/") + 1, 
				    m_tracks[0].second.length() - m_tracks[0].second.findRev("/") - 5)
	  << endl; // filename without extension

      // filename
      if( m_relativePathInPlaylist )
	t << findRelativePath( m_tracks[0].second, playlistDir )
	  << endl;
      else
	t << m_tracks[0].second << endl;
    }
    else {
      for( unsigned int i = 0; i < m_tracks.count(); ++i ) {
	int trackIndex = m_tracks[i].first-1;

	// extra info
	t << "#EXTINF:" << m_doc->length().totalFrames()/75 << ",";

	if( !m_cddbEntry.artists[trackIndex].isEmpty() && !m_cddbEntry.titles[trackIndex].isEmpty() )
	  t << m_cddbEntry.artists[trackIndex] << " - " << m_cddbEntry.titles[trackIndex] << endl;
	else
	  t << m_tracks[i].second.mid(m_tracks[i].second.findRev("/") + 1, 
				      m_tracks[i].second.length() 
				      - m_tracks[i].second.findRev("/") - 5)
	    << endl; // filename without extension

	// filename
	if( m_relativePathInPlaylist )
	  t << findRelativePath( m_tracks[i].second, playlistDir )				 
	    << endl;
	else
	  t << m_tracks[i].second << endl;
      }
    }

    return ( t.device()->status() == IO_Ok );
  }
  else {
    emitInfoMessage( i18n("Unable to open '%1' for writing.",m_playlistFilename), K3bJob::ERROR );
    kDebug() << "(K3bAudioProjectConvertingThread) could not open file " << m_playlistFilename << " for writing.";
    return false;
  }
}


bool K3bAudioProjectConvertingThread::writeCueFile()
{
  K3bCueFileWriter cueWriter;

  // create a new toc and cd-text
  K3bDevice::Toc toc;
  K3bDevice::CdText text;
  text.setPerformer( m_cddbEntry.cdArtist );
  text.setTitle( m_cddbEntry.cdTitle );
  text.reserve( m_tracks.count() );
  K3b::Msf currentSector;
  K3bAudioTrack* track = m_doc->firstTrack();
  int trackNum = 1;
  while( track ) {

    K3bDevice::Track newTrack( currentSector, (currentSector+=track->length()) - 1, K3bDevice::Track::AUDIO );
    toc.append( newTrack );

    K3bDevice::TrackCdText trackText;
    trackText.setPerformer( m_cddbEntry.artists[trackNum-1] );
    trackText.setTitle( m_cddbEntry.titles[trackNum-1] );
    text.append( trackText );

    track = track->next();
    ++trackNum;
  }

  cueWriter.setData( toc );
  cueWriter.setCdText( text );


  // we always use a relative filename here
  QString imageFile = m_tracks[0].second.section( '/', -1 );
  cueWriter.setImage( imageFile, ( d->fileType.isEmpty() ? QString("WAVE") : d->fileType ) );

  // use the same base name as the image file
  QString cueFile = m_tracks[0].second;
  cueFile.truncate( cueFile.findRev(".") );
  cueFile += ".cue";

  emitInfoMessage( i18n("Writing cue file to %1.",cueFile), K3bJob::INFO );

  return cueWriter.save( cueFile );
}


QString K3bAudioProjectConvertingThread::findRelativePath( const QString& absPath, const QString& baseDir )
{
  QString baseDir_ = K3b::prepareDir( K3b::fixupPath(baseDir) );
  QString path = K3b::fixupPath( absPath );

  // both paths have an equal beginning. That's just how it's configured by K3b
  int pos = baseDir_.find( "/" );
  int oldPos = pos;
  while( pos != -1 && path.left( pos+1 ) == baseDir_.left( pos+1 ) ) {
    oldPos = pos;
    pos = baseDir_.find( "/", pos+1 );
  }

  // now the paths are equal up to oldPos, so that's how "deep" we go
  path = path.mid( oldPos+1 );
  baseDir_ = baseDir_.mid( oldPos+1 );
  int numberOfDirs = baseDir_.count( '/' );
  for( int i = 0; i < numberOfDirs; ++i )
    path.prepend( "../" );

  return path;
}


QString K3bAudioProjectConvertingThread::jobDescription() const 
{
  if( m_cddbEntry.cdTitle.isEmpty() )
    return i18n("Converting Audio Tracks");
  else
    return i18n("Converting Audio Tracks From '%1'",m_cddbEntry.cdTitle);
}

QString K3bAudioProjectConvertingThread::jobDetails() const 
{
  if( d->encoder )
    return i18np("1 track (encoding to %2)", 
		"%1 tracks (encoding to %2)", 
		m_tracks.count() ,d->encoder->fileTypeComment(d->fileType));
  else
    return i18np("1 track", "%1 tracks", m_doc->numOfTracks() );
}

