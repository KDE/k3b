/* 
 *
 * $Id: sourceheader 380067 2005-01-19 13:03:46Z trueg $
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bimagecache.h"
#include "k3bimagesource.h"

#include <k3btoc.h>
#include <k3btrack.h>
#include <k3bglobals.h>
#include <k3bwavefilewriter.h>

#include <qfile.h>
#include <qfileinfo.h>
#include <qcstring.h>
#include <qdir.h>
#include <qtimer.h>

#include <klocale.h>
#include <kstandarddirs.h>
#include <kdebug.h>
#include <kio/netaccess.h>

#include <unistd.h>


class K3bImageCache::Private
{
public:
  QString cachePath;
  bool canceled;

  K3bDevice::Toc cachedToc;

  QFile outFile;
  K3bWaveFileWriter* waveFileWriter;

  KIO::filesize_t writtenTrackBytes;
  KIO::filesize_t writtenTocBytes;
  unsigned int currentTrack;
  QByteArray buffer;

  int lastTrackProgress;
  int lastTocProgress;

  QString createdTempFolder;
};



K3bImageCache::K3bImageCache( K3bJobHandler* hdl, QObject* parent )
  : K3bJob( hdl, parent )
{
  d = new Private;
  d->waveFileWriter = 0;
}


K3bImageCache::~K3bImageCache()
{
  delete d->waveFileWriter;
  delete d;
}


void K3bImageCache::start()
{
  kdDebug() << k_funcinfo << endl;

  jobStarted();

  d->canceled = false;
  d->writtenTocBytes = 0;
  d->writtenTrackBytes = 0;
  d->currentTrack = 1;
  d->lastTocProgress = 0;

  // connect source
  source()->disconnect( this );
  connect( source(), SIGNAL(tocReady(bool)), SLOT(slotTocReady(bool)) );
  connect( source(), SIGNAL(infoMessage(const QString&, int)), SIGNAL(infoMessage(const QString&, int)) );

  // FIXME: emit some newTask
  emit newTask( "Doing stuff with the toc (I am a temp message, REPLACEME)" );

  // we start by determining the toc
  source()->determineToc();
}


void K3bImageCache::slotTocReady( bool success )
{
  kdDebug() << k_funcinfo << endl;

  if( success ) {
    if( initCachePath() ) {
      
      initBuffer( 1 );
      
      if( openImageFile( 1 ) ) {
	source()->writeToFd( -1/*fdIn()*/ );
	source()->start();

	// source->start() may already result in a failure
	if( active() )
	  QTimer::singleShot( 0, this, SLOT(slotReadAsync()) );
      }
      else
	jobFinished( false );
    }
    else
      jobFinished( false );
  }
  else {
    // FIXME: should the source emit a proper error message or we?
    jobFinished( false );
  }
}


void K3bImageCache::cancel()
{
  if( active() ) {
    d->canceled = true;

    source()->cancel();

    cleanup();

    emit canceled();
    jobFinished( false );
  }
}


bool K3bImageCache::initCachePath()
{
  kdDebug() << k_funcinfo << endl;

  d->cachedToc = source()->toc();

  // let's see if we need a file or a folder
  // in case of a single track we allow a filename to be specified
  bool folder = ( d->cachedToc.count() > 1 );

  // this will become the folder in which we create the image files
  QString folderToCreate = d->cachePath;

  QFileInfo f( d->cachePath );
  if( f.isDir() ) {
    // if we need a dir and this one is empty we simply use it
    if( folder && QDir( f.absFilePath() ).count() > 2 ) { // "." and ".."
      folderToCreate = d->cachePath = K3b::findUniqueFilePrefix( "k3bimage", f.absFilePath() );
    }

    // if we need a file we simply create one in this dir
    else if( !folder )
      d->cachePath = createCacheFilename( 1 );
  }

  // if we want a file and have a file we simply overwrite it
  else if( f.isFile() && folder ) {
    emit infoMessage( i18n("Unable to create folder '%1'").arg( d->cachePath ), ERROR );
    emit infoMessage( i18n("A file is in the way."), ERROR );
    return false;
  }

  // The only case left is if the path does not exist
  else if( !f.exists() ) {
    bool looksLikeFilename = d->cachePath.section( '/', -1 ).contains( "." );

    // if the thing looks like a filename we don't use it as a folder
    if( looksLikeFilename )
      folderToCreate.truncate( folderToCreate.findRev( "/" ) );
      
    if( folder )
      d->cachePath = folderToCreate;
    else if( !looksLikeFilename )
      d->cachePath = createCacheFilename( 1 );
  }

  if( !KStandardDirs::makeDir( folderToCreate ) ) {
    emit infoMessage( i18n("Unable to create folder '%1'").arg( folderToCreate ), ERROR );
    return false;
  }
  else
    d->createdTempFolder = folderToCreate;


  // test available space
  QString pathToTest = d->cachePath;
  if( !folder )
    pathToTest = pathToTest.section( '/', 0, -2 );
  unsigned long avail, size;
  if( !K3b::kbFreeOnFs( pathToTest, size, avail ) ) {
    emit infoMessage( i18n("Unable to determine free space in folder '%1'.").arg(pathToTest), ERROR );
    return false;
  }
  else {
    if( avail < K3bImageSource::tocSize( d->cachedToc )/1024 ) {
      emit infoMessage( i18n("Not enough space left in folder '%1'.").arg(pathToTest), ERROR );
      return false;
    }
  }

  return true;
}


QString K3bImageCache::createCacheFilename( unsigned int track ) const
{
  kdDebug() << k_funcinfo << endl;

  if( track <= d->cachedToc.count() ) {
    QString name( d->cachePath );
    if( !name.endsWith( "/" ) )
      name += "/";
    name += "track";
    if( track < 10 )
      name += "0";
    name += QString::number( track );

    // extension base on the track type
    if( d->cachedToc[track-1].type() == K3bDevice::Track::AUDIO )
      name += ".wav";
    else if( d->cachedToc[track-1].mode() == K3bDevice::Track::MODE1 )
      name += ".iso";
    else
      name += ".bin";

    return name;
  }

  else
    return QString::null;
}


void K3bImageCache::initBuffer( unsigned int track )
{
  kdDebug() << k_funcinfo << endl;

  int secSize = 2048;

  if( track <= d->cachedToc.count() ) {
    // we always create a buffer which holds 10 sectors
    if( d->cachedToc[track-1].type() == K3bDevice::Track::AUDIO )
      secSize = 2352;
    else if ( d->cachedToc[track-1].mode() == K3bDevice::Track::XA_FORM1 )
      secSize = 2056;
    else if ( d->cachedToc[track-1].mode() == K3bDevice::Track::XA_FORM2 )
      secSize = 2332;
  }

  d->buffer.resize( 10*secSize );
}


bool K3bImageCache::openImageFile( unsigned int track )
{
  kdDebug() << k_funcinfo << endl;

  d->writtenTrackBytes = 0;
  d->lastTrackProgress = 0;

  bool isOpen = false;
  QString filename = d->cachePath;
  if( d->cachedToc.count() > 1 )
    filename = createCacheFilename( track );
  if( d->cachedToc[track-1].type() == K3bDevice::Track::AUDIO ) {
    // the 44 bytes wave header is part of the image size but is created by the
    // wave filewriter. that's why we have to add it here.
    d->writtenTrackBytes = 44;
    d->writtenTocBytes += 44;

    if( !d->waveFileWriter )
      d->waveFileWriter = new K3bWaveFileWriter();

    isOpen = d->waveFileWriter->open( filename );
  }
  else {
    d->outFile.setName( filename );
    isOpen = d->outFile.open( IO_WriteOnly );
  }

  if( !isOpen ) {
    emit infoMessage( i18n("Unable to open file '%1' for writing.").arg(filename), ERROR );
    return false;
  }
  else {
    emit nextTrack( track, d->cachedToc.count() );
    // FIXME: emit some subTask instead of the infoMessage
    emit infoMessage( i18n("Caching track %1 to '%2'.").arg(track).arg(filename), INFO );

    return true;
  }
}


void K3bImageCache::setCachePath( const QString& path )
{
  d->cachePath = path;
}


void K3bImageCache::slotReadAsync()
{
  if( d->canceled || !active() )
    return;

  int maxSize = d->buffer.size();
  if( d->writtenTrackBytes + maxSize > K3bImageSource::trackSize( d->cachedToc[d->currentTrack-1] ) )
    maxSize = K3bImageSource::trackSize( d->cachedToc[d->currentTrack-1] ) - d->writtenTrackBytes;

  long read = source()->read( d->buffer.data(), maxSize );

  if( read < 0 ) {
    jobFinished( false );
    return;
  }

  if( d->cachedToc[d->currentTrack-1].type() == K3bDevice::Track::AUDIO )
    d->waveFileWriter->write( d->buffer.data(), 
			      read, 
			      K3bWaveFileWriter::BigEndian );
  else
    d->outFile.writeBlock( d->buffer.data(), read );

  d->writtenTrackBytes += read;
  d->writtenTocBytes += read;

  // calculate progress
  int trackProgress = 100*d->writtenTrackBytes/K3bImageSource::trackSize( d->cachedToc[d->currentTrack-1] );
  int tocProgress = 100*d->writtenTocBytes/K3bImageSource::tocSize( d->cachedToc );
  if( trackProgress > d->lastTrackProgress ) {
    emit subPercent( trackProgress );
    d->lastTrackProgress = trackProgress;
  }
  if( tocProgress > d->lastTocProgress ) {
    emit percent( tocProgress );
    d->lastTocProgress = tocProgress;
  }

  // FIXME: processes size signals

  if( d->writtenTrackBytes == K3bImageSource::trackSize( d->cachedToc[d->currentTrack-1] ) ) {
    d->outFile.close();
    if( d->waveFileWriter )
      d->waveFileWriter->close();

    d->currentTrack++;

    // do we have to open another track file?
    if( d->currentTrack <= d->cachedToc.count() ) {
      if( !openImageFile( d->currentTrack ) ) {
	jobFinished( false );
	return;
      }
    }

    // finished!
    else {
      jobFinished( true );
      return;
    }
  }

  QTimer::singleShot( 0, this, SLOT(slotReadAsync()) );
}


void K3bImageCache::cleanup()
{
  // FIXME: maybe we should remember the names of the files we created
  //        to be sure not to overwrite files we did not create.

  bool filesRemoved = false;

  // remove all the temp files
  if( d->cachedToc.count() > 1 ) {
    for( unsigned int i = 0; i < d->cachedToc.count(); ++i ) {
      QString filename = createCacheFilename( i+1 );
      if( QFile::exists( filename ) ) {
	QFile::remove( filename );
	filesRemoved = true;
      }
    }

    if( !d->createdTempFolder.isEmpty() ) {
      KIO::NetAccess::del( KURL::fromPathOrURL(d->createdTempFolder), 0 );
      d->createdTempFolder.truncate(0);
      filesRemoved = true;
    }
  }
  else {
    if( QFile::exists( d->cachePath ) ) {
      QFile::remove( d->cachePath );
      filesRemoved = true;
    }
  }

  if( filesRemoved )
    emit infoMessage( i18n("Removed temporary files."), INFO );
}

#include "k3bimagecache.moc"
