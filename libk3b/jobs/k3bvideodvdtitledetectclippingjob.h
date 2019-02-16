/*
 *
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

#ifndef _K3B_VIDEODVD_TITLE_DETECTCLIPPING_JOB_H_
#define _K3B_VIDEODVD_TITLE_DETECTCLIPPING_JOB_H_

#include "k3b_export.h"
#include "k3bjob.h"
#include "k3bvideodvd.h"
#include <QProcess>

namespace K3b {
    /**
     * Job to detect the clipping values for a Video DVD title.
     */
    class LIBK3B_EXPORT VideoDVDTitleDetectClippingJob : public Job
    {
        Q_OBJECT

    public:
        VideoDVDTitleDetectClippingJob( JobHandler* hdl, QObject* parent );
        ~VideoDVDTitleDetectClippingJob() override;

        const VideoDVD::VideoDVD& videoDVD() const { return m_dvd; }
        int title() const { return m_titleNumber; }
        bool lowPriority() const { return m_lowPriority; }

        /**
         * Only valid after a successful completion of the job.
         */
        int clippingTop() const { return m_clippingTop; }

        /**
         * Only valid after a successful completion of the job.
         */
        int clippingLeft() const { return m_clippingLeft; }

        /**
         * Only valid after a successful completion of the job.
         */
        int clippingBottom() const { return m_clippingBottom; }

        /**
         * Only valid after a successful completion of the job.
         */
        int clippingRight() const { return m_clippingRight; }

    public Q_SLOTS:
        void start() override;
        void cancel() override;

        /**
         * The device containing the Video DVD
         */
        void setVideoDVD( const K3b::VideoDVD::VideoDVD& dvd ) { m_dvd = dvd; }

        /**
         * Set the title number to be analysed
         *
         * The default value is 1, denoting the first title.
         */
        void setTitle( int t ) { m_titleNumber = t; }

        /**
         * If true the transcode processes will be run with a very low scheduling
         * priority.
         *
         * The default is true.
         */
        void setLowPriority( bool b ) { m_lowPriority = b; }

    private Q_SLOTS:
        void slotTranscodeStderr( const QString& );
        void slotTranscodeExited( int, QProcess::ExitStatus );

    private:
        void startTranscode( int chapter );

        VideoDVD::VideoDVD m_dvd;

        int m_clippingTop;
        int m_clippingBottom;
        int m_clippingLeft;
        int m_clippingRight;

        int m_titleNumber;

        bool m_lowPriority;

        class Private;
        Private* d;
    };
}

#endif
