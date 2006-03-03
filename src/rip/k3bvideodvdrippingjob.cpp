/* 
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 * Copyright (C) 2006 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2006 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bvideodvdrippingjob.h"

#include <k3bvideodvdtitletranscodingjob.h>
#include <k3bvideodvdtitledetectclippingjob.h>

#include <kdebug.h>
#include <klocale.h>


K3bVideoDVDRippingJob::TitleRipInfo::TitleRipInfo()
  : title(1),
    audioStream(0),
    width(0),
    height(0),
    clipTop(0),
    clipLeft(0),
    clipBottom(0),
    clipRight(0)
{
}


K3bVideoDVDRippingJob::TitleRipInfo::TitleRipInfo( int _title,
						   int _audioStream,
						   const QString& fn,
						   int _width,
						   int _height,
						   int _clipTop,
						   int _clipLeft,
						   int _clipBottom,
						   int _clipRight )
  : title(_title),
    audioStream(_audioStream),
    filename(fn),
    width(_width),
    height(_height),
    clipTop(_clipTop),
    clipLeft(_clipLeft),
    clipBottom(_clipBottom),
    clipRight(_clipRight)
{
}



class K3bVideoDVDRippingJob::Private {
public:
  Private()
    : autoClipping( true ) {
  }

  unsigned int currentTitleInfoIndex;
  bool autoClipping;

  bool canceled;
};



K3bVideoDVDRippingJob::K3bVideoDVDRippingJob( K3bJobHandler* hdl, QObject* parent )
  : K3bJob( hdl, parent )
{
  d = new Private();

  m_transcodingJob = new K3bVideoDVDTitleTranscodingJob( this, this );
  connectSubJob( m_transcodingJob,
		 SLOT(slotTranscodingJobFinished(bool)),
		 SIGNAL(newTask(const QString&)),
		 SIGNAL(newSubTask(const QString&)),
		 SLOT(slotTranscodingProgress(int)),
		 SIGNAL(subPercent(int)),
		 0,
		 0 );
  m_detectClippingJob = 0;
}


K3bVideoDVDRippingJob::~K3bVideoDVDRippingJob()
{
  delete d;
}


QString K3bVideoDVDRippingJob::jobDescription() const
{
  return i18n("Ripping Video DVD Titles");
}


QString K3bVideoDVDRippingJob::jobDetails() const
{
  return i18n("Transcoding %n title to %1/%2", "Transcoding %n titles to %1/%2", m_titleRipInfos.count() )
    .arg( K3bVideoDVDTitleTranscodingJob::videoCodecString( m_transcodingJob->videoCodec() ) )
    .arg( K3bVideoDVDTitleTranscodingJob::audioCodecString( m_transcodingJob->audioCodec() ) );
}


void K3bVideoDVDRippingJob::start()
{
  jobStarted();
  d->canceled = false;

  if( d->autoClipping )
    startTranscoding( 0 );
  else
    startDetectClipping( 0 );
}


void K3bVideoDVDRippingJob::slotTranscodingJobFinished( bool success )
{
  if( d->canceled ) {
    emit canceled();
    jobFinished( false );
  }
  else if( success ) {
    ++d->currentTitleInfoIndex ;
    if( d->currentTitleInfoIndex < m_titleRipInfos.count() ) {
      if( d->autoClipping )
	startDetectClipping( d->currentTitleInfoIndex );
      else
	startTranscoding( d->currentTitleInfoIndex );
    }
    else
      jobFinished( true );
  }
  else {
    // the transcoding job should say what went wrong
    jobFinished( false );
  }
}


void K3bVideoDVDRippingJob::slotDetectClippingJobFinished( bool success )
{
  if( d->canceled ) {
    emit canceled();
    jobFinished( false );
  }
  else if( success ) {
    m_titleRipInfos[d->currentTitleInfoIndex].clipTop = m_detectClippingJob->clippingTop();
    m_titleRipInfos[d->currentTitleInfoIndex].clipLeft = m_detectClippingJob->clippingLeft();
    m_titleRipInfos[d->currentTitleInfoIndex].clipBottom = m_detectClippingJob->clippingBottom();
    m_titleRipInfos[d->currentTitleInfoIndex].clipRight = m_detectClippingJob->clippingRight();

    startTranscoding( d->currentTitleInfoIndex );
  }
  else {
    //
    // This will probably never happen since transcode does not provide a proper error code and
    // the detect clipping job rather returns 0 clipping values than fail.
    //
    jobFinished( false );
  }
}


void K3bVideoDVDRippingJob::startTranscoding( int ripInfoIndex )
{
  d->currentTitleInfoIndex = ripInfoIndex;

  m_transcodingJob->setVideoDVD( m_dvd );
  m_transcodingJob->setTitle( m_titleRipInfos[ripInfoIndex].title );
  m_transcodingJob->setAudioStream( m_titleRipInfos[ripInfoIndex].audioStream );
  m_transcodingJob->setClipping( m_titleRipInfos[ripInfoIndex].clipTop,
				 m_titleRipInfos[ripInfoIndex].clipLeft,
				 m_titleRipInfos[ripInfoIndex].clipBottom,
				 m_titleRipInfos[ripInfoIndex].clipRight );
  m_transcodingJob->setSize( m_titleRipInfos[ripInfoIndex].width, m_titleRipInfos[ripInfoIndex].height );
  m_transcodingJob->setFilename( m_titleRipInfos[ripInfoIndex].filename );

  m_transcodingJob->start();
}


void K3bVideoDVDRippingJob::startDetectClipping( int ripInfoIndex )
{
  d->currentTitleInfoIndex = ripInfoIndex;

  if( !m_detectClippingJob ) {
    m_detectClippingJob = new K3bVideoDVDTitleDetectClippingJob( this, this );
    connectSubJob( m_detectClippingJob,
		   SLOT(slotDetectClippingJobFinished(bool)),
		   SIGNAL(newTask(const QString&)),
		   SIGNAL(newSubTask(const QString&)),
		   SLOT(slotDetectClippingProcess(int)),
		   SIGNAL(subPercent(int)),
		   0,
		   0 );
  }

  m_detectClippingJob->setVideoDVD( m_dvd );
  m_detectClippingJob->setTitle( m_titleRipInfos[ripInfoIndex].title );
  m_detectClippingJob->setLowPriority( m_transcodingJob->lowPriority() );

  m_detectClippingJob->start();
}


void K3bVideoDVDRippingJob::slotTranscodingProgress( int p )
{
  // FIXME: it is not really accurate to treat the clipping detection as half the work!
  int parts = m_titleRipInfos.count();
  if( d->autoClipping )
    parts *= 2;

  int partsDone = d->currentTitleInfoIndex;
  if( d->autoClipping ) {
    partsDone *= 2;
    partsDone++; // the autoclipping for the current title is already done
  }

  emit percent( (int)( 100.0*(float)partsDone/(float)parts + (float)p/(float)parts ) );
}


void K3bVideoDVDRippingJob::slotDetectClippingProcess( int p )
{
  // FIXME: it is not really accurate to treat the clipping detection as half the work!
  int parts = m_titleRipInfos.count();
  if( d->autoClipping )
    parts *= 2;

  int partsDone = d->currentTitleInfoIndex;
  if( d->autoClipping ) {
    partsDone *= 2;
  }

  emit percent( (int)( 100.0*(float)partsDone/(float)parts + (float)p/(float)parts ) );
}


void K3bVideoDVDRippingJob::cancel()
{
  d->canceled = true;
  if( m_transcodingJob->active() )
    m_transcodingJob->cancel();
  else if( m_detectClippingJob && m_detectClippingJob->active() )
    m_detectClippingJob->cancel();
}


void K3bVideoDVDRippingJob::setVideoCodec( int codec )
{
  m_transcodingJob->setVideoCodec( codec );
}


void K3bVideoDVDRippingJob::setVideoBitrate( int bitrate )
{
  m_transcodingJob->setVideoBitrate( bitrate );
}


void K3bVideoDVDRippingJob::setTwoPassEncoding( bool b )
{
  m_transcodingJob->setTwoPassEncoding( b );
}


void K3bVideoDVDRippingJob::setAudioCodec( int codec )
{
  m_transcodingJob->setAudioCodec( codec );
}


void K3bVideoDVDRippingJob::setAudioBitrate( int bitrate )
{
  m_transcodingJob->setAudioBitrate( bitrate );
}


void K3bVideoDVDRippingJob::setAudioVBR( bool vbr )
{
  m_transcodingJob->setAudioVBR( vbr );
}


void K3bVideoDVDRippingJob::setResampleAudioTo44100( bool b )
{
  m_transcodingJob->setResampleAudioTo44100( b );
}


void K3bVideoDVDRippingJob::setLowPriority( bool b )
{
  m_transcodingJob->setLowPriority( b );
}


void K3bVideoDVDRippingJob::setAutoClipping( bool b )
{
  d->autoClipping = b;
}

#include "k3bvideodvdrippingjob.moc"
