/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include <k3bglobals.h>
#include "k3baudiodoc.h"
#include "k3baudioview.h"
#include "k3baudiotrack.h"
#include "k3baudioburndialog.h"
#include "k3baudiojob.h"
#include "k3baudiotitlemetainfo.h"
#include <songdb/k3bsong.h>
#include <songdb/k3bsongmanager.h>
#include <k3bthread.h>
#include <k3bthreadjob.h>
#include <k3bcore.h>
#include <k3bpluginmanager.h>
#include <k3baudiodecoder.h>


// QT-includes
#include <qstring.h>
#include <qstringlist.h>
#include <qfile.h>
#include <qdatastream.h>
#include <qdom.h>
#include <qdatetime.h>
#include <qtimer.h>
#include <qtextstream.h>

// KDE-includes
#include <kprocess.h>
#include <kurl.h>
#include <kapplication.h>
#include <kmessagebox.h>
#include <kconfig.h>
#include <klocale.h>
#include <kstandarddirs.h>
#include <kio/global.h>
#include <kdebug.h>

#include <iostream>


// this simple thread is just used to asynchronously determine the
// length and integrety of the tracks
class K3bAudioDoc::AudioTrackStatusThread : public K3bThread
{
public:
  AudioTrackStatusThread()
    : K3bThread(),
      m_track(0) {
  }

  void analyseTrack( K3bAudioTrack* track ) {
    m_track = track;
    start();
  }

  K3bAudioTrack* track() const {
    return m_track;
  }

protected:
  void run() {
    if( m_track->module()->analyseFile() )
      m_track->setStatus( K3bAudioTitleMetaInfo::OK );
    else
      m_track->setStatus( K3bAudioTitleMetaInfo::CORRUPT );
    emitFinished(true);
  }

private:
  K3bAudioTrack* m_track;
};



K3bAudioDoc::K3bAudioDoc( QObject* parent )
  : K3bDoc( parent )
{
  m_tracks = 0L;
  m_cdText = false;
  m_padding = true;  // padding is enabled forever since there is no use in disabling it!

  m_docType = AUDIO;

  m_urlAddingTimer = new QTimer( this );
  connect( m_urlAddingTimer, SIGNAL(timeout()), this, SLOT(slotWorkUrlQueue()) );

  m_trackStatusThread = new AudioTrackStatusThread();
  m_trackMetaInfoJob = new K3bThreadJob( this );
  m_trackMetaInfoJob->setThread( m_trackStatusThread );
  connect( m_trackMetaInfoJob, SIGNAL(finished(bool)),
	   this, SLOT(slotDetermineTrackStatus()) );

  // FIXME: remove the newTracks() signal and replace it with the changed signal
  connect( this, SIGNAL(newTracks()), this, SIGNAL(changed()) );
  connect( this, SIGNAL(trackRemoved(K3bAudioTrack*)), this, SIGNAL(changed()) );
}

K3bAudioDoc::~K3bAudioDoc()
{
  if( m_tracks )
    m_tracks->setAutoDelete( true );

  delete m_tracks;
  delete m_trackStatusThread;
}

bool K3bAudioDoc::newDocument()
{
  if( m_tracks ) {
    while( m_tracks->first() )
      removeTrack( m_tracks->first() );
  }
  else
    m_tracks = new QPtrList<K3bAudioTrack>;
  m_tracks->setAutoDelete(false);

  return K3bDoc::newDocument();
}



KIO::filesize_t K3bAudioDoc::size() const 
{
  // This is not really correct but what the user expects ;)
  return length().mode1Bytes();
}


K3b::Msf K3bAudioDoc::length() const
{
  K3b::Msf length = 0;
  for( QPtrListIterator<K3bAudioTrack> it(*m_tracks); it.current(); ++it ) {
    length += it.current()->length() + it.current()->pregap();
  }	

  return length;
}


void K3bAudioDoc::addUrls( const KURL::List& urls )
{
  addTracks( urls, m_tracks->count() );
}


void K3bAudioDoc::addTracks( const KURL::List& urls, uint position )
{
  for( KURL::List::ConstIterator it = urls.begin(); it != urls.end(); it++ ) {
    urlsToAdd.enqueue( new PrivateUrlToAdd( *it, position++ ) );
  }

  m_urlAddingTimer->start(0);
}

void K3bAudioDoc::slotWorkUrlQueue()
{
  if( !urlsToAdd.isEmpty() ) {
    PrivateUrlToAdd* item = urlsToAdd.dequeue();
    lastAddedPosition = item->position;

    // append at the end by default
    if( lastAddedPosition > m_tracks->count() )
      lastAddedPosition = m_tracks->count();

    if( !item->url.isLocalFile() ) {
      kdDebug() << item->url.path() << " no local file" << endl;
      return;
    }

    if( !QFile::exists( item->url.path() ) ) {
      m_notFoundFiles.append( item->url.path() );
      return;
    }

    if( !readM3uFile( item->url, lastAddedPosition ) )
      if( K3bAudioTrack* newTrack = createTrack( item->url ) ) {
        addTrack( newTrack, lastAddedPosition );
	determineAudioMetaInfo( newTrack );
	slotDetermineTrackStatus();
      }

    delete item;

    emit newTracks();
  }

  else {
    m_urlAddingTimer->stop();

    emit newTracks();

    informAboutNotFoundFiles();
  }
}


bool K3bAudioDoc::readM3uFile( const KURL& url, int pos )
{
  // check if the file is a m3u playlist
  // and if so add all listed files

  QFile f( url.path() );
  if( !f.open( IO_ReadOnly ) )
    return false;

  QTextStream t( &f );
  char buf[7];
  t.readRawBytes( buf, 7 );
  if( QString::fromLatin1( buf, 7 ) != "#EXTM3U" )
    return false;

  // skip the first line
  t.readLine();

  // read the file
  while( !t.atEnd() ) {
    QString line = t.readLine();
    if( line[0] != '#' ) {
      KURL mp3url;
      // relative paths
      if( line[0] != '/' )
        mp3url.setPath( url.directory(false) + line );
      else
        mp3url.setPath( line );
      urlsToAdd.enqueue( new PrivateUrlToAdd( mp3url , pos++ ) );
    }
  }

  m_urlAddingTimer->start(0);
  return true;
}


K3bAudioTrack* K3bAudioDoc::createTrack( const KURL& url )
{
  QPtrList<K3bPluginFactory> fl = k3bpluginmanager->factories( "AudioDecoder" );
  for( QPtrListIterator<K3bPluginFactory> it( fl );
       it.current(); ++it ) {
    if( ((K3bAudioDecoderFactory*)it.current())->canDecode( url ) ) {
      kdDebug() << "(K3bAudioDoc) using " << it.current()->className() << " for decoding of " << url.path() << endl;
      K3bAudioTrack* newTrack =  new K3bAudioTrack( m_tracks, url.path() );
      newTrack->setModule( (K3bAudioDecoder*)((K3bAudioDecoderFactory*)it.current())->createPlugin() );
      return newTrack;
    }
  }

  m_unknownFileFormatFiles.append( url.path() );
  return 0;
}


void K3bAudioDoc::addTrack(const KURL& url, uint position )
{
  urlsToAdd.enqueue( new PrivateUrlToAdd( url, position ) );

  m_urlAddingTimer->start(0);
}



void K3bAudioDoc::addTrack( K3bAudioTrack* track, uint position )
{
  if( m_tracks->count() >= 99 ) {
    kdDebug() << "(K3bAudioDoc) Red Book only allows 99 tracks." << endl;
    // TODO: show some messagebox
    delete track;
    return;
  }

  lastAddedPosition = position;

  if( !m_tracks->insert( position, track ) ) {
    lastAddedPosition = m_tracks->count();
    m_tracks->insert( m_tracks->count(), track );
  }

  emit newTracks();
  
  setModified( true );
}


void K3bAudioDoc::removeTrack( K3bAudioTrack* track )
{
  if( !track ) {
    return;
  }
	
  // set the current item to track
  if( m_tracks->findRef( track ) >= 0 ) {
    // take the current item
    track = m_tracks->take();

    // if the AudioTrackStatusThread currently works this file we kill and restart it
    if( m_trackStatusThread->track() == track && m_trackStatusThread->running() )
      m_trackMetaInfoJob->cancel(); // this will emit a finished signal
      
    // emit signal before deleting the track to avoid crashes
    // when the view tries to call some of the tracks' methods
    emit trackRemoved( track );
      
    delete track;

    // now make sure the first track has a pregap of at least 150 frames
    if( m_tracks->first() )
      m_tracks->first()->setPregap( m_tracks->first()->pregap() );

    setModified();
  }
}

void K3bAudioDoc::moveTrack( const K3bAudioTrack* track, const K3bAudioTrack* after )
{
  if( track == after )
    return;

  // set the current item to track
  m_tracks->findRef( track );
  // take the current item
  track = m_tracks->take();

  // if after == 0 findRef returnes -1
  int pos = m_tracks->findRef( after );
  m_tracks->insert( pos+1, track );

  // now make sure the first track has a pregap of at least 150 frames
  m_tracks->first()->setPregap( m_tracks->first()->pregap() );

  setModified();
}


K3bView* K3bAudioDoc::newView( QWidget* parent )
{
  return new K3bAudioView( this, parent );
}


QString K3bAudioDoc::documentType() const
{
  return "audio";
}


bool K3bAudioDoc::loadDocumentData( QDomElement* root )
{
  newDocument();

  // we will parse the dom-tree and create a K3bAudioTrack for all entries immediately
  // this should not take long and so not block the gui

  QDomNodeList nodes = root->childNodes();

  for( uint i = 0; i < nodes.count(); i++ ) {

    QDomElement e = nodes.item(i).toElement();

    if( e.isNull() )
      return false;
    
    if( e.nodeName() == "general" ) {
      if( !readGeneralDocumentData( e ) )
	return false;
    }

    else if( e.nodeName() == "normalize" )
      setNormalize( e.text() == "yes" );
    
    else if( e.nodeName() == "hide_first_track" )
      setHideFirstTrack( e.text() == "yes" );
    

    // parse cd-text
    else if( e.nodeName() == "cd-text" ) {
      if( !e.hasAttribute( "activated" ) )
	return false;

      writeCdText( e.attributeNode( "activated" ).value() == "yes" );
    
      QDomNodeList cdTextNodes = e.childNodes();
      setTitle( cdTextNodes.item(0).toElement().text() );
      setArtist( cdTextNodes.item(1).toElement().text() );
      setArranger( cdTextNodes.item(2).toElement().text() );
      setSongwriter( cdTextNodes.item(3).toElement().text() );
      setDisc_id( cdTextNodes.item(4).toElement().text() );
      setUpc_ean( cdTextNodes.item(5).toElement().text() );
      setCdTextMessage( cdTextNodes.item(6).toElement().text() );
    }

    else if( e.nodeName() == "contents" ) {
	
      QDomNodeList trackNodes = e.childNodes();

      for( uint j = 0; j< trackNodes.length(); j++ ) {

	// check if url is available
	QDomElement trackElem = trackNodes.item(j).toElement();
	QString url = trackElem.attributeNode( "url" ).value();
	if( !QFile::exists( url ) )
	  m_notFoundFiles.append( url );
	else {
	  KURL k;
	  k.setPath( url );
	  if( K3bAudioTrack* track = createTrack( k ) ) {
	    
	    QDomNodeList trackNodes = trackElem.childNodes();
	    // set cd-text
	    QDomElement cdTextElem = trackNodes.item(0).toElement();
	    QDomNodeList cdTextNodes = cdTextElem.childNodes();
	    track->setTitle( cdTextNodes.item(0).toElement().text() );
	    track->setArtist( cdTextNodes.item(1).toElement().text() );
	    track->setArranger( cdTextNodes.item(2).toElement().text() );
	    track->setSongwriter( cdTextNodes.item(3).toElement().text() );
	    track->setIsrc( cdTextNodes.item(4).toElement().text() );
	    track->setCdTextMessage( cdTextNodes.item(5).toElement().text() );
	    
	    // set pregap
	    QDomElement pregapElem = trackNodes.item(1).toElement();
	    track->setPregap( pregapElem.text().toInt() );
	    
	    // set copy-protection
	    QDomElement copyProtectElem = trackNodes.item(2).toElement();
	    track->setCopyProtection( copyProtectElem.text() == "yes" );
	    
	    // set pre-emphasis
	    QDomElement preEmpElem = trackNodes.item(3).toElement();
	    track->setPreEmp( preEmpElem.text() == "yes" );
	    
	    addTrack( track, m_tracks->count() );
	  }
	}
      }
    }
  }

  emit newTracks();

  slotDetermineTrackStatus();

  informAboutNotFoundFiles();

  if ( m_notFoundFiles.isEmpty() )
    setModified(false);

  return true;
}

bool K3bAudioDoc::saveDocumentData( QDomElement* docElem )
{
  QDomDocument doc = docElem->ownerDocument();
  saveGeneralDocumentData( docElem );

  // add normalize
  QDomElement normalizeElem = doc.createElement( "normalize" );
  normalizeElem.appendChild( doc.createTextNode( normalize() ? "yes" : "no" ) );
  docElem->appendChild( normalizeElem );

  // add hide track
  QDomElement hideFirstTrackElem = doc.createElement( "hide_first_track" );
  hideFirstTrackElem.appendChild( doc.createTextNode( hideFirstTrack() ? "yes" : "no" ) );
  docElem->appendChild( hideFirstTrackElem );


  // save disc cd-text
  // -------------------------------------------------------------
  QDomElement cdTextMain = doc.createElement( "cd-text" );
  cdTextMain.setAttribute( "activated", cdText() ? "yes" : "no" );
  QDomElement cdTextElem = doc.createElement( "title" );
  cdTextElem.appendChild( doc.createTextNode( (title())) );
  cdTextMain.appendChild( cdTextElem );

  cdTextElem = doc.createElement( "artist" );
  cdTextElem.appendChild( doc.createTextNode( (artist())) );
  cdTextMain.appendChild( cdTextElem );

  cdTextElem = doc.createElement( "arranger" );
  cdTextElem.appendChild( doc.createTextNode( (arranger())) );
  cdTextMain.appendChild( cdTextElem );

  cdTextElem = doc.createElement( "songwriter" );
  cdTextElem.appendChild( doc.createTextNode( (songwriter())) );
  cdTextMain.appendChild( cdTextElem );

  cdTextElem = doc.createElement( "disc_id" );
  cdTextElem.appendChild( doc.createTextNode( (disc_id())) );
  cdTextMain.appendChild( cdTextElem );

  cdTextElem = doc.createElement( "upc_ean" );
  cdTextElem.appendChild( doc.createTextNode( (upc_ean())) );
  cdTextMain.appendChild( cdTextElem );

  cdTextElem = doc.createElement( "message" );
  cdTextElem.appendChild( doc.createTextNode( (cdTextMessage())) );
  cdTextMain.appendChild( cdTextElem );

  docElem->appendChild( cdTextMain );
  // -------------------------------------------------------------

  // save the tracks
  // -------------------------------------------------------------
  QDomElement contentsElem = doc.createElement( "contents" );

  for( K3bAudioTrack* track = first(); track != 0; track = next() ) {

    QDomElement trackElem = doc.createElement( "track" );
    trackElem.setAttribute( "url", KIO::decodeFileName(track->absPath()) );

    // add cd-text
    cdTextMain = doc.createElement( "cd-text" );
    cdTextElem = doc.createElement( "title" );
    cdTextElem.appendChild( doc.createTextNode( (track->title())) );
    cdTextMain.appendChild( cdTextElem );
    
    cdTextElem = doc.createElement( "artist" );
    cdTextElem.appendChild( doc.createTextNode( (track->artist())) );
    cdTextMain.appendChild( cdTextElem );
    
    cdTextElem = doc.createElement( "arranger" );
    cdTextElem.appendChild( doc.createTextNode( (track->arranger()) ) );
    cdTextMain.appendChild( cdTextElem );
    
    cdTextElem = doc.createElement( "songwriter" );
    cdTextElem.appendChild( doc.createTextNode( (track->songwriter()) ) );
    cdTextMain.appendChild( cdTextElem );
    
    cdTextElem = doc.createElement( "isrc" );
    cdTextElem.appendChild( doc.createTextNode( ( track->isrc()) ) );
    cdTextMain.appendChild( cdTextElem );
    
    cdTextElem = doc.createElement( "message" );
    cdTextElem.appendChild( doc.createTextNode( (track->cdTextMessage()) ) );
    cdTextMain.appendChild( cdTextElem );

    trackElem.appendChild( cdTextMain );


    // add pregap
    QDomElement pregapElem = doc.createElement( "pregap" );    
    pregapElem.appendChild( doc.createTextNode( QString::number(track->pregap().totalFrames()) ) );
    trackElem.appendChild( pregapElem );

    // add copy protection
    QDomElement copyElem = doc.createElement( "copy_protection" );    
    copyElem.appendChild( doc.createTextNode( track->copyProtection() ? "yes" : "no" ) );
    trackElem.appendChild( copyElem );

    // add pre emphasis
    copyElem = doc.createElement( "pre_emphasis" );    
    copyElem.appendChild( doc.createTextNode( track->preEmp() ? "yes" : "no" ) );
    trackElem.appendChild( copyElem );

    contentsElem.appendChild( trackElem );
  }
  // -------------------------------------------------------------

  docElem->appendChild( contentsElem );

  return true;
}


int K3bAudioDoc::numOfTracks() const
{
  return m_tracks->count();
}


bool K3bAudioDoc::padding() const
{
  return m_padding;
}


K3bBurnJob* K3bAudioDoc::newBurnJob()
{
  return new K3bAudioJob( this );
}


void K3bAudioDoc::informAboutNotFoundFiles()
{
  if( !m_notFoundFiles.isEmpty() ) {
    KMessageBox::informationList( view(), i18n("Could not find the following files:"),
 				  m_notFoundFiles, i18n("Not Found") );

    m_notFoundFiles.clear();
  }
  if( !m_unknownFileFormatFiles.isEmpty() ) {
    KMessageBox::informationList( view(), i18n("Unable to handle the following files due to an unsupported format:"),
 				  m_unknownFileFormatFiles, i18n("Unsupported format") );

    m_unknownFileFormatFiles.clear();
  }
}


void K3bAudioDoc::loadDefaultSettings( KConfig* c )
{
  K3bDoc::loadDefaultSettings(c);

  m_cdText = c->readBoolEntry( "cd_text", false );
  m_padding = true;  // padding is always a good idea!
  m_hideFirstTrack = c->readBoolEntry( "hide_first_track", false );
  setNormalize( c->readBoolEntry( "normalize", false ) );
}


void K3bAudioDoc::removeCorruptTracks()
{
  K3bAudioTrack* track = m_tracks->first();
  while( track ) {
    if( track->status() == K3bAudioTitleMetaInfo::CORRUPT ) {
      removeTrack(track);
      track = m_tracks->current();
    }
    else
      track = m_tracks->next();
  }
}


void K3bAudioDoc::slotDetermineTrackStatus()
{
  kdDebug() << "(K3bAudioDoc) slotDetermineTrackStatus()" << endl;
  if( !m_trackStatusThread->running() ) {
    kdDebug() << "(K3bAudioDoc) AudioTrackStatusThread not running." << endl;
    // find the next track to meta-info
    for( QPtrListIterator<K3bAudioTrack> it( *m_tracks ); *it; ++it ) {
      if( it.current()->length() == 0 && it.current()->status() != K3bAudioTitleMetaInfo::CORRUPT ) {
	kdDebug() << "(K3bAudioDoc) starting AudioTrackStatusThread for " << it.current()->absPath() << endl;
	m_trackStatusThread->analyseTrack( it.current() );
	return;
      }
    }
  }
  else
    kdDebug() << "(K3bAudioDoc) AudioTrackStatusThread running." << endl;
}


void K3bAudioDoc::determineAudioMetaInfo( K3bAudioTrack* track )
{
  // first search the songdb
  K3bSong *song = K3bSongManager::instance()->findSong( track->absPath() );
  if( song != 0 ){
    track->setArtist( song->getArtist() );
    //      newTrack->setAlbum( song->getAlbum() );
    track->setTitle( song->getTitle() );
  }
  else {
    // no song found, try the module
    track->setTitle( track->module()->metaInfo( "Title" ) );
    track->setPerformer( track->module()->metaInfo( "Artist" ) );
    track->setComposer( track->module()->metaInfo( "Composer" ) );
    track->setSongwriter( track->module()->metaInfo( "Songwriter" ) );
    track->setCdTextMessage( track->module()->metaInfo( "Comment" ) );
  }
}


K3bProjectBurnDialog* K3bAudioDoc::newBurnDialog( QWidget* parent, const char* name )
{
  return new K3bAudioBurnDialog( this, parent, name, true );
}

#include "k3baudiodoc.moc"
