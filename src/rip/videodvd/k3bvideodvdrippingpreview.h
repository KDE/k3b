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

#ifndef _K3B_VIDEODVD_RIPPING_PREVIEW_H_
#define _K3B_VIDEODVD_RIPPING_PREVIEW_H_

#include <QObject>
#include <QProcess>
#include <QScopedPointer>
#include <QImage>

#include "k3bvideodvd.h"


class QTemporaryDir;

namespace K3b {
    class Process;

    class VideoDVDRippingPreview : public QObject
    {
        Q_OBJECT

    public:
        explicit VideoDVDRippingPreview( QObject* parent = 0 );
        ~VideoDVDRippingPreview() override;

        QImage preview() const { return m_preview; }

    public Q_SLOTS:
        /**
         * \param dvd The Video DVD object
         * \param title The Video DVD title to generate the preview for
         * \param chapter The Chapter number to use for the preview.
         *                If 0 the middle of the title is used.
         */
        void generatePreview( const VideoDVD::VideoDVD& dvd, int title, int chapter = 0 );

        void cancel();

    Q_SIGNALS:
        void previewDone( bool );

    private Q_SLOTS:
        void slotTranscodeFinished( int exitCode, QProcess::ExitStatus status );

    private:
        QImage m_preview;
        QScopedPointer<QTemporaryDir> m_tempDir;
        Process* m_process;
        int m_title;
        int m_chapter;
        VideoDVD::VideoDVD m_dvd;

        bool m_canceled;
    };
}

#endif
