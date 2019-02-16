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

#ifndef _K3B_VIDEODVD_RIPPING_JOB_H_
#define _K3B_VIDEODVD_RIPPING_JOB_H_

#include "k3bjob.h"
#include "k3bvideodvd.h"
#include "k3bvideodvdtitletranscodingjob.h"

#include <QVector>


namespace K3b {
    class VideoDVDTitleDetectClippingJob;

    /**
     * For details on the options see VideoDVDTitleTranscodingJob
     */
    class VideoDVDRippingJob : public Job
    {
        Q_OBJECT

    public:
        VideoDVDRippingJob( JobHandler* hdl, QObject* parent );
        ~VideoDVDRippingJob() override;

        class TitleRipInfo {
        public:
            TitleRipInfo();
            explicit TitleRipInfo( int title,
                          int audioStream = 0,
                          const QString& fn = QString(),
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

        QString jobDescription() const override;
        QString jobDetails() const override;

    public Q_SLOTS:
        void start() override;
        void cancel() override;

        void setVideoDVD( const K3b::VideoDVD::VideoDVD& dvd ) { m_dvd = dvd; }
        void setTitles( const QVector<TitleRipInfo>& titles ) { m_titleRipInfos = titles; }

        void setVideoCodec( K3b::VideoDVDTitleTranscodingJob::VideoCodec codec );
        void setVideoBitrate( int bitrate );
        void setTwoPassEncoding( bool b );
        void setAudioCodec( K3b::VideoDVDTitleTranscodingJob::AudioCodec codec );
        void setAudioBitrate( int bitrate );
        void setAudioVBR( bool vbr );
        void setResampleAudioTo44100( bool b );
        void setLowPriority( bool b );
        void setAutoClipping( bool b );

    private Q_SLOTS:
        void slotTranscodingJobFinished( bool );
        void slotDetectClippingJobFinished( bool );
        void slotTranscodingProgress( int );
        void slotDetectClippingProgress( int );

    private:
        void startTranscoding( int ripInfoIndex );
        void startDetectClipping( int ripInfoIndex );
        void initProgressInfo();

        VideoDVD::VideoDVD m_dvd;
        QVector<TitleRipInfo> m_titleRipInfos;

        VideoDVDTitleTranscodingJob* m_transcodingJob;
        VideoDVDTitleDetectClippingJob* m_detectClippingJob;

        class Private;
        Private* d;
    };
}

#endif
