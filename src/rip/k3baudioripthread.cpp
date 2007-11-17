/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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
 

#include "k3baudioripthread.h"
#include "k3bpatternparser.h"

#include <k3bcdparanoialib.h>
#include <k3bjob.h>
#include <k3baudioencoder.h>
#include <k3bwavefilewriter.h>
#include <k3bglobalsettings.h>
#include <k3bcore.h>
#include "k3bcuefilewriter.h"

#include <k3bdevice.h>
#include <k3btoc.h>
#include <k3btrack.h>
#include <k3bglobals.h>

#include <qfile.h>
#include <qfileinfo.h>
#include <qtimer.h>

#include <kdebug.h>
#include <klocale.h>
#include <kstandarddirs.h>




class K3bAudioRipThread::Private
{
public:
  Private()
    : paranoiaRetries(5),
      neverSkip(false),
      encoder(0),
      waveFileWriter(0),
      paranoiaLib(0),
      canceled(false) {
  }

  // the index of the currently ripped track in m_tracks
  int currentTrackIndex;
  long overallSectorsRead;
  long overallSectorsToRead;

  int paranoiaMode;
  int paranoiaRetries;
  int neverSkip;

  K3bAudioEncoder* encoder;
  K3bWaveFileWriter* waveFileWriter;

  K3bCdparanoiaLib* paranoiaLib;

  bool canceled;

  K3bDevice::Toc toc;

  QString fileType;
};


K3bAudioRipThread::K3bAudioRipThread()
  :  K3bThread(),
    m_device(0),
    m_useIndex0(false)
{
  d = new Private();
}


K3bAudioRipThread::~K3bAudioRipThread()
{
  delete d->waveFileWriter;
  delete d->paranoiaLib;
  delete d;
}


void K3bAudioRipThread::setFileType( const QString& t )
{
  d->fileType = t;
}


void K3bAudioRipThread::setParanoiaMode( int mode )
{
  d->paranoiaMode = mode;
}


void K3bAudioRipThread::setMaxRetries( int r )
{
  d->paranoiaRetries = r;
}


void K3bAudioRipThread::setNeverSkip( bool b )
{
  d->neverSkip = b;
}


void K3bAudioRipThread::setEncoder( K3bAudioEncoder* f )
{
  d->encoder = f;
}


void K3bAudioRipThread::run()
{
  emitStarted();
  emitNewTask( i18n("Extracting Digital Audio")  );

  if( !d->paranoiaLib ) {
    d->paranoiaLib = K3bCdparanoiaLib::create();
  }

  if( !d->paranoiaLib ) {
    emitInfoMessage( i18n("Could not load libcdparanoia."), K3bJob::ERROR );
    emitFinished(false);
    return;
  }

  // try to open the device
  if( !m_device ) {
    emitFinished(false);
    return;
  }

  m_device->block(true);

  emitInfoMessage( i18n("Reading CD table of contents."), K3bJob::INFO );
  d->toc = m_device->readToc();

  if( !d->paranoiaLib->initParanoia( m_device, d->toc ) ) {
    emitInfoMessage( i18n("Could not open device %1",m_device->blockDeviceName()),
		     K3bJob::ERROR );
    m_device->block(false);

    // check if we have write access to the generic device
    if( m_device->interfaceType() == K3bDevice::SCSI &&
	!m_device->genericDevice().isEmpty() &&
	!QFileInfo( m_device->genericDevice() ).isWritable() )
      emitInfoMessage( i18n("You need write access to %1", m_device->genericDevice() ), K3bJob::ERROR );
    
    emitFinished(false);
    return;
  }
  d->paranoiaLib->setParanoiaMode( d->paranoiaMode );
  d->paranoiaLib->setNeverSkip( d->neverSkip );
  d->paranoiaLib->setMaxRetries( d->paranoiaRetries );


  if( !d->encoder )
    if( !d->waveFileWriter ) {
      d->waveFileWriter = new K3bWaveFileWriter();
    }


  if( m_useIndex0 ) {
    emitNewSubTask( i18n("Searching index 0 for all tracks") );
    m_device->indexScan( d->toc );
  }


  d->canceled = false;
  d->overallSectorsRead = 0;
  d->overallSectorsToRead = 0;
  for( unsigned int i = 0; i < m_tracks.count(); ++i ) {
    if( m_useIndex0 )
      d->overallSectorsToRead += d->toc[m_tracks[i].first-1].realAudioLength().lba();
    else
      d->overallSectorsToRead += d->toc[m_tracks[i].first-1].length().lba();
  }


  if( m_singleFile ) {
    QString& filename = m_tracks[0].second;

    QString dir = filename.left( filename.findRev("/") );
    if( !KStandardDirs::makeDir( dir, 0777 ) ) {
      d->paranoiaLib->close();
      emitInfoMessage( i18n("Unable to create directory %1",dir), K3bJob::ERROR );
      m_device->block(false);
      emitFinished(false);
      return;
    }

    // initialize
    bool isOpen = true;
    if( d->encoder ) {
      if( isOpen = d->encoder->openFile( d->fileType, filename, d->overallSectorsToRead ) ) {
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
      d->paranoiaLib->close();
      emitInfoMessage( i18n("Unable to open '%1' for writing.",filename), K3bJob::ERROR );
      m_device->block(false);
      emitFinished(false);
      return;
    }

    emitInfoMessage( i18n("Ripping to single file '%1'.",filename), K3bJob::INFO );
  }

  emitInfoMessage( i18n("Starting digital audio extraction (ripping)."), K3bJob::INFO );

  bool success = true;
  for( unsigned int i = 0; i < m_tracks.count(); ++i ) {
    d->currentTrackIndex = i;
    if( !ripTrack( m_tracks[i].first, m_singleFile ? m_tracks[0].second : m_tracks[i].second ) ) {
      success = false;
      break;
    }
  }

  if( m_singleFile ) {
    if( d->encoder )
      d->encoder->closeFile();
    else
      d->waveFileWriter->close();

    if( success && !d->canceled ) {
      QString& filename = m_tracks[0].second;
      emitInfoMessage( i18n("Successfully ripped to %2.",filename), K3bJob::INFO );
    }
  }

  if( !d->canceled && m_writePlaylist ) {
    success = success && writePlaylist();
  }

  if( !d->canceled && m_writeCueFile && m_singleFile ) {
    if( !m_useIndex0 ) {
      emitNewSubTask( i18n("Searching index 0 for all tracks") );
      m_device->indexScan( d->toc );
    }
    success = success && writeCueFile();
  }

  d->paranoiaLib->close();
  m_device->block(false);

  if( d->canceled ) {
    emitCanceled();
    emitFinished(false);
  }
  else {
    if( k3bcore->globalSettings()->ejectMedia() )
      m_device->eject();

    emitFinished(success);
  }
}


bool K3bAudioRipThread::ripTrack( int track, const QString& filename )
{
  const K3bTrack& tt = d->toc[track-1];

  long endSec = ( (m_useIndex0 && tt.index0() > 0) 
		  ? tt.firstSector().lba() + tt.index0().lba() - 1
		  : tt.lastSector().lba() );
  
  if( d->paranoiaLib->initReading( tt.firstSector().lba(), endSec ) ) {

    long trackSectorsRead = 0;

    QString dir = filename.left( filename.findRev("/") );
    if( !KStandardDirs::makeDir( dir, 0777 ) ) {
      emitInfoMessage( i18n("Unable to create directory %1",dir), K3bJob::ERROR );
      return false;
    }

    // initialize
    bool isOpen = true;
    if( !m_singleFile ) {
      if( d->encoder ) {
	if( isOpen = d->encoder->openFile( d->fileType, 
					   filename, 
					   m_useIndex0 ? d->toc[track-1].realAudioLength() : d->toc[track-1].length() ) ) {
	
	  d->encoder->setMetaData( K3bAudioEncoder::META_TRACK_ARTIST, m_cddbEntry.artists[track-1] );
	  d->encoder->setMetaData( K3bAudioEncoder::META_TRACK_TITLE, m_cddbEntry.titles[track-1] );
	  d->encoder->setMetaData( K3bAudioEncoder::META_TRACK_COMMENT, m_cddbEntry.extInfos[track-1] );
	  d->encoder->setMetaData( K3bAudioEncoder::META_TRACK_NUMBER, QString::number(track).rightJustified( 2, '0' ) );
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

  if( !m_cddbEntry.artists[track-1].isEmpty() &&
      !m_cddbEntry.titles[track-1].isEmpty() )
    emitNewSubTask( i18n("Ripping track %1 (%2 - %3)",track,m_cddbEntry.artists[track-1],m_cddbEntry.titles[track-1]) );
  else
    emitNewSubTask( i18n("Ripping track %1").arg(track) );

    int status;
    while( 1 ) {
      if( d->canceled ) {
	cleanupAfterCancellation();
	return false;
      }
      
      char* buf = d->paranoiaLib->read( &status );
      if( status == K3bCdparanoiaLib::S_OK ) {
	if( buf == 0 ) {
	  if( m_singleFile )
	    emitInfoMessage( i18n("Successfully ripped track %1.",track), K3bJob::INFO );
	  else
	    emitInfoMessage( i18n("Successfully ripped track %1 to %2.",track,filename), K3bJob::INFO );

	  if( !m_singleFile ) {
	    if( d->encoder )
	      d->encoder->closeFile();
	    else
	      d->waveFileWriter->close();
	  }

	  return true;
	}
	else {
	  if( d->encoder ) {
	    if( d->encoder->encode( buf, 
				    CD_FRAMESIZE_RAW ) < 0 ) {
	      kDebug() << "(K3bAudioRipThread) error while encoding.";
	      emitInfoMessage( d->encoder->lastErrorString(), K3bJob::ERROR );
	      emitInfoMessage( i18n("Error while encoding track %1.",track), K3bJob::ERROR );
	      return false;
	    }
	  }
	  else
	    d->waveFileWriter->write( buf, 
				      CD_FRAMESIZE_RAW, 
				      K3bWaveFileWriter::LittleEndian );

	  trackSectorsRead++;
	  d->overallSectorsRead++;
	  emitSubPercent( 100*trackSectorsRead/d->toc[track-1].length().lba() );
	  emitPercent( 100*d->overallSectorsRead/d->overallSectorsToRead );
	}
      }
      else {
	emitInfoMessage( i18n("Unrecoverable error while ripping track %1.",track), K3bJob::ERROR );
	return false;
      }
    }
    return true;
  }
  else {
    emitInfoMessage( i18n("Error while initializing audio ripping."), K3bJob::ERROR );
    return false;
  }
}


void K3bAudioRipThread::cancel()
{
  d->canceled = true;

  // what if paranoia is stuck in paranoia_read?
  // we need to terminate in that case
  // wait for 1 second. I the thread still is working terminate it
  // and trigger the finished slot manually
  emitInfoMessage( i18n("Cancellation could take a while..."), K3bJob::INFO );
  QTimer::singleShot( 1000, this, SLOT(slotCheckIfThreadStillRunning()) );
}


void K3bAudioRipThread::slotCheckIfThreadStillRunning()
{
  if( running() ) {
    d->paranoiaLib->close();
    m_device->block(false);

    // this could happen if the thread is stuck in paranoia_read
    // because of an unreadable cd
    terminate();
    cleanupAfterCancellation();
    emitCanceled();
    emitFinished(false);
  }
}


// this needs to be called if the thread was killed due to a hung paranoia_read
void K3bAudioRipThread::cleanupAfterCancellation()
{
  if( d->currentTrackIndex >= 0 && d->currentTrackIndex < (int)m_tracks.count() ) {
    if( QFile::exists( m_tracks[d->currentTrackIndex].second ) ) {
      QFile::remove( m_tracks[d->currentTrackIndex].second );
      emitInfoMessage( i18n("Removed partial file '%1'.",m_tracks[d->currentTrackIndex].second), K3bJob::INFO );
    }
  }
}


bool K3bAudioRipThread::writePlaylist()
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
    Q3TextStream t( &f );

    // format descriptor
    t << "#EXTM3U" << endl;

    // now write the entries (or the entry if m_singleFile)
    if( m_singleFile ) {
      // extra info
      t << "#EXTINF:" << d->overallSectorsToRead/75 << ",";
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
	t << "#EXTINF:" << d->toc[trackIndex].length().totalFrames()/75 << ",";

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
    kDebug() << "(K3bAudioRipThread) could not open file " << m_playlistFilename << " for writing.";
    return false;
  }
}


bool K3bAudioRipThread::writeCueFile()
{
  K3bCueFileWriter cueWriter;

  // create a new toc and cd-text
  K3bDevice::Toc toc;
  K3bDevice::CdText text;
  text.setPerformer( m_cddbEntry.cdArtist );
  text.setTitle( m_cddbEntry.cdTitle );
  text.reserve( m_tracks.count() );
  K3b::Msf currentSector;
  for( unsigned int i = 0; i < m_tracks.count(); ++i ) {
    int trackNum = m_tracks[i].first;

    const K3bDevice::Track& oldTrack = d->toc[trackNum-1];
    K3bDevice::Track newTrack( oldTrack );
    newTrack.setFirstSector( currentSector );
    newTrack.setLastSector( (currentSector+=oldTrack.length()) - 1 );
    toc.append( newTrack );

    K3bDevice::TrackCdText trackText;
    trackText.setPerformer( m_cddbEntry.artists[trackNum-1] );
    trackText.setTitle( m_cddbEntry.titles[trackNum-1] );
    text.append( trackText );
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


QString K3bAudioRipThread::findRelativePath( const QString& absPath, const QString& baseDir )
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


QString K3bAudioRipThread::jobDescription() const 
{
  if( m_cddbEntry.cdTitle.isEmpty() )
    return i18n("Ripping Audio Tracks");
  else
    return i18n("Ripping Audio Tracks From '%1'",m_cddbEntry.cdTitle);
}

QString K3bAudioRipThread::jobDetails() const 
{
  if( d->encoder )
    return i18np("1 track (encoding to %1)", 
		"%n tracks (encoding to %1)", 
		m_tracks.count() ,d->encoder->fileTypeComment(d->fileType));
  else
    return i18np("1 track", "%n tracks", m_tracks.count() );
}

#include "k3baudioripthread.moc"
