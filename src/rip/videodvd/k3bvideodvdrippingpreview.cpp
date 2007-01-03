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

#include "k3bvideodvdrippingpreview.h"

#include <k3bcore.h>
#include <k3bexternalbinmanager.h>
#include <k3bdevice.h>

#include <kprocess.h>
#include <ktempdir.h>
#include <kdebug.h>

#include <qdir.h>



K3bVideoDVDRippingPreview::K3bVideoDVDRippingPreview( QObject* parent )
  : QObject( parent ),
    m_tempDir( 0 ),
    m_process( 0 )
{
}


K3bVideoDVDRippingPreview::~K3bVideoDVDRippingPreview()
{
  delete m_process;
  delete m_tempDir;
}


void K3bVideoDVDRippingPreview::generatePreview( const K3bVideoDVD::VideoDVD& dvd, int title, int chapter )
{
  // cleanup first
  delete m_process;
  delete m_tempDir;
  m_process = 0;
  m_tempDir = 0;
  m_canceled = false;

  const K3bExternalBin* bin = k3bcore->externalBinManager()->binObject("transcode");
  if( !bin ) {
    emit previewDone( false );
    return;
  }

  // auto-select a chapter
  // choose the center chapter, but not the first or last if possible
  if( chapter == 0 )
    chapter = QMIN( QMAX( dvd[title-1].numChapters()/2, 2 ), QMAX( dvd[title-1].numChapters() - 1, 1 ) );

  // select a frame number
  unsigned int frame = 30;
  if( dvd[title-1][chapter-1].playbackTime().totalFrames() < frame )
    frame = dvd[title-1][chapter-1].playbackTime().totalFrames() / 2;

  m_dvd = dvd;
  m_title = title;
  m_chapter = chapter;

  m_tempDir = new KTempDir();
  m_tempDir->setAutoDelete( true );

  m_process = new KProcess();
  *m_process << bin->path;
  *m_process << "-i" << dvd.device()->blockDeviceName();
  *m_process << "-T" << QString("%1,%2").arg(title).arg(chapter);
  *m_process << "-x" << "dvd,null";
  *m_process << "--dvd_access_delay" << "0";
  *m_process << "-y" << "ppm,null";
  *m_process << "-c" << QString("%1-%2").arg( frame ).arg( frame+1 );
  *m_process << "-Z" << "x200";
  *m_process << "-o" << m_tempDir->name();

  connect( m_process, SIGNAL(processExited(KProcess*)),
	   this, SLOT(slotTranscodeFinished(KProcess*)) );
  if( !m_process->start( KProcess::NotifyOnExit, KProcess::AllOutput ) ) { // we use AllOutput to not pollute stdout
    // something went wrong when starting the program
    // it "should" be the executable
    kdDebug() << "(K3bVideoDVDRippingPreview) Could not start transcode." << endl;
    delete m_process;
    delete m_tempDir;
    m_process = 0;
    m_tempDir = 0;
    emit previewDone( false );
  }
}


void K3bVideoDVDRippingPreview::cancel()
{
  if( m_process && m_process->isRunning() ) {
    m_canceled = true;
    m_process->kill();
  }
}


void K3bVideoDVDRippingPreview::slotTranscodeFinished( KProcess* )
{
  // read the image
  QString filename = m_tempDir->name() + "000000.ppm";// + tempQDir->entryList( QDir::Files ).first();
  kdDebug() << "(K3bVideoDVDRippingPreview) reading from file " << filename << endl;
  m_preview = QImage( filename );
  bool success = !m_preview.isNull() && !m_canceled;

  // remove temp files
  delete m_tempDir;
  m_tempDir = 0;

  // clean up
  delete m_process;
  m_process = 0;

  // retry the first chapter in case another failed
  if( !success && m_chapter > 1 )
    generatePreview( m_dvd, m_title, 1 );
  else
    emit previewDone( success );
}

#include "k3bvideodvdrippingpreview.moc"
