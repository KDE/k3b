/* 
 *
 * $Id$
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include <config.h>

#ifdef HAVE_MUSICBRAINZ

#include "k3bmusicbrainzjob.h"
#include "k3btrm.h"
#include "k3bmusicbrainz.h"

#include <k3bthread.h>
#include <k3bthreadjob.h>
#include <k3baudiotrack.h>
#include <k3baudiodatasource.h>
#include <k3bsimplejobhandler.h>

#include <kmessagebox.h>
#include <kinputdialog.h>
#include <klocale.h>


// determine one trm
class K3bMusicBrainzJob::TRMThread : public K3bThread
{
public:
  TRMThread()
    : m_canceled(false) {
  }

  void run() {
    m_canceled = false;

    emitStarted();

    track->seek(0);
    m_trm.start( track->length() );

    char buffer[2352*10];
    int len = 0;
    long long dataRead = 0;
    while( !m_canceled && 
	   (len = track->read( buffer, 2352*10 )) > 0 ) {

      dataRead += len;

      // swap data
      char buf;
      for( int i = 0; i < len-1; i+=2 ) {
	buf = buffer[i];
	buffer[i] = buffer[i+1];
	buffer[i+1] = buf;
      }

      if( m_trm.generate( buffer, len ) ) {
	len = 0;
	break;
      }

      // FIXME: useless since libmusicbrainz does never need all the data
      emitPercent( 100*dataRead/track->length().audioBytes() );
    }

    if( m_canceled )
      emitCanceled();

    if( len == 0 ) {
      if( !m_trm.finalize() )
	emitFinished( false );
      else
	emitFinished( !m_canceled );
    }
    else
      emitFinished( false );
  }


  void cancel() {
    m_canceled = true;
  }

  const QCString& signature() const {
    return m_trm.signature();
  }

  K3bAudioTrack* track;

private:
  bool m_canceled;
  K3bTRM m_trm;
};



class K3bMusicBrainzJob::MusicBrainzThread : public K3bThread
{
public:
  MusicBrainzThread() {
  }

  void run() {
    emitStarted();
    m_results = m_mb.query( m_sig );
    emitFinished( m_results > 0 );
  }

  void setSignature( const QCString& sig ) {
    m_sig = sig;
  }

  unsigned int results() {
    return m_results;
  }

  const QString& title( unsigned int i = 0 ) const {
    return m_mb.title( i );
  }

  const QString& artist( unsigned int i = 0 ) const {
    return m_mb.artist( i );
  }

private:
  K3bMusicBrainz m_mb;
  int m_results;
  QCString m_sig;
};


// cannot use this as parent for the K3bSimpleJobHandler since this has not been constructed yet
K3bMusicBrainzJob::K3bMusicBrainzJob( QWidget* parent, const char* name )
  : K3bJob( new K3bSimpleJobHandler( 0 ), parent, name )
{
  m_trmThread = new TRMThread();
  m_mbThread = new MusicBrainzThread();
  m_trmJob = new K3bThreadJob( m_trmThread, this, this );
  m_mbJob = new K3bThreadJob( m_mbThread, this, this );

  connect( m_trmJob, SIGNAL(percent(int)), this, SIGNAL(subPercent(int)) );
  connect( m_trmJob, SIGNAL(percent(int)), this, SLOT(slotTrmPercent(int)) );
  connect( m_trmJob, SIGNAL(finished(bool)), this, SLOT(slotTrmJobFinished(bool)) );
  connect( m_mbJob, SIGNAL(finished(bool)), this, SLOT(slotMbJobFinished(bool)) );
}


K3bMusicBrainzJob::~K3bMusicBrainzJob()
{
  delete m_trmThread;
  delete m_trmJob;
  delete m_mbThread;
  delete m_mbJob;
  delete jobHandler();
}


void K3bMusicBrainzJob::start()
{
  jobStarted();

  m_trmThread->track = m_tracks.first();

  emit infoMessage( i18n("Generating fingerprint for track %1.")
		    .arg(m_tracks.current()->trackNumber()), INFO );

  m_trmJob->start();  
}


void K3bMusicBrainzJob::cancel()
{
  m_trmJob->cancel();
}


void K3bMusicBrainzJob::slotTrmPercent( int p )
{
  // the easy way (inaccurate)
  emit percent( (100*m_tracks.at() + p) / m_tracks.count() );
}


void K3bMusicBrainzJob::slotTrmJobFinished( bool success )
{
  if( success ) {
    // now query musicbrainz
    m_mbThread->setSignature( m_trmThread->signature() );
    emit infoMessage( i18n("Querying MusicBrainz for track %1.")
		      .arg(m_tracks.current()->trackNumber()), INFO );
    m_mbJob->start();    
  }
  else {
    if( hasBeenCanceled() )
      emit canceled();
    jobFinished(false);
  }
}


void K3bMusicBrainzJob::slotMbJobFinished( bool success )
{
  if( hasBeenCanceled() ) {
    emit canceled();
    jobFinished(false);
  }
  else {
    if( success ) {
      // found entries
      QStringList resultStrings, resultStringsUnique;
      for( unsigned int i = 0; i < m_mbThread->results(); ++i )
	resultStrings.append( m_mbThread->artist(i) + " - " + m_mbThread->title(i) );

      // since we are only using the title and the artist a lot of entries are alike to us
      // so to not let the user have to choose between two equal entries we trim the list down
      for( QStringList::const_iterator it = resultStrings.begin();
	   it != resultStrings.end(); ++it )
	if( resultStringsUnique.find( *it ) == resultStringsUnique.end() )
	  resultStringsUnique.append( *it );

      QString s;
      bool ok = true;
      if( resultStringsUnique.count() > 1 )
	s = KInputDialog::getItem( i18n("MusicBrainz Query"),
				   i18n("Found multiple matches for track %1 (%2). Please select one.")
				   .arg(m_tracks.current()->trackNumber())
				   .arg(m_tracks.current()->firstSource()->sourceComment()),
				   resultStringsUnique,
				   0,
				   false,
				   &ok,
				   dynamic_cast<QWidget*>(parent()) );
      else
	s = resultStringsUnique.first();

      if( ok ) {
	int i = resultStrings.findIndex( s );
	m_tracks.current()->setTitle( m_mbThread->title(i) );
	m_tracks.current()->setArtist( m_mbThread->artist(i) );
      }
    }
    else {
      // no entry found
      // inform user
      KMessageBox::error( dynamic_cast<QWidget*>(parent()),
			  i18n("Track %1 was not found in the MusicBrainz database.")
			  .arg( m_tracks.current()->trackNumber()) );
    }

    // query next track
    if( m_tracks.next() ) {
      emit infoMessage( i18n("Generating fingerprint for track %1.")
			.arg(m_tracks.current()->trackNumber()), INFO );
      m_trmThread->track = m_tracks.current();
      m_trmJob->start();  
    }
    else
      jobFinished( true );
  }
}

#include "k3bmusicbrainzjob.moc"

#endif
