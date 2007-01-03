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

#include "k3baudiotrackaddingdialog.h"

#include <k3baudiodoc.h>
#include <k3baudiotrack.h>
#include <k3baudiofile.h>
#include <k3baudiodecoder.h>
#include <k3bbusywidget.h>
#include <k3bglobals.h>
#include <k3bthread.h>
#include <k3bthreadjob.h>
#include <k3bcuefileparser.h>

#include <klocale.h>
#include <kdebug.h>
#include <kmessagebox.h>

#include <qlabel.h>
#include <qlayout.h>
#include <qfileinfo.h>
#include <qtimer.h>
#include <qdir.h>


class K3bAudioTrackAddingDialog::AnalyserThread : public K3bThread
{
public:
  AnalyserThread()
    : K3bThread(),
      m_decoder(0) {
  }

  void run() {
    emitStarted();
    m_decoder->analyseFile();
    emitFinished( m_decoder->isValid() );
  }

  K3bAudioDecoder* m_decoder;
};


K3bAudioTrackAddingDialog::K3bAudioTrackAddingDialog( QWidget* parent, const char* name )
  : KDialogBase( Plain,
		 i18n("Please be patient..."),
		 Cancel,
		 Cancel,
		 parent,
		 name,
		 true,
		 true ),
    m_bCanceled(false)
{
  QWidget* page = plainPage();
  QGridLayout* grid = new QGridLayout( page );
  grid->setSpacing( spacingHint() );
  grid->setMargin( marginHint() );

  m_infoLabel = new QLabel( page );
  m_busyWidget = new K3bBusyWidget( page );

  grid->addWidget( m_infoLabel, 0, 0 );
  grid->addWidget( m_busyWidget, 1, 0 );

  m_analyserThread = new AnalyserThread();
  m_analyserJob = new K3bThreadJob( m_analyserThread, this, this );
  connect( m_analyserJob, SIGNAL(finished(bool)), this, SLOT(slotAnalysingFinished(bool)) );
}


K3bAudioTrackAddingDialog::~K3bAudioTrackAddingDialog()
{
  delete m_analyserThread;
}


int K3bAudioTrackAddingDialog::addUrls( const KURL::List& urls, 
					K3bAudioDoc* doc,
					K3bAudioTrack* afterTrack,
					K3bAudioTrack* parentTrack,
					K3bAudioDataSource* afterSource,
					QWidget* parent )
{
  if( urls.isEmpty() )
    return 0;

  K3bAudioTrackAddingDialog dlg( parent );
  dlg.m_urls = extractUrlList( urls );
  dlg.m_doc = doc;
  dlg.m_trackAfter = afterTrack;
  dlg.m_parentTrack = parentTrack;
  dlg.m_sourceAfter = afterSource;
  dlg.m_infoLabel->setText( i18n("Adding files to project \"%1\"...").arg(doc->URL().fileName()) );

  dlg.m_busyWidget->showBusy(true);
  QTimer::singleShot( 0, &dlg, SLOT(slotAddUrls()) );
  int ret = dlg.exec();

  QString message;
  if( !dlg.m_unreadableFiles.isEmpty() )
    message += QString("<p><b>%1:</b><br>%2")
      .arg( i18n("Insufficient permissions to read the following files") )
      .arg( dlg.m_unreadableFiles.join( "<br>" ) );
  if( !dlg.m_notFoundFiles.isEmpty() )
    message += QString("<p><b>%1:</b><br>%2")
      .arg( i18n("Unable to find the following files") )
      .arg( dlg.m_notFoundFiles.join( "<br>" ) );
  if( !dlg.m_nonLocalFiles.isEmpty() )
    message += QString("<p><b>%1:</b><br>%2")
      .arg( i18n("No non-local files supported") )
      .arg( dlg.m_unreadableFiles.join( "<br>" ) );
  if( !dlg.m_unsupportedFiles.isEmpty() )
    message += QString("<p><b>%1:</b><br><i>%2</i><br>%3")
      .arg( i18n("Unable to handle the following files due to an unsupported format" ) )
      .arg( i18n("You may manually convert these audio files to wave using another "
		 "application supporting the audio format and then add the wave files "
		 "to the K3b project.") )
      .arg( dlg.m_unsupportedFiles.join( "<br>" ) );

  if( !message.isEmpty() )
    KMessageBox::detailedSorry( parent, i18n("Problems while adding files to the project."), message );

  return ret;
}



void K3bAudioTrackAddingDialog::slotAddUrls()
{
  if( m_bCanceled )
    return;

  if( m_urls.isEmpty() ) {
    accept();
    return;
  }

  KURL url = m_urls.first();
  bool valid = true;

  if( url.path().right(3).lower() == "cue" ) {
    // see if its a cue file
    K3bCueFileParser parser( url.path() );
    if( parser.isValid() && parser.toc().contentType() == K3bDevice::AUDIO ) {
      // remember cue url and set the new audio file url
      m_cueUrl = url;
      url = m_urls[0] = KURL::fromPathOrURL( parser.imageFilename() );
    }
  }

  m_infoLabel->setText( i18n("Analysing file '%1'...").arg( url.fileName() ) );

  if( !url.isLocalFile() ) {
    valid = false;
    m_nonLocalFiles.append( url.path() );
  }
  else {
    QFileInfo fi( url.path() );
    if( !fi.exists() ) {
      valid = false;
      m_notFoundFiles.append( url.path() );
    }
    else if( !fi.isReadable() ) {
      valid = false;
      m_unreadableFiles.append( url.path() );
    }
  }

  if( valid ) {
    bool reused;
    K3bAudioDecoder* dec = m_doc->getDecoderForUrl( url, &reused );
    if( dec ) {
      m_analyserThread->m_decoder = dec;
      if( reused )
	slotAnalysingFinished( true );
      else
	m_analyserJob->start();
    }
    else {
      valid = false;
      m_unsupportedFiles.append( url.path() );
    }
  }

  // invalid file, next url
  if( !valid ) {
    m_urls.remove( m_urls.begin() );
    QTimer::singleShot( 0, this, SLOT(slotAddUrls()) );
  }
}


void K3bAudioTrackAddingDialog::slotAnalysingFinished( bool /*success*/ )
{
  if( m_bCanceled ) {
    // We only started the analyser thread in case the decoder was new
    // thus, we can safely delete it since no other source needs it.
    delete m_analyserThread->m_decoder;
    return;
  }

  KURL url = m_urls.first();
  m_urls.remove( m_urls.begin() );

  if( m_cueUrl.isValid() ) {
    // import the cue file
    m_doc->importCueFile( m_cueUrl.path(), m_trackAfter, m_analyserThread->m_decoder );
    m_cueUrl = KURL();
  }
  else {
    // create the track and source items
    K3bAudioDecoder* dec = m_analyserThread->m_decoder;
    K3bAudioFile* file = new K3bAudioFile( dec, m_doc );
    if( m_parentTrack ) {
      if( m_sourceAfter )
	file->moveAfter( m_sourceAfter );
      else
	file->moveAhead( m_parentTrack->firstSource() );
      m_sourceAfter = file;
    }
    else {
      K3bAudioTrack* track = new K3bAudioTrack( m_doc );
      track->setFirstSource( file );

      track->setTitle( dec->metaInfo( K3bAudioDecoder::META_TITLE ) );
      track->setArtist( dec->metaInfo( K3bAudioDecoder::META_ARTIST ) );
      track->setSongwriter( dec->metaInfo( K3bAudioDecoder::META_SONGWRITER ) );
      track->setComposer( dec->metaInfo( K3bAudioDecoder::META_COMPOSER ) );
      track->setCdTextMessage( dec->metaInfo( K3bAudioDecoder::META_COMMENT ) );

      if( m_trackAfter )
	track->moveAfter( m_trackAfter );
      else
	track->moveAfter( m_doc->lastTrack() );

      m_trackAfter = track;
    }
  }

  QTimer::singleShot( 0, this, SLOT(slotAddUrls()) );
}


void K3bAudioTrackAddingDialog::slotCancel()
{
  m_bCanceled = true;
  m_analyserJob->cancel();
  KDialogBase::slotCancel();
}


KURL::List K3bAudioTrackAddingDialog::extractUrlList( const KURL::List& urls )
{
  KURL::List allUrls = urls;
  KURL::List urlsFromPlaylist;
  KURL::List::iterator it = allUrls.begin();
  while( it != allUrls.end() ) {

    const KURL& url = *it;
    QFileInfo fi( url.path() );

    if( fi.isDir() ) {
      it = allUrls.remove( it );
      // add all files in the dir
      QDir dir(fi.filePath());
      QStringList entries = dir.entryList( QDir::Files );
      KURL::List::iterator oldIt = it;
      // add all files into the list after the current item
      for( QStringList::iterator dirIt = entries.begin();
	   dirIt != entries.end(); ++dirIt )
	it = allUrls.insert( oldIt, KURL::fromPathOrURL( dir.absPath() + "/" + *dirIt ) );
    }
    else if( K3bAudioDoc::readPlaylistFile( url, urlsFromPlaylist ) ) {
      it = allUrls.remove( it );
      KURL::List::iterator oldIt = it;
      // add all files into the list after the current item
      for( KURL::List::iterator dirIt = urlsFromPlaylist.begin();
	   dirIt != urlsFromPlaylist.end(); ++dirIt )
	it = allUrls.insert( oldIt, *dirIt );
    }
    else
      ++it;
  }

  return allUrls;
}

#include "k3baudiotrackaddingdialog.moc"
