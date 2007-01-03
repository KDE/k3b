/* 
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 * Copyright (C) 2006 Sebastian Trueg <trueg@k3b.org>
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

#ifndef _K3B_VIDEODVD_RIPPING_JOB_H_
#define _K3B_VIDEODVD_RIPPING_JOB_H_

#include <k3bjob.h>
#include <k3bvideodvd.h>
#include <k3bvideodvdtitletranscodingjob.h>

#include <qvaluevector.h>


class K3bVideoDVDTitleDetectClippingJob;


/**
 * For details on the options see K3bVideoDVDTitleTranscodingJob
 */
class K3bVideoDVDRippingJob : public K3bJob
{
  Q_OBJECT

 public:
  K3bVideoDVDRippingJob( K3bJobHandler* hdl, QObject* parent );
  ~K3bVideoDVDRippingJob();

  class TitleRipInfo {
  public:
    TitleRipInfo();
    TitleRipInfo( int title,
		  int audioStream = 0,
		  const QString& fn = QString::null,
		  int width = 0,  // 0 -> no resize
		  int height = 0, // 0 -> no resize
		  int videoBitrate = 0, // 0 -> use default from job settings
		  int clipTop = 0,
		  int clipLeft = 0,
		  int clipBottom = 0,
		  int clipRight = 0 );
    int title;
    int audioStream;
    QString filename;
    int width;
    int height;
    int videoBitrate;
    int clipTop;
    int clipLeft;
    int clipBottom;
    int clipRight;
  };

  QString jobDescription() const;
  QString jobDetails() const;

 public slots:
  void start();
  void cancel();

  void setVideoDVD( const K3bVideoDVD::VideoDVD& dvd ) { m_dvd = dvd; }
  void setTitles( const QValueVector<TitleRipInfo>& titles ) { m_titleRipInfos = titles; }

  void setVideoCodec( K3bVideoDVDTitleTranscodingJob::VideoCodec codec );
  void setVideoBitrate( int bitrate );
  void setTwoPassEncoding( bool b );
  void setAudioCodec( K3bVideoDVDTitleTranscodingJob::AudioCodec codec );
  void setAudioBitrate( int bitrate );
  void setAudioVBR( bool vbr );
  void setResampleAudioTo44100( bool b );
  void setLowPriority( bool b );
  void setAutoClipping( bool b );

 private slots:
  void slotTranscodingJobFinished( bool );
  void slotDetectClippingJobFinished( bool );
  void slotTranscodingProgress( int );
  void slotDetectClippingProgress( int );

 private:
  void startTranscoding( int ripInfoIndex );
  void startDetectClipping( int ripInfoIndex );
  void initProgressInfo();

  K3bVideoDVD::VideoDVD m_dvd;
  QValueVector<TitleRipInfo> m_titleRipInfos;

  K3bVideoDVDTitleTranscodingJob* m_transcodingJob;
  K3bVideoDVDTitleDetectClippingJob* m_detectClippingJob;

  class Private;
  Private* d;
};

#endif
