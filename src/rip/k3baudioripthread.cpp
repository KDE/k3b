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
 

#include "k3baudioripthread.h"
#include "k3bpatternparser.h"
#include "k3bcdparanoialib.h"

#include <k3bjob.h>
#include <k3baudioencoder.h>
#include <k3bwavefilewriter.h>

#include <k3bdevice.h>
#include <k3btoc.h>
#include <k3btrack.h>

#include <songdb/k3bsong.h>
#include <songdb/k3bsongmanager.h>

#include <qfile.h>
#include <qtimer.h>

#include <kdebug.h>
#include <klocale.h>
#include <kstandarddirs.h>




class K3bAudioRipThread::Private
{
public:
  Private()
    : encoderFactory(0),
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

  K3bAudioEncoderFactory* encoderFactory;
  K3bAudioEncoder* encoder;
  K3bWaveFileWriter* waveFileWriter;

  K3bCdparanoiaLib* paranoiaLib;

  bool canceled;

  K3bCdDevice::Toc toc;

  QString fileType;
};


K3bAudioRipThread::K3bAudioRipThread()
  : QObject(),
    K3bThread(),
    m_device(0)
{
  d = new Private();
}


K3bAudioRipThread::~K3bAudioRipThread()
{
  delete d->encoder;
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


void K3bAudioRipThread::setEncoderFactory( K3bAudioEncoderFactory* f )
{
  d->encoderFactory = f;
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

  emitInfoMessage( i18n("Reading CD table of contents."), K3bJob::INFO );
  d->toc = m_device->readToc();

  if( !d->paranoiaLib->initParanoia( m_device ) ) {
    emitInfoMessage( i18n("Could not open device %1").arg(m_device->blockDeviceName()),
		     K3bJob::ERROR );
    emitFinished(false);
    return;
  }
  d->paranoiaLib->setParanoiaMode( d->paranoiaMode );
  d->paranoiaLib->setNeverSkip( d->neverSkip );
  d->paranoiaLib->setMaxRetries( d->paranoiaRetries );


  if( d->encoderFactory ) {
    delete d->encoder;
    d->encoder = (K3bAudioEncoder*)d->encoderFactory->createPlugin();
  }
  else if( !d->waveFileWriter ) {
    d->waveFileWriter = new K3bWaveFileWriter();
  }


  d->canceled = false;
  d->overallSectorsRead = 0;
  d->overallSectorsToRead = 0;
  for( unsigned int i = 0; i < m_tracks.count(); ++i ) 
    d->overallSectorsToRead += d->toc[m_tracks[i].first-1].length().lba();


  if( m_singleFile ) {
    QString& filename = m_tracks[0].second;

    QString dir = filename.left( filename.findRev("/") );
    if( !KStandardDirs::makeDir( dir ) ) {
      emitInfoMessage( i18n("Unable to create directory %1").arg(dir), K3bJob::ERROR );
      emitFinished(false);
      return;
    }

    // initialize
    bool isOpen = true;
    if( d->encoder ) {
      isOpen = d->encoder->openFile( d->fileType, filename );
      
      // here we use cd Title and Artist
      d->encoder->setMetaData( "Artist", m_cddbEntry.cdArtist );
      d->encoder->setMetaData( "Title", m_cddbEntry.cdTitle );
      d->encoder->setMetaData( "Comment", m_cddbEntry.cdExtInfo );
      d->encoder->setMetaData( "Year", QString::number(m_cddbEntry.year) );
    }
    else {
      isOpen = d->waveFileWriter->open( filename );
    }

    if( !isOpen ) {
      emitInfoMessage( i18n("Unable to open '%1' for writing.").arg(filename), K3bJob::ERROR );
      emitFinished(false);
      return;
    }

    emitInfoMessage( i18n("Ripping to single file '%1'.").arg(filename), K3bJob::INFO );
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

      if( !m_cddbEntry.cdArtist.isEmpty() &&
	  !m_cddbEntry.cdTitle.isEmpty() ) {
	kdDebug() << "(K3bAudioRipThread) creating new entry in SongDb." << endl;
	K3bSong* song = new K3bSong( filename.right( filename.length() - 1 - filename.findRev("/") ),
				     m_cddbEntry.cdTitle,
				     m_cddbEntry.cdArtist,
				     m_cddbEntry.cdTitle,
				     m_cddbEntry.discid,
				     0 );
	K3bSongManager::instance()->addSong( filename.left(filename.findRev("/")), song );
      }

      emitInfoMessage( i18n("Successfully ripped to %2.").arg(filename), K3bJob::INFO );
    }
  }

  if( d->canceled ) {
    emitCanceled();
    emitFinished(false);
  }
  else
    emitFinished(success);
}


bool K3bAudioRipThread::ripTrack( int track, const QString& filename )
{
  if( d->paranoiaLib->initReading( track ) ) {

    long trackSectorsRead = 0;

    QString dir = filename.left( filename.findRev("/") );
    if( !KStandardDirs::makeDir( dir ) ) {
      emitInfoMessage( i18n("Unable to create directory %1").arg(dir), K3bJob::ERROR );
      return false;
    }

    // initialize
    bool isOpen = true;
    if( !m_singleFile ) {
      if( d->encoder ) {
	isOpen = d->encoder->openFile( d->fileType, filename );
	
	d->encoder->setMetaData( "Artist", m_cddbEntry.artists[track-1] );
	d->encoder->setMetaData( "Title", m_cddbEntry.titles[track-1] );
	d->encoder->setMetaData( "Comment", m_cddbEntry.extInfos[track-1] );
	d->encoder->setMetaData( "Album", m_cddbEntry.cdTitle );
	d->encoder->setMetaData( "Year", QString::number(m_cddbEntry.year) );
      }
      else {
	isOpen = d->waveFileWriter->open( filename );
      }

      if( !isOpen ) {
	emitInfoMessage( i18n("Unable to open '%1' for writing.").arg(filename), K3bJob::ERROR );
	return false;
      }
    }

  if( !m_cddbEntry.artists[track-1].isEmpty() &&
      !m_cddbEntry.titles[track-1].isEmpty() )
    emitNewSubTask( i18n("Ripping track %1 (%2 - %3)").arg(track).arg(m_cddbEntry.artists[track-1]).arg(m_cddbEntry.titles[track-1]) );
  else
    emitNewSubTask( i18n("Ripping track %1").arg(track) );

    int status;
    while( 1 ) {
      if( d->canceled ) {
	cleanupAfterCancellation();
	return false;
      }
      
      Q_INT16* buf = d->paranoiaLib->read( &status );
      if( status == K3bCdparanoiaLib::S_OK ) {
	if( buf == 0 ) {
	  if( m_singleFile )
	    emitInfoMessage( i18n("Successfully ripped track %1.").arg(track), K3bJob::INFO );
	  else
	    emitInfoMessage( i18n("Successfully ripped track %1 to %2.").arg(track).arg(filename), K3bJob::INFO );

	  if( !m_singleFile ) {
	    if( d->encoder )
	      d->encoder->closeFile();
	    else
	      d->waveFileWriter->close();
	  }


	  if( !m_singleFile &&
	      !m_cddbEntry.artists[track-1].isEmpty() &&
	      !m_cddbEntry.titles[track-1].isEmpty() ) {
	    kdDebug() << "(K3bAudioRipThread) creating new entry in SongDb." << endl;
	    K3bSong* song = new K3bSong( filename.right( filename.length() - 1 - filename.findRev("/") ),
					 m_cddbEntry.cdTitle,
					 m_cddbEntry.artists[track-1],
					 m_cddbEntry.titles[track-1],
					 m_cddbEntry.discid,
					 track );
	    K3bSongManager::instance()->addSong( filename.left(filename.findRev("/")), song );
	  }

	  return true;
	}
	else {
	  if( d->encoder ) {
	    if( d->encoder->encode( reinterpret_cast<char*>(buf), 
				    CD_FRAMESIZE_RAW ) < 0 ) {
	      kdDebug() << "(K3bAudioRipThread) error while encoding." << endl;
	      emitInfoMessage( i18n("Error while encoding track %1.").arg(track), K3bJob::ERROR );
	      return false;
	    }
	  }
	  else
	    d->waveFileWriter->write( reinterpret_cast<char*>(buf), 
				      CD_FRAMESIZE_RAW, 
				      K3bWaveFileWriter::LittleEndian );

	  trackSectorsRead++;
	  d->overallSectorsRead++;
	  emitSubPercent( 100*trackSectorsRead/d->toc[track-1].length().lba() );
	  emitPercent( 100*d->overallSectorsRead/d->overallSectorsToRead );
	}
      }
      else {
	emitInfoMessage( i18n("Unrecoverable error while ripping track %1.").arg(track), K3bJob::ERROR );
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


// QString K3bAudioRipThread::createFileName( int track )
// {
//   QString extension = ".wav";

//   if( d->encoderFactory )
//     extension = "." + d->encoderFactory->extension();

//   if( m_baseDirectory[m_baseDirectory.length()-1] != '/' )
//     m_baseDirectory.append("/");

//   QString dir = m_baseDirectory;
//   QString filename;

//   if( m_bUsePattern ) {
//     dir += K3bPatternParser::parsePattern( m_cddbEntry, track, 
// 					   m_dirPattern,
// 					   m_replaceBlanksInDir,
// 					   m_dirReplaceString );
//   }

//   if( dir[dir.length()-1] != '/' )
//     dir.append("/");

//   if( m_bUsePattern ) {
//     filename = K3bPatternParser::parsePattern( m_cddbEntry, track, 
// 					       m_filenamePattern,
// 					       m_replaceBlanksInFilename,
// 					       m_filenameReplaceString ) + extension;
//   }
//   else {
//     filename = i18n("Track%1").arg( QString::number(track).rightJustify( 2, '0' ) ) + extension;
//   }

//   return dir + filename;
// }


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
      emitInfoMessage( i18n("Removed partial file '%1'.").arg(m_tracks[d->currentTrackIndex].second), K3bJob::INFO );
    }
  }
}



QString K3bAudioRipThread::jobDescription() const 
{
  if( m_cddbEntry.cdTitle.isEmpty() )
    return i18n("Ripping audio tracks");
  else
    return i18n("Ripping audio tracks from %1").arg(m_cddbEntry.cdTitle);
}

QString K3bAudioRipThread::jobDetails() const 
{
  if( d->encoderFactory )
    return i18n("1 track (encoding to %1)", 
		"%n tracks (encoding to %1)", 
		m_tracks.count() ).arg(d->encoderFactory->fileTypeComment(d->fileType));
  else
    return i18n("1 track", "%n tracks", m_tracks.count() );
}

#include "k3baudioripthread.moc"
