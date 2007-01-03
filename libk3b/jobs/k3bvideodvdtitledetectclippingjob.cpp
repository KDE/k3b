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

#include "k3bvideodvdtitledetectclippingjob.h"

#include <k3bexternalbinmanager.h>
#include <k3bprocess.h>
#include <k3bcore.h>
#include <k3bglobals.h>

#include <klocale.h>
#include <kdebug.h>


static const int s_unrealisticHighClippingValue = 100000;


class K3bVideoDVDTitleDetectClippingJob::Private
{
public:
  const K3bExternalBin* usedTranscodeBin;

  K3bProcess* process;

  bool canceled;

  unsigned int currentChapter;
  unsigned int currentFrames;
  unsigned int totalChapters;

  int lastProgress;
  int lastSubProgress;
};



K3bVideoDVDTitleDetectClippingJob::K3bVideoDVDTitleDetectClippingJob( K3bJobHandler* hdl, QObject* parent )
  : K3bJob( hdl, parent ),
    m_clippingTop( 0 ),
    m_clippingBottom( 0 ),
    m_clippingLeft( 0 ),
    m_clippingRight( 0 ),
    m_lowPriority( true )
{
  d = new Private;
  d->process = 0;
}


K3bVideoDVDTitleDetectClippingJob::~K3bVideoDVDTitleDetectClippingJob()
{
  delete d->process;
  delete d;
}


void K3bVideoDVDTitleDetectClippingJob::start()
{
  jobStarted();

  d->canceled = false;
  d->lastProgress = 0;

  //
  // It seems as if the last chapter is often way too short
  //
  d->totalChapters = m_dvd[m_titleNumber-1].numPTTs();
  if( d->totalChapters > 1 && m_dvd[m_titleNumber-1][d->totalChapters-1].playbackTime().totalFrames() < 200 )
    d->totalChapters--;

  // initial values (some way to big value)
  m_clippingTop = s_unrealisticHighClippingValue;
  m_clippingBottom = s_unrealisticHighClippingValue;
  m_clippingLeft = s_unrealisticHighClippingValue;
  m_clippingRight = s_unrealisticHighClippingValue;

  d->usedTranscodeBin = k3bcore->externalBinManager()->binObject("transcode");
  if( !d->usedTranscodeBin ) {
    emit infoMessage( i18n("%1 executable could not be found.").arg("transcode"), ERROR );
    jobFinished( false );
    return;
  }

  if( d->usedTranscodeBin->version < K3bVersion( 1, 0, 0 ) ){
    emit infoMessage( i18n("%1 version %2 is too old.")
		      .arg("transcode")
		      .arg(d->usedTranscodeBin->version), ERROR );
    jobFinished( false );
    return;
  }

  emit debuggingOutput( "Used versions", "transcode: " + d->usedTranscodeBin->version );

  if( !d->usedTranscodeBin->copyright.isEmpty() )
    emit infoMessage( i18n("Using %1 %2 - Copyright (C) %3")
		      .arg(d->usedTranscodeBin->name())
		      .arg(d->usedTranscodeBin->version)
		      .arg(d->usedTranscodeBin->copyright), INFO );

  emit newTask( i18n("Analysing Title %1 of Video DVD %2").arg(m_titleNumber).arg(m_dvd.volumeIdentifier()) );

  startTranscode( 1 );
}


void K3bVideoDVDTitleDetectClippingJob::startTranscode( int chapter )
{
  d->currentChapter = chapter;
  d->lastSubProgress = 0;

  //
  // If we have only one chapter and it is not longer than 2 minutes (value guessed based on some test DVD)
  // use the whole chapter
  //
  if( d->totalChapters == 1 )
    d->currentFrames = QMIN( 3000, QMAX( 1, m_dvd[m_titleNumber-1][d->currentChapter-1].playbackTime().totalFrames() ) );
  else
    d->currentFrames = QMIN( 200, QMAX( 1, m_dvd[m_titleNumber-1][d->currentChapter-1].playbackTime().totalFrames() ) );

  //
  // prepare the process
  //
  delete d->process;
  d->process = new K3bProcess();
  d->process->setSuppressEmptyLines(true);
  d->process->setSplitStdout(true);
  //  connect( d->process, SIGNAL(stderrLine(const QString&)), this, SLOT(slotTranscodeStderr(const QString&)) );
  connect( d->process, SIGNAL(stdoutLine(const QString&)), this, SLOT(slotTranscodeStderr(const QString&)) );
  connect( d->process, SIGNAL(processExited(KProcess*)), this, SLOT(slotTranscodeExited(KProcess*)) );

  // the executable
  *d->process << d->usedTranscodeBin;

  // low priority
  if( m_lowPriority )
    *d->process << "--nice" << "19";

  // the input
  *d->process << "-i" << m_dvd.device()->blockDeviceName();

  // select the title number and chapter
  *d->process << "-T" << QString("%1,%2").arg(m_titleNumber).arg(chapter);

  // null output
  *d->process << "-y" << "null,null" << "-o" << "/dev/null";

  // analyze the first 200 frames
  *d->process << "-J" << QString("detectclipping=range=0-%1/5").arg(d->currentFrames);

  // also only decode the first 200 frames
  *d->process << "-c" << QString("0-%1").arg(d->currentFrames+1);

  // additional user parameters from config
  const QStringList& params = d->usedTranscodeBin->userParameters();
  for( QStringList::const_iterator it = params.begin(); it != params.end(); ++it )
    *d->process << *it;

  // produce some debugging output
  kdDebug() << "***** transcode parameters:\n";
  const QValueList<QCString>& args = d->process->args();
  QString s;
  for( QValueList<QCString>::const_iterator it = args.begin(); it != args.end(); ++it ) {
    s += *it + " ";
  }
  kdDebug() << s << flush << endl;
  emit debuggingOutput( d->usedTranscodeBin->name() + " command:", s);

  // start the process
  if( !d->process->start( KProcess::NotifyOnExit, KProcess::All ) ) {
    // something went wrong when starting the program
    // it "should" be the executable
    emit infoMessage( i18n("Could not start %1.").arg(d->usedTranscodeBin->name()), K3bJob::ERROR );
    jobFinished(false);
  }
  else {
    emit newSubTask( i18n("Analysing Chapter %1 of %2").arg(chapter).arg(m_dvd[m_titleNumber-1].numPTTs()) );
    emit subPercent( 0 );
  }
}


void K3bVideoDVDTitleDetectClippingJob::cancel()
{
  d->canceled = true;
  if( d->process && d->process->isRunning() )
    d->process->kill();
}


void K3bVideoDVDTitleDetectClippingJob::slotTranscodeStderr( const QString& line )
{
  emit debuggingOutput( "transcode", line );

  // parse progress
  // encoding frame [185],  24.02 fps, 93.0%, ETA: 0:00:00, ( 0| 0| 0)
  if( line.startsWith( "encoding frame" ) ) {
    int pos1 = line.find( '[', 15 );
    int pos2 = line.find( ']', pos1+1 );
    if( pos1 > 0 && pos2 > 0 ) {
      bool ok;
      int encodedFrames = line.mid( pos1+1, pos2-pos1-1 ).toInt( &ok );
      if( ok ) {
	int progress = 100 * encodedFrames / d->currentFrames;

	if( progress > d->lastSubProgress ) {
	  d->lastSubProgress = progress;
	  emit subPercent( progress );
	}

	double part = 100.0 / (double)d->totalChapters;

	progress = (int)( ( (double)(d->currentChapter-1) * part )
			  + ( (double)progress / (double)d->totalChapters )
			  + 0.5 );

	if( progress > d->lastProgress ) {
	  d->lastProgress = progress;
	  emit percent( progress );
	}
      }
    }
  }

  // [detectclipping#0] valid area: X: 5..719 Y: 72..507  -> -j 72,6,68,0
  else if( line.startsWith( "[detectclipping" ) ) {
    int pos = line.find( "-j" );
    if( pos > 0 ) {
      QStringList values = QStringList::split( ',', line.mid( pos+3 ) );
      m_clippingTop = QMIN( m_clippingTop, values[0].toInt() );
      m_clippingLeft = QMIN( m_clippingLeft, values[1].toInt() );
      m_clippingBottom = QMIN( m_clippingBottom, values[2].toInt() );
      m_clippingRight = QMIN( m_clippingRight, values[3].toInt() );
    }
    else
      kdDebug() << "(K3bVideoDVDTitleDetectClippingJob) failed to parse line: " << line << endl;
  }
}


void K3bVideoDVDTitleDetectClippingJob::slotTranscodeExited( KProcess* p )
{
  switch( p->exitStatus() ) {
  case 0:
    d->currentChapter++;
    if( d->currentChapter > d->totalChapters ) {
      //
      // check if we did set any values at all
      //
      if( m_clippingTop == s_unrealisticHighClippingValue )
	m_clippingTop = m_clippingLeft = m_clippingBottom = m_clippingRight = 0;
      
      if( d->totalChapters < m_dvd[m_titleNumber-1].numPTTs() )
	emit infoMessage( i18n("Ignoring last chapter due to its short playback time."), INFO );

      jobFinished( true );
    }
    else {
      startTranscode( d->currentChapter );
    }
    break;
    
  default:
    // FIXME: error handling

    if( d->canceled ) {
      emit canceled();
    }
    else {
      emit infoMessage( i18n("%1 returned an unknown error (code %2).")
			.arg(d->usedTranscodeBin->name()).arg(p->exitStatus()), 
			K3bJob::ERROR );
      emit infoMessage( i18n("Please send me an email with the last output."), K3bJob::ERROR );
    }

    jobFinished( false );
  }
}

#include "k3bvideodvdtitledetectclippingjob.moc"
