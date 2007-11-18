/* 
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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

#include "k3bcdda2wavreader.h"

#include <k3bexternalbinmanager.h>
#include <k3bdevice.h>
#include <k3bdevicemanager.h>
#include <k3bcore.h>
#include <k3bprocess.h>

#include <kdebug.h>
#include <klocale.h>

#include <q3valuevector.h>
#include <qregexp.h>


class K3bCdda2wavReader::Private
{
public:
  Private() 
    : cdda2wavBin(0),
      process(0),
      cancaled(false),
      running(false),
      fdToWriteTo(-1) {
  }

  const K3bExternalBin* cdda2wavBin;
  K3bProcess* process;

  bool cancaled;
  bool running;

  int fdToWriteTo;

  int currentTrack;
  Q3ValueVector<int> trackOffsets;
};


K3bCdda2wavReader::K3bCdda2wavReader( QObject* parent )
  : K3bJob( parent )
{
  d = new Private();
}


K3bCdda2wavReader::~K3bCdda2wavReader()
{
  delete d->process;
  delete d;
}


bool K3bCdda2wavReader::active() const
{
  return d->running;
}


void K3bCdda2wavReader::writeToFd( int fd )
{
  d->fdToWriteTo = fd;
}


void K3bCdda2wavReader::start()
{
  start( false );
}


void K3bCdda2wavReader::start( bool onlyInfo )
{
  d->running = true;
  d->cancaled = false;
  d->currentTrack = 1;
  d->trackOffsets.clear();

  jobStarted();

  d->cdda2wavBin = k3bcore->externalBinManager()->binObject( "cdda2wav" );
  if( !d->cdda2wavBin ) {
    emit infoMessage( i18n("Could not find %1 executable.").arg("cdda2wav"), ERROR );
    jobFinished(false);
    d->running = false;
    return;
  }

  // prepare the process
  delete d->process;
  d->process = new K3bProcess();
  d->process->setSplitStdout(true);
  d->process->setSuppressEmptyLines(true);
  d->process->setWorkingDirectory( m_imagePath );
  connect( d->process, SIGNAL(stdoutLine(const QString&)), this, SLOT(slotProcessLine(const QString&)) );
  connect( d->process, SIGNAL(stderrLine(const QString&)), this, SLOT(slotProcessLine(const QString&)) );
  connect( d->process, SIGNAL(processExited(K3Process*)), this, SLOT(slotProcessExited(K3Process*)) );

  // create the command line
  *d->process << d->cdda2wavBin->path;
  *d->process << "-vall" << ( d->cdda2wavBin->hasFeature( "gui" ) ? "-gui" : "-g" );
  if( d->cdda2wavBin->hasFeature( "dev" ) )
    *d->process << QString("dev=%1").arg(K3bDevice::externalBinDeviceParameter(m_device, d->cdda2wavBin));
  else
    *d->process << "-D" << K3bDevice::externalBinDeviceParameter(m_device, d->cdda2wavBin);
  *d->process << ( d->cdda2wavBin->hasFeature( "bulk" ) ? "-bulk" : "-B" );
  if( onlyInfo )
    *d->process << ( d->cdda2wavBin->hasFeature( "info-only" ) ? "-info-only" : "-J" );
  else if( d->fdToWriteTo != -1 )
    *d->process << ( d->cdda2wavBin->hasFeature( "no-infofile" ) ? "-no-infofile" : "-H" );

  // additional user parameters from config
  const QStringList& params = d->cdda2wavBin->userParameters();
  for( QStringList::const_iterator it = params.begin(); it != params.end(); ++it )
    *d->process << *it;

  // start the thing
  if( !d->process->start( K3Process::NotifyOnExit, K3Process::All ) ) {
    // something went wrong when starting the program
    // it "should" be the executable
    kDebug() << "(K3bCdda2wavReader) could not start cdda2wav";
    emit infoMessage( i18n("Could not start %1.").arg("cdda2wav"), K3bJob::ERROR );
    d->running = false;
    jobFinished(false);
  }
}


void K3bCdda2wavReader::cancel()
{
  if( d->running ) {
    d->cancaled = true;
    if( d->process )
      if( d->process->isRunning() )
	d->process->kill();
  }
}


void K3bCdda2wavReader::slotProcessLine( const QString& line )
{
  // Tracks:11 44:37.30
  // CDINDEX discid: ZvzBXv614ACgzn1bWWy107cs0nA-
  // CDDB discid: 0x8a0a730b
  // CD-Text: not detected
  // CD-Extra: not detected
  // Album title: '' from ''
  // T01:       0  3:39.70 audio linear copydenied stereo title '' from ''
  // T02:   16495  3:10.47 audio linear copydenied stereo title '' from ''
  // T03:   30792  3:30.00 audio linear copydenied stereo title '' from ''
  // T04:   46542  4:05.05 audio linear copydenied stereo title '' from ''
  // T05:   64922  3:44.35 audio linear copydenied stereo title '' from ''
  // T06:   81757  4:36.45 audio linear copydenied stereo title '' from ''
  // T07:  102502  3:59.30 audio linear copydenied stereo title '' from ''
  // T08:  120457  5:24.30 audio linear copydenied stereo title '' from ''
  // T09:  144787  3:26.28 audio linear copydenied stereo title '' from ''
  // T10:  160265  4:07.20 audio linear copydenied stereo title '' from ''
  // T11:  178810  4:51.20 audio linear copydenied stereo title '' from ''

  // percent_done:
  // 100%  track  1 successfully recorded
  // 100%  track  2 successfully recorded
  // 100%  track  3 successfully recorded



  static QRegExp rx( "T\\d\\d:" );
  if( rx.exactMatch( line.left(4) ) || line.startsWith( "Leadout" ) ) {
    int pos = line.find( " " );
    int endpos = line.find( QRegExp( "\\d" ), pos );
    endpos = line.find( " ", endpos );
    bool ok;
    int offset = line.mid( pos, endpos-pos ).toInt(&ok);
    if( ok )
      d->trackOffsets.append( offset );
    else
      kDebug() << "(K3bCdda2wavReader) track offset parsing error: '" << line.mid( pos, endpos-pos ) << "'";
  }

  else if( line.startsWith( "percent_done" ) ) {
    // the reading starts
    d->currentTrack = 1;
    emit nextTrack( d->currentTrack, d->trackOffsets.count() );
  }

  else if( line.contains("successfully recorded") ) {
    d->currentTrack++;
    emit nextTrack( d->currentTrack, d->trackOffsets.count() );
  }

  else if( line.contains("%") ) {
    // parse progress
    bool ok;
    int p = line.left(3).toInt(&ok);
    if( ok ) {
      emit subPercent( p );

      int overall = d->trackOffsets[d->currentTrack-1];
      int tSize = d->trackOffsets[d->currentTrack] - d->trackOffsets[d->currentTrack-1];
      overall += (tSize*p/100);

      emit percent( overall*100/d->trackOffsets[d->trackOffsets.count()-1] );
    }
    else
      kDebug() << "(K3bCdda2wavReader) track progress parsing error: '" << line.left(3) << "'";
  }
}


void K3bCdda2wavReader::slotProcessExited( K3Process* p )
{
  d->running = false;

  if( d->cancaled ) {
    emit canceled();
    jobFinished(false);
    return;
  }

  if( p->normalExit() ) {
    // TODO: improve this

    if( p->exitStatus() == 0 ) {
      jobFinished( true );
    }
    else {
      emit infoMessage( i18n("%1 returned an unknown error (code %2).")
			.arg("Cdda2wav").arg(p->exitStatus()), ERROR );
      jobFinished( false );
    }
  }
  else {
    emit infoMessage( i18n("%1 did not exit cleanly.").arg("Cdda2wav"), 
		      ERROR );
    jobFinished( false );
  }
}

#include "k3bcdda2wavreader.moc"
