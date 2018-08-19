/*
 *
 * Copyright (C) 2006-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009-2010 Michal Malek <michalm@jabster.pl>
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

#include "k3bvideodvdrippingpreview.h"

#include "k3bcore.h"
#include "k3bexternalbinmanager.h"
#include "k3bdevice.h"
#include "k3bprocess.h"
#include <QDebug>
#include <QDir>
#include <QTemporaryDir>



K3b::VideoDVDRippingPreview::VideoDVDRippingPreview( QObject* parent )
    : QObject( parent ),
      m_process( 0 )
{
}


K3b::VideoDVDRippingPreview::~VideoDVDRippingPreview()
{
    cancel();
    if (m_process) {
        m_process->deleteLater();
        m_process = Q_NULLPTR;
    }
}


void K3b::VideoDVDRippingPreview::generatePreview( const K3b::VideoDVD::VideoDVD& dvd, int title, int chapter )
{
    // cleanup first
    cancel();
    if (m_process) {
        m_process->deleteLater();
        m_process = 0;
    }
    m_tempDir.reset();
    m_canceled = false;

    const K3b::ExternalBin* bin = k3bcore->externalBinManager()->binObject("transcode");
    if( !bin ) {
        emit previewDone( false );
        return;
    }

    // auto-select a chapter
    // choose the center chapter, but not the first or last if possible
    if( chapter == 0 )
        chapter = qMin( qMax( dvd[title-1].numChapters()/2, 2U ), qMax( dvd[title-1].numChapters() - 1, 1U ) );

    // select a frame number
    unsigned int frame = 30;
    if( dvd[title-1][chapter-1].playbackTime().totalFrames() < frame )
        frame = dvd[title-1][chapter-1].playbackTime().totalFrames() / 2;

    m_dvd = dvd;
    m_title = title;
    m_chapter = chapter;

    m_tempDir.reset( new QTemporaryDir );

    m_process = new Process();
    *m_process << bin->path();
    if ( bin->version() >= Version( 1, 1, 0 ) )
        *m_process << "--log_no_color";
    *m_process << "-i" << dvd.device()->blockDeviceName();
    *m_process << "-T" << QString("%1,%2").arg(title).arg(chapter);
    if ( bin->version() < Version( 1, 1, 0 ) ) {
        *m_process << "-x" << "dvd,null";
        *m_process << "--dvd_access_delay" << "0";
    }
    else {
        *m_process << "-x" << "dvd='delay=0',null";
    }
    *m_process << "-y" << "ppm,null";
    *m_process << "-c" << QString("%1-%2").arg( frame ).arg( frame+1 );
    *m_process << "-Z" << "x200";
    *m_process << "-o" << m_tempDir->path();

    connect( m_process, SIGNAL(finished(int,QProcess::ExitStatus)),
             this, SLOT(slotTranscodeFinished(int,QProcess::ExitStatus)) );
    if ( !m_process->start(KProcess::ForwardedChannels) ) {
        // something went wrong when starting the program
        // it "should" be the executable
        qDebug() << "(K3b::VideoDVDRippingPreview) Could not start transcode.";
        m_process->deleteLater();
        m_process = 0;
        m_tempDir.reset();
        emit previewDone( false );
    }
}


void K3b::VideoDVDRippingPreview::cancel()
{
    if( m_process && m_process->isRunning() ) {
        m_canceled = true;
        m_process->kill();
        m_process->waitForFinished();
    }
}


void K3b::VideoDVDRippingPreview::slotTranscodeFinished( int, QProcess::ExitStatus exitStatus)
{
    if( exitStatus != QProcess::NormalExit )
        return;

    // read the image
    QString filename = QDir( m_tempDir->path() ).filePath( "000000.ppm" );// + tempQDir->entryList( QDir::Files ).first();
    qDebug() << "(K3b::VideoDVDRippingPreview) reading from file " << filename;
    m_preview = QImage( filename );
    bool success = !m_preview.isNull() && !m_canceled;

    // remove temp files
    m_tempDir.reset();

    // clean up
    m_process->deleteLater();
    m_process = 0;

    qDebug() << "Preview done:" << success;

    // retry the first chapter in case another failed
    if( !success && m_chapter > 1 )
        generatePreview( m_dvd, m_title, 1 );
    else
        emit previewDone( success );
}


