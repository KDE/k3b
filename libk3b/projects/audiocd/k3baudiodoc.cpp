/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
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
#include "k3baudiofile.h"
#include "k3baudiozerodata.h"
#include <k3bcuefileparser.h>

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
#include <qfileinfo.h>
#include <qdatastream.h>
#include <qdir.h>
#include <qdom.h>
#include <qdatetime.h>
#include <qtimer.h>
#include <qtextstream.h>
#include <qsemaphore.h>

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


static QSemaphore s_threadCnt( 10 );

// this simple thread is just used to asynchronously determine the
// length and integrety of the tracks
class K3bAudioDoc::AudioFileAnalyzerThread : public QThread
{
public:
  AudioFileAnalyzerThread( K3bAudioDecoder* dec ) 
    : m_decoder(dec) {
  }

  K3bAudioDecoder* m_decoder;

protected:
  void run() {
    s_threadCnt++;
    kdDebug() << "(AudioFileAnalyzerThread) running on decoder for " << m_decoder->filename() << endl;
    m_decoder->analyseFile();
    kdDebug() << "(AudioFileAnalyzerThread) finished for " << m_decoder->filename() << endl;
    s_threadCnt--;
  }
};



K3bAudioDoc::K3bAudioDoc( QObject* parent )
  : K3bDoc( parent ),
    m_firstTrack(0),
    m_lastTrack(0)
{
  m_cdText = false;

  m_docType = AUDIO;
  m_audioTrackStatusThreads.setAutoDelete(true);
}

K3bAudioDoc::~K3bAudioDoc()
{
  // delete all tracks
  while( m_firstTrack )
    delete m_firstTrack->take();
}

bool K3bAudioDoc::newDocument()
{
  return K3bDoc::newDocument();
}


K3bAudioTrack* K3bAudioDoc::firstTrack() const
{
  return m_firstTrack;
}


K3bAudioTrack* K3bAudioDoc::lastTrack() const
{
  return m_lastTrack;
}


// this one is called by K3bAudioTrack to update the list
void K3bAudioDoc::setFirstTrack( K3bAudioTrack* track )
{
  m_firstTrack = track;
}

// this one is called by K3bAudioTrack to update the list
void K3bAudioDoc::setLastTrack( K3bAudioTrack* track )
{
  m_lastTrack = track;
}


KIO::filesize_t K3bAudioDoc::size() const 
{
  // This is not really correct but what the user expects ;)
  return length().mode1Bytes();
}


K3b::Msf K3bAudioDoc::length() const
{
  K3b::Msf length = 0;
  K3bAudioTrack* track = m_firstTrack;
  while( track ) {
    length += track->length();
    track = track->next();
  }

  return length;
}


void K3bAudioDoc::addUrls( const KURL::List& urls )
{
  // make sure we add them at the end even if urls are in the queue
  addTracks( urls, 99 );
}


void K3bAudioDoc::addTracks( const KURL::List& urls, uint position )
{
  KURL::List allUrls = extractUrlList( urls );
  for( KURL::List::iterator it = allUrls.begin(); it != allUrls.end(); it++, position++ ) {
    KURL& url = *it;
    if( url.path().right(3).lower() == "cue" ) {
      // try adding a cue file
      if( K3bAudioTrack* newAfter = importCueFile( url.path(), getTrack(position-1) ) ) {
	position = newAfter->index()+1;
	continue;
      }
    }
    
    if( K3bAudioTrack* track = createTrack( url ) ) {
      addTrack( track, position );

      // first search the songdb
      K3bSong *song = K3bSongManager::instance()->findSong( url.path() );
      if( song && !song->getArtist().isEmpty() && !song->getTitle().isEmpty() ){
	track->setArtist( song->getArtist() );
	track->setTitle( song->getTitle() );
      }
      else {

	//
	// We need some evil hacking here because the meta info in the decoder will
	// not be ready before it did not finish the analysing. So we somehow need to
	// determine when this happens. With the current design that's a problem.
	// So we check here if the decoder already finished and if so set the meta info
	// right away. If not we set it in the housekeeping slot before the analysing
	// thread is deleted.
	//

	K3bAudioDecoder* dec = static_cast<K3bAudioFile*>( track->firstSource() )->decoder();
	if( dec->length() == 0 && dec->isValid() )
	  m_decoderMetaInfoSetMap[dec].append( track );
	else {
	  track->setTitle( dec->metaInfo( K3bAudioDecoder::META_TITLE ) );
	  track->setArtist( dec->metaInfo( K3bAudioDecoder::META_ARTIST ) );
	  track->setSongwriter( dec->metaInfo( K3bAudioDecoder::META_SONGWRITER ) );
	  track->setComposer( dec->metaInfo( K3bAudioDecoder::META_COMPOSER ) );
	  track->setCdTextMessage( dec->metaInfo( K3bAudioDecoder::META_COMMENT ) );
	}
      }
    }
  }
  
  emit changed();

  informAboutNotFoundFiles();
}


KURL::List K3bAudioDoc::extractUrlList( const KURL::List& urls )
{
  KURL::List allUrls = urls;
  KURL::List urlsFromPlaylist;
  KURL::List::iterator it = allUrls.begin();
  while( it != allUrls.end() ) {

    const KURL& url = *it;
    QFileInfo fi( url.path() );

    if( !url.isLocalFile() ) {
      kdDebug() << url.path() << " no local file" << endl;
      it = allUrls.remove( it );
      m_notFoundFiles.append( url );
    }
    else if( !fi.exists() ) {
      it = allUrls.remove( it );
      m_notFoundFiles.append( url );
    }
    else if( fi.isDir() ) {
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
    else if( readM3uFile( url, urlsFromPlaylist ) ) {
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


bool K3bAudioDoc::readM3uFile( const KURL& url, KURL::List& playlist )
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

      playlist.append( mp3url );
    }
  }

  return true;
}


void K3bAudioDoc::addSources( K3bAudioTrack* parent, 
			      const KURL::List& urls, 
			      K3bAudioDataSource* sourceAfter )
{
  kdDebug() << "(K3bAudioDoc::addSources( " << parent << ", "
	    << urls.first().path() << ", " 
	    << sourceAfter << " )" << endl;
  KURL::List allUrls = extractUrlList( urls );
  for( KURL::List::const_iterator it = allUrls.begin(); it != allUrls.end(); it++ ) {
    if( K3bAudioFile* file = createAudioFile( *it ) ) {
      if( sourceAfter )
	file->moveAfter( sourceAfter );
      else
	file->moveAhead( parent->firstSource() );
      sourceAfter = file;
    }
  }

  informAboutNotFoundFiles();
  kdDebug() << "(K3bAudioDoc::addSources) finished." << endl;
}


K3bAudioTrack* K3bAudioDoc::importCueFile( const QString& cuefile, K3bAudioTrack* after )
{
  kdDebug() << "(K3bAudioDoc::importCueFile( " << cuefile << ", " << after << ")" << endl;
  K3bCueFileParser parser( cuefile );
  if( parser.isValid() && parser.toc().contentType() == K3bCdDevice::AUDIO ) {

    kdDebug() << "(K3bAudioDoc::importCueFile) parsed with image: " << parser.imageFilename() << endl;

    // global cd-text
    if( !parser.cdText().title().isEmpty() )
      setTitle( parser.cdText().title() );
    if( !parser.cdText().performer().isEmpty() )
      setPerformer( parser.cdText().performer() );

    K3bAudioDecoder* decoder = getDecoderForUrl( parser.imageFilename() );
    if( decoder ) {
      K3bAudioFile* newFile = 0;
      unsigned int i = 0;
      for( K3bCdDevice::Toc::const_iterator it = parser.toc().begin();
	   it != parser.toc().end(); ++it ) {
	const K3bCdDevice::Track& track = *it;

	newFile = new K3bAudioFile( decoder, this );
	newFile->setStartOffset( track.firstSector() );
	newFile->setEndOffset( track.lastSector()+1 );

	K3bAudioTrack* newTrack = new K3bAudioTrack( this );
	newTrack->addSource( newFile );
	newTrack->moveAfter( after );

	// cd-text
	newTrack->setTitle( parser.cdText()[i].title() );
	newTrack->setPerformer( parser.cdText()[i].performer() );

	// add the next track after this one
	after = newTrack;
	++i;
      }

      // let the last source use the data up to the end of the file
      if( newFile )
	newFile->setEndOffset(0);

      return after;
    }
  }
  return 0;
}


K3bAudioDecoder* K3bAudioDoc::getDecoderForUrl( const KURL& url )
{
  K3bAudioDecoder* decoder = 0;
  // check if we already have a proper decoder
  if( m_decoderPresenceMap.contains( url.path() ) )
    decoder = m_decoderPresenceMap[url.path()];

  // if not create one
  if( !decoder ) {
    QPtrList<K3bPluginFactory> fl = k3bpluginmanager->factories( "AudioDecoder" );
    for( QPtrListIterator<K3bPluginFactory> it( fl ); it.current(); ++it ) {
      K3bAudioDecoderFactory* f = static_cast<K3bAudioDecoderFactory*>( it.current() );
      if( f->canDecode( url ) ) {
	kdDebug() << "(K3bAudioDoc) using " << it.current()->className()
		  << " for decoding of " << url.path() << endl;
	
	decoder = static_cast<K3bAudioDecoder*>(f->createPlugin());
	decoder->setFilename( url.path() );

	//
	// start a thread to analyse the file
	//
	AudioFileAnalyzerThread* thread = new AudioFileAnalyzerThread( decoder );
	thread->start();
	m_audioTrackStatusThreads.append( thread );
	QTimer::singleShot( 500, this, SLOT(slotHouseKeeping()) );
	return decoder;
      }
    }
  }

  return decoder;
}


K3bAudioFile* K3bAudioDoc::createAudioFile( const KURL& url )
{
  kdDebug() << "(K3bAudioDoc::createAudioFile( " << url.path() << " )" << endl;
  K3bAudioDecoder* decoder = getDecoderForUrl( url );
  if( decoder ) {
    return new K3bAudioFile( decoder, this );
  }
  else {
    m_unknownFileFormatFiles.append( url.path() );
    return 0;
  }
}


K3bAudioTrack* K3bAudioDoc::createTrack( const KURL& url )
{
  kdDebug() << "(K3bAudioDoc::createTrack( " << url.path() << " )" << endl;
  if( K3bAudioFile* file = createAudioFile( url ) ) {
    K3bAudioTrack* newTrack = new K3bAudioTrack( this );
    newTrack->setFirstSource( file );
    return newTrack;
  }
  else
    return 0;
}


void K3bAudioDoc::addTrack( const KURL& url, uint position )
{
  addTracks( KURL::List(url), position );
}



K3bAudioTrack* K3bAudioDoc::getTrack( unsigned int index )
{
  K3bAudioTrack* track = m_firstTrack;
  while( track ) {
    if( track->index() == index )
      return track;
    track = track->next();
  }

  return m_lastTrack;
}


void K3bAudioDoc::addTrack( K3bAudioTrack* track, uint position )
{
  kdDebug() << "(K3bAudioDoc::addTrack( " << track << ", " << position << " )" << endl;
  if( !m_firstTrack )
    m_firstTrack = m_lastTrack = track;
  else if( position == 0 )
    track->moveAhead( m_firstTrack );
  else {
    K3bAudioTrack* after = getTrack( position-1 );
    if( after )
      track->moveAfter( after );
    else
      track->moveAfter( m_lastTrack );  // just to be sure it's anywhere...
  }
}


void K3bAudioDoc::removeTrack( K3bAudioTrack* track )
{
  delete track;
}


void K3bAudioDoc::moveTrack( K3bAudioTrack* track, K3bAudioTrack* after )
{
  track->moveAfter( after );
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
      for( uint j = 0; j < cdTextNodes.length(); j++ ) {
	if( cdTextNodes.item(j).nodeName() == "title" )
	  setTitle( cdTextNodes.item(j).toElement().text() );

	else if( cdTextNodes.item(j).nodeName() == "artist" )
	  setArtist( cdTextNodes.item(j).toElement().text() );

	else if( cdTextNodes.item(j).nodeName() == "arranger" )
	  setArranger( cdTextNodes.item(j).toElement().text() );

	else if( cdTextNodes.item(j).nodeName() == "songwriter" )
	  setSongwriter( cdTextNodes.item(j).toElement().text() );

	else if( cdTextNodes.item(j).nodeName() == "composer" )
	  setComposer( cdTextNodes.item(j).toElement().text() );

	else if( cdTextNodes.item(j).nodeName() == "disc_id" )
	  setDisc_id( cdTextNodes.item(j).toElement().text() );

	else if( cdTextNodes.item(j).nodeName() == "upc_ean" )
	  setUpc_ean( cdTextNodes.item(j).toElement().text() );

	else if( cdTextNodes.item(j).nodeName() == "message" )
	  setCdTextMessage( cdTextNodes.item(j).toElement().text() );
      }
    }

    else if( e.nodeName() == "contents" ) {
	
      QDomNodeList contentNodes = e.childNodes();

      for( uint j = 0; j< contentNodes.length(); j++ ) {

	QDomElement trackElem = contentNodes.item(j).toElement();

	// first of all we need a track
	K3bAudioTrack* track = new K3bAudioTrack( this );

	QDomNodeList trackNodes = trackElem.childNodes();
	for( uint trackJ = 0; trackJ < trackNodes.length(); trackJ++ ) {

	  if( trackNodes.item(trackJ).nodeName() == "sources" ) {
	    QDomNodeList sourcesNodes = trackNodes.item(trackJ).childNodes();
	    for( unsigned int sourcesIndex = 0; sourcesIndex < sourcesNodes.length(); sourcesIndex++ ) {
	      QDomElement sourceElem = sourcesNodes.item(sourcesIndex).toElement();
	      if( sourceElem.nodeName() == "file" ) {
		if( K3bAudioFile* file = 
		    createAudioFile( KURL::fromPathOrURL( sourceElem.attributeNode( "url" ).value() ) ) ) {
		  file->setStartOffset( K3b::Msf::fromString( sourceElem.attributeNode( "start_offset" ).value() ) );
		  file->setEndOffset( K3b::Msf::fromString( sourceElem.attributeNode( "end_offset" ).value() ) );
		  track->addSource( file );
		}
	      }
	      else if( sourceElem.nodeName() == "silence" ) {
		K3bAudioZeroData* zero = new K3bAudioZeroData( this );
		zero->setLength( K3b::Msf::fromString( sourceElem.attributeNode( "length" ).value() ) );
		track->addSource( zero );
	      }
	      else {
		kdDebug() << "(K3bAudioDoc) unknown source type: " << sourceElem.nodeName() << endl;
		return false;
	      }
	    }
	  }

	  // load cd-text
	  else if( trackNodes.item(trackJ).nodeName() == "cd-text" ) {
	    QDomNodeList cdTextNodes = trackNodes.item(trackJ).childNodes();
	    for( uint trackCdTextJ = 0; trackCdTextJ < cdTextNodes.length(); trackCdTextJ++ ) {
	      if( cdTextNodes.item(trackCdTextJ).nodeName() == "title" )
		track->setTitle( cdTextNodes.item(trackCdTextJ).toElement().text() );
	      
	      else if( cdTextNodes.item(trackCdTextJ).nodeName() == "artist" )
		track->setArtist( cdTextNodes.item(trackCdTextJ).toElement().text() );
	      
	      else if( cdTextNodes.item(trackCdTextJ).nodeName() == "arranger" )
		track->setArranger( cdTextNodes.item(trackCdTextJ).toElement().text() );
	      
	      else if( cdTextNodes.item(trackCdTextJ).nodeName() == "songwriter" )
		track->setSongwriter( cdTextNodes.item(trackCdTextJ).toElement().text() );
	      
	      else if( cdTextNodes.item(trackCdTextJ).nodeName() == "composer" )
		track->setComposer( cdTextNodes.item(trackCdTextJ).toElement().text() );
	      
	      else if( cdTextNodes.item(trackCdTextJ).nodeName() == "isrc" )
		track->setIsrc( cdTextNodes.item(trackCdTextJ).toElement().text() );
	      
	      else if( cdTextNodes.item(trackCdTextJ).nodeName() == "message" )
		track->setCdTextMessage( cdTextNodes.item(trackCdTextJ).toElement().text() );
	    }
	  }

	  // load options
	  else if( trackNodes.item(trackJ).nodeName() == "copy_protection" )
	    track->setCopyProtection( trackNodes.item(trackJ).toElement().text() == "yes" );

	  else if( trackNodes.item(trackJ).nodeName() == "pre_emphasis" )
	    track->setPreEmp( trackNodes.item(trackJ).toElement().text() == "yes" );
	}

	// add the track
	if( track->numberSources() > 0 )
	  addTrack( track, 99 ); // append to the end // TODO improve
	else
	  delete track;
      }
    }
  }

  emit changed();

  informAboutNotFoundFiles();

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

  cdTextElem = doc.createElement( "composer" );
  cdTextElem.appendChild( doc.createTextNode( composer()) );
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

  for( K3bAudioTrack* track = firstTrack(); track != 0; track = track->next() ) {

    QDomElement trackElem = doc.createElement( "track" );

    // add sources
    QDomElement sourcesParent = doc.createElement( "sources" );

    for( K3bAudioDataSource* source = track->firstSource(); source; source = source->next() ) {
      // TODO: better have something like isFile() or isSilence()
      if( K3bAudioFile* file = dynamic_cast<K3bAudioFile*>(source) ) {
	QDomElement sourceElem = doc.createElement( "file" );
	sourceElem.setAttribute( "url", file->filename() );
	sourceElem.setAttribute( "start_offset", file->startOffset().toString() );
	sourceElem.setAttribute( "end_offset", file->endOffset().toString() );
	sourcesParent.appendChild( sourceElem );
      }
      else {
	K3bAudioZeroData* zero = static_cast<K3bAudioZeroData*>(source);
	QDomElement sourceElem = doc.createElement( "silence" );
	sourceElem.setAttribute( "length", zero->length().toString() );
	sourcesParent.appendChild( sourceElem );
      }
    }
    trackElem.appendChild( sourcesParent );

    // index 0
    QDomElement index0Elem = doc.createElement( "index0" );
    index0Elem.appendChild( doc.createTextNode( track->index0().toString() ) );
    trackElem.appendChild( index0Elem );

    // TODO: other indices

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

    cdTextElem = doc.createElement( "composer" );
    cdTextElem.appendChild( doc.createTextNode( (track->composer()) ) );
    cdTextMain.appendChild( cdTextElem );
    
    cdTextElem = doc.createElement( "isrc" );
    cdTextElem.appendChild( doc.createTextNode( ( track->isrc()) ) );
    cdTextMain.appendChild( cdTextElem );
    
    cdTextElem = doc.createElement( "message" );
    cdTextElem.appendChild( doc.createTextNode( (track->cdTextMessage()) ) );
    cdTextMain.appendChild( cdTextElem );

    trackElem.appendChild( cdTextMain );

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
  return ( m_lastTrack ? m_lastTrack->index()+1 : 0 );
}


K3bBurnJob* K3bAudioDoc::newBurnJob( K3bJobHandler* hdl, QObject* parent )
{
  return new K3bAudioJob( this, hdl, parent );
}


void K3bAudioDoc::informAboutNotFoundFiles()
{
  if( !m_notFoundFiles.isEmpty() ) {
    QStringList l;
    for( KURL::List::const_iterator it = m_notFoundFiles.begin(); 
	 it != m_notFoundFiles.end(); ++it )
      l.append( (*it).path() );
    KMessageBox::informationList( qApp->activeWindow(), 
				  i18n("Could not find the following files:"),
 				  l,
				  i18n("Not Found") );
    
    m_notFoundFiles.clear();
  }
  if( !m_unknownFileFormatFiles.isEmpty() ) {
    QStringList l;
    for( KURL::List::const_iterator it = m_unknownFileFormatFiles.begin(); 
	 it != m_unknownFileFormatFiles.end(); ++it )
      l.append( (*it).path() );
    KMessageBox::informationList( qApp->activeWindow(), 
				  i18n("Unable to handle the following files due to an unsupported format:"),
 				  l,
				  i18n("Unsupported Format") );

    m_unknownFileFormatFiles.clear();
  }
}


void K3bAudioDoc::loadDefaultSettings( KConfig* c )
{
  K3bDoc::loadDefaultSettings(c);

  m_cdText = c->readBoolEntry( "cd_text", false );
  m_hideFirstTrack = c->readBoolEntry( "hide_first_track", false );
  setNormalize( c->readBoolEntry( "normalize", false ) );
}


void K3bAudioDoc::removeCorruptTracks()
{
//   K3bAudioTrack* track = m_tracks->first();
//   while( track ) {
//     if( track->status() != 0 ) {
//       removeTrack(track);
//       track = m_tracks->current();
//     }
//     else
//       track = m_tracks->next();
//   }
}


K3bProjectBurnDialog* K3bAudioDoc::newBurnDialog( QWidget* parent, const char* name )
{
  return new K3bAudioBurnDialog( this, parent, name, true );
}


void K3bAudioDoc::slotTrackChanged( K3bAudioTrack* track )
{
  setModified( true );
  emit changed();
  // if the track is empty now we simply delete it
  if( track->firstSource() )
    emit trackChanged(track);
  else {
    kdDebug() << "(K3bAudioTrack::slotTrackChanged) track " << track << " empty. Deleting." << endl;
    delete track; // this will emit the proper signal
  }
}


void K3bAudioDoc::slotTrackDestroyed( QObject* o )
{
  setModified( true );
  K3bAudioTrack* track = static_cast<K3bAudioTrack*>(o);
  emit trackRemoved(track);
  emit changed();
}


void K3bAudioDoc::increaseDecoderUsage( K3bAudioDecoder* decoder )
{
  kdDebug() << "(K3bAudioDoc::increaseDecoderUsage)" << endl;
  if( !m_decoderUsageCounterMap.contains( decoder ) ) {
    m_decoderUsageCounterMap[decoder] = 1;
    m_decoderPresenceMap[decoder->filename()] = decoder;
  }
  else
    m_decoderUsageCounterMap[decoder]++;
  kdDebug() << "(K3bAudioDoc::increaseDecoderUsage) finished" << endl;
}


void K3bAudioDoc::decreaseDecoderUsage( K3bAudioDecoder* decoder )
{
  // FIXME: what if the thread did not finish yet?
  m_decoderUsageCounterMap[decoder]--;
  if( m_decoderUsageCounterMap[decoder] <= 0 ) {
    m_decoderUsageCounterMap.erase(decoder);
    m_decoderPresenceMap.erase(decoder->filename());
    m_decoderMetaInfoSetMap.erase(decoder);
    delete decoder;
  }
}


void K3bAudioDoc::slotHouseKeeping()
{
  kdDebug() << "(K3bAudioDoc::slotHouseKeeping)" << endl;
  for( QPtrListIterator<AudioFileAnalyzerThread> it( m_audioTrackStatusThreads );
       it.current(); ++it ) {
    AudioFileAnalyzerThread* thread = *it;
    if( thread->finished() ) {
      kdDebug() << "(K3bAudioDoc::slotHouseKeeping) removing thread for "
		<< thread->m_decoder->filename() << endl;

      const QPtrList<K3bAudioTrack>& tracks = m_decoderMetaInfoSetMap[thread->m_decoder];
      for( QPtrListIterator<K3bAudioTrack> it( tracks ); *it; ++it ) {
	K3bAudioTrack* track = *it;
	track->setTitle( thread->m_decoder->metaInfo( K3bAudioDecoder::META_TITLE ) );
	track->setArtist( thread->m_decoder->metaInfo( K3bAudioDecoder::META_ARTIST ) );
	track->setSongwriter( thread->m_decoder->metaInfo( K3bAudioDecoder::META_SONGWRITER ) );
	track->setComposer( thread->m_decoder->metaInfo( K3bAudioDecoder::META_COMPOSER ) );
	track->setCdTextMessage( thread->m_decoder->metaInfo( K3bAudioDecoder::META_COMMENT ) );
      }
      m_decoderMetaInfoSetMap.erase( thread->m_decoder );
      m_audioTrackStatusThreads.removeRef( thread );
    }
  }

  if( !m_audioTrackStatusThreads.isEmpty() )
    QTimer::singleShot( 500, this, SLOT(slotHouseKeeping()) );
  else
    emit changed(); // let the gui update the length values

  kdDebug() << "(K3bAudioDoc::slotHouseKeeping) finished" << endl;
}


K3bCdDevice::CdText K3bAudioDoc::cdTextData() const
{
  K3bCdDevice::CdText text( m_cdTextData );
  text.reserve( numOfTracks() );
  K3bAudioTrack* track = firstTrack();
  while( track ) {
    text.append( track->cdText() );

    track = track->next();
  }
  return text;
}


K3bCdDevice::Toc K3bAudioDoc::toToc() const
{
  K3bCdDevice::Toc toc;

  // FIXME: add MCN

  K3bAudioTrack* track = firstTrack();
  K3b::Msf pos = 0;
  while( track ) {
    toc.append( track->toCdTrack() );
    track = track->next();
  }

  return toc; 
}

#include "k3baudiodoc.moc"
