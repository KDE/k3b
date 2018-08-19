/*
 *
 * Copyright (C) 2006-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bvideodvdrippingjob.h"

#include "k3bvideodvdtitletranscodingjob.h"
#include "k3bvideodvdtitledetectclippingjob.h"

#include <QDebug>
#include <KLocalizedString>


K3b::VideoDVDRippingJob::TitleRipInfo::TitleRipInfo()
    : title(1),
      audioStream(0),
      width(0),
      height(0),
      videoBitrate(0),
      clipTop(0),
      clipLeft(0),
      clipBottom(0),
      clipRight(0)
{
}


K3b::VideoDVDRippingJob::TitleRipInfo::TitleRipInfo( int _title,
                                                     int _audioStream,
                                                     const QString& fn,
                                                     int _width,
                                                     int _height,
                                                     int _videoBitrate,
                                                     int _clipTop,
                                                     int _clipLeft,
                                                     int _clipBottom,
                                                     int _clipRight )
    : title(_title),
      audioStream(_audioStream),
      filename(fn),
      width(_width),
      height(_height),
      videoBitrate(_videoBitrate),
      clipTop(_clipTop),
      clipLeft(_clipLeft),
      clipBottom(_clipBottom),
      clipRight(_clipRight)
{
}



class K3b::VideoDVDRippingJob::Private {
public:
    Private()
        : autoClipping( true ) {
    }

    int currentTitleInfoIndex;
    bool autoClipping;

    bool canceled;

    int videoBitrate;

    int failedTitles;

    QVector<double> titleProgressParts;
    QVector<double> titleClippingProgressParts;
};



K3b::VideoDVDRippingJob::VideoDVDRippingJob( K3b::JobHandler* hdl, QObject* parent )
    : K3b::Job( hdl, parent )
{
    d = new Private();

    m_transcodingJob = new K3b::VideoDVDTitleTranscodingJob( this, this );
    connectSubJob( m_transcodingJob,
                   SLOT(slotTranscodingJobFinished(bool)),
                   SIGNAL(newTask(QString)),
                   SIGNAL(newSubTask(QString)),
                   SLOT(slotTranscodingProgress(int)),
                   SIGNAL(subPercent(int)),
                   0,
                   0 );
    m_detectClippingJob = 0;
}


K3b::VideoDVDRippingJob::~VideoDVDRippingJob()
{
    delete d;
}


QString K3b::VideoDVDRippingJob::jobDescription() const
{
    return i18n("Ripping Video DVD Titles");
}


QString K3b::VideoDVDRippingJob::jobDetails() const
{
    return i18np("Transcoding 1 title to %2/%3", "Transcoding %1 titles to %2/%3", m_titleRipInfos.count(),
                 K3b::VideoDVDTitleTranscodingJob::videoCodecString( m_transcodingJob->videoCodec() ) ,
                 K3b::VideoDVDTitleTranscodingJob::audioCodecString( m_transcodingJob->audioCodec() ) );
}


void K3b::VideoDVDRippingJob::start()
{
    jobStarted();
    d->canceled = false;
    d->failedTitles = 0;

    initProgressInfo();

    if( d->autoClipping )
        startDetectClipping( 0 );
    else
        startTranscoding( 0 );
}


void K3b::VideoDVDRippingJob::slotTranscodingJobFinished( bool success )
{
    if( d->canceled ) {
        emit canceled();
        jobFinished( false );
    }
    else {
        if( success )
            emit infoMessage( i18n("Successfully ripped title %1 to '%2'",
                                   m_titleRipInfos[d->currentTitleInfoIndex].title,
                                   m_titleRipInfos[d->currentTitleInfoIndex].filename ), MessageSuccess );
        else {
            d->failedTitles++;
            emit infoMessage( i18n("Failed to rip title %1", m_titleRipInfos[d->currentTitleInfoIndex].title), MessageError );
        }

        ++d->currentTitleInfoIndex ;
        if( d->currentTitleInfoIndex < m_titleRipInfos.count() ) {
            if( d->autoClipping )
                startDetectClipping( d->currentTitleInfoIndex );
            else
                startTranscoding( d->currentTitleInfoIndex );
        }
        else {
            jobFinished( d->failedTitles == 0 );
        }
    }
}


void K3b::VideoDVDRippingJob::slotDetectClippingJobFinished( bool success )
{
    if( d->canceled ) {
        emit canceled();
        jobFinished( false );
    }
    else {
        m_titleRipInfos[d->currentTitleInfoIndex].clipTop = 0;
        m_titleRipInfos[d->currentTitleInfoIndex].clipLeft = 0;
        m_titleRipInfos[d->currentTitleInfoIndex].clipBottom = 0;
        m_titleRipInfos[d->currentTitleInfoIndex].clipRight = 0;

        if( success ) {
            emit infoMessage( i18n("Determined clipping values for title %1",m_titleRipInfos[d->currentTitleInfoIndex].title), MessageSuccess );
            emit infoMessage( i18n("Top: %1, Bottom: %2", m_detectClippingJob->clippingTop(), m_detectClippingJob->clippingBottom()), MessageInfo );
            emit infoMessage( i18n("Left: %1, Right: %2", m_detectClippingJob->clippingLeft(), m_detectClippingJob->clippingRight()), MessageInfo );

            // let's see if the clipping values make sense
            if( m_detectClippingJob->clippingTop() + m_detectClippingJob->clippingBottom()
                >= (int)m_dvd[d->currentTitleInfoIndex].videoStream().pictureHeight() ||
                m_detectClippingJob->clippingLeft() + m_detectClippingJob->clippingRight()
                >= (int)m_dvd[d->currentTitleInfoIndex].videoStream().pictureWidth() ) {
                emit infoMessage( i18n("Insane clipping values. No clipping will be done at all."), MessageWarning );
            }
            else {
                m_titleRipInfos[d->currentTitleInfoIndex].clipTop = m_detectClippingJob->clippingTop();
                m_titleRipInfos[d->currentTitleInfoIndex].clipLeft = m_detectClippingJob->clippingLeft();
                m_titleRipInfos[d->currentTitleInfoIndex].clipBottom = m_detectClippingJob->clippingBottom();
                m_titleRipInfos[d->currentTitleInfoIndex].clipRight = m_detectClippingJob->clippingRight();
            }
        }
        else
            emit infoMessage( i18n("Failed to determine clipping values for title %1",m_titleRipInfos[d->currentTitleInfoIndex].title), MessageError );

        startTranscoding( d->currentTitleInfoIndex );
    }
}


void K3b::VideoDVDRippingJob::startTranscoding( int ripInfoIndex )
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

    if( m_titleRipInfos[ripInfoIndex].videoBitrate > 0 )
        m_transcodingJob->setVideoBitrate( m_titleRipInfos[ripInfoIndex].videoBitrate );
    else
        m_transcodingJob->setVideoBitrate( d->videoBitrate );

    m_transcodingJob->start();
}


void K3b::VideoDVDRippingJob::startDetectClipping( int ripInfoIndex )
{
    d->currentTitleInfoIndex = ripInfoIndex;

    if( !m_detectClippingJob ) {
        m_detectClippingJob = new K3b::VideoDVDTitleDetectClippingJob( this, this );
        connectSubJob( m_detectClippingJob,
                       SLOT(slotDetectClippingJobFinished(bool)),
                       SIGNAL(newTask(QString)),
                       SIGNAL(newSubTask(QString)),
                       SLOT(slotDetectClippingProgress(int)),
                       SIGNAL(subPercent(int)),
                       0,
                       0 );
    }

    m_detectClippingJob->setVideoDVD( m_dvd );
    m_detectClippingJob->setTitle( m_titleRipInfos[ripInfoIndex].title );
    m_detectClippingJob->setLowPriority( m_transcodingJob->lowPriority() );

    m_detectClippingJob->start();
}


void K3b::VideoDVDRippingJob::slotTranscodingProgress( int p )
{
    // calculate the part already done
    double doneParts = 0.0;
    for( int i = 0; i < d->currentTitleInfoIndex; ++i ) {
        doneParts += d->titleProgressParts[i];
        if( d->autoClipping )
            doneParts += d->titleClippingProgressParts[i];
    }
    if( d->autoClipping )
        doneParts += d->titleClippingProgressParts[d->currentTitleInfoIndex];

    // and the current thing
    doneParts += (double)p/100.0*d->titleProgressParts[d->currentTitleInfoIndex];

    emit percent( (int)( 100.0*doneParts ) );
}


void K3b::VideoDVDRippingJob::slotDetectClippingProgress( int p )
{
    // calculate the part already done
    double doneParts = 0.0;
    for( int i = 0; i < d->currentTitleInfoIndex; ++i ) {
        doneParts += d->titleProgressParts[i];
        doneParts += d->titleClippingProgressParts[i];
    }

    // and the current thing
    doneParts += (double)p/100.0*d->titleClippingProgressParts[d->currentTitleInfoIndex];

    emit percent( (int)( 100.0*doneParts ) );
}


void K3b::VideoDVDRippingJob::cancel()
{
    d->canceled = true;
    if( m_transcodingJob->active() )
        m_transcodingJob->cancel();
    else if( m_detectClippingJob && m_detectClippingJob->active() )
        m_detectClippingJob->cancel();
}


void K3b::VideoDVDRippingJob::setVideoCodec( K3b::VideoDVDTitleTranscodingJob::VideoCodec codec )
{
    m_transcodingJob->setVideoCodec( codec );
}


void K3b::VideoDVDRippingJob::setVideoBitrate( int bitrate )
{
    d->videoBitrate = bitrate;
}


void K3b::VideoDVDRippingJob::setTwoPassEncoding( bool b )
{
    m_transcodingJob->setTwoPassEncoding( b );
}


void K3b::VideoDVDRippingJob::setAudioCodec( K3b::VideoDVDTitleTranscodingJob::AudioCodec codec )
{
    m_transcodingJob->setAudioCodec( codec );
}


void K3b::VideoDVDRippingJob::setAudioBitrate( int bitrate )
{
    m_transcodingJob->setAudioBitrate( bitrate );
}


void K3b::VideoDVDRippingJob::setAudioVBR( bool vbr )
{
    m_transcodingJob->setAudioVBR( vbr );
}


void K3b::VideoDVDRippingJob::setResampleAudioTo44100( bool b )
{
    m_transcodingJob->setResampleAudioTo44100( b );
}


void K3b::VideoDVDRippingJob::setLowPriority( bool b )
{
    m_transcodingJob->setLowPriority( b );
}


void K3b::VideoDVDRippingJob::setAutoClipping( bool b )
{
    d->autoClipping = b;
}


void K3b::VideoDVDRippingJob::initProgressInfo()
{
    d->titleProgressParts.resize( m_titleRipInfos.count() );
    d->titleClippingProgressParts.resize( m_titleRipInfos.count() );

    unsigned long long totalFrames = 0ULL;
    for( int i = 0; i < m_titleRipInfos.count(); ++i ) {
        if( m_transcodingJob->twoPassEncoding() )
            totalFrames += m_dvd[m_titleRipInfos[i].title-1].playbackTime().totalFrames() * 2;
        else
            totalFrames += m_dvd[m_titleRipInfos[i].title-1].playbackTime().totalFrames();

        // using my knowledge of the internals of the clipping detection job: it decodes 200 frames
        // of every chapter
        if( d->autoClipping )
            totalFrames += m_dvd[m_titleRipInfos[i].title-1].numChapters() * 200;
    }

    for( int i = 0; i < m_titleRipInfos.count(); ++i ) {
        unsigned long long titleFrames = m_dvd[m_titleRipInfos[i].title-1].playbackTime().totalFrames();
        if( m_transcodingJob->twoPassEncoding() )
            titleFrames *= 2;

        // using my knowledge of the internals of the clipping detection job: it decodes 200 frames
        // of every chapter
        unsigned long long titleClippingFrames = m_dvd[m_titleRipInfos[i].title-1].numChapters() * 200;

        if (totalFrames) {
            d->titleProgressParts[i] = (double)titleFrames/(double)totalFrames;
            d->titleClippingProgressParts[i] = (double)titleClippingFrames/(double)totalFrames;
        }
    }
}


