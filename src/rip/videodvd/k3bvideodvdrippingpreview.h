/* 
 *
 * Copyright (C) 2006-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#ifndef _K3B_VIDEODVD_RIPPING_PREVIEW_H_
#define _K3B_VIDEODVD_RIPPING_PREVIEW_H_

#include <QtCore/QObject>
#include <QtGui/QImage>
#include <QtCore/QProcess>

#include <k3bvideodvd.h>


class KTempDir;
class KProcess;

class K3bVideoDVDRippingPreview : public QObject
{
    Q_OBJECT

public:
    K3bVideoDVDRippingPreview( QObject* parent = 0 );
    ~K3bVideoDVDRippingPreview();

    QImage preview() const { return m_preview; }

public Q_SLOTS:
    /**
     * \param dvd The Video DVD object
     * \param title The Video DVD title to generate the preview for
     * \param chapter The Chapter number to use for the preview. 
     *                If 0 the middle of the title is used.
     */
    void generatePreview( const K3bVideoDVD::VideoDVD& dvd, int title, int chapter = 0 );

    void cancel();

Q_SIGNALS:
    void previewDone( bool );

private Q_SLOTS:
    void slotTranscodeFinished( int exitCode, QProcess::ExitStatus status );

private:
    QImage m_preview;
    KTempDir* m_tempDir;
    KProcess* m_process;
    int m_title;
    int m_chapter;
    K3bVideoDVD::VideoDVD m_dvd;

    bool m_canceled;
};

#endif
