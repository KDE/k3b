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
#include "k3baudiotrack.h"
#include "k3baudiojob.h"
#include "k3baudiofile.h"
#include "k3baudiozerodata.h"
#include <k3bcuefileparser.h>

#include <k3bthread.h>
#include <k3bthreadjob.h>
#include <k3bcore.h>
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
  m_docType = AUDIO;
  m_audioTrackStatusThreads.setAutoDelete(true);
}

K3bAudioDoc::~K3bAudioDoc()
{
  // delete all tracks
  int i = 1;
  int cnt = numOfTracks();
  while( m_firstTrack ) {
    kdDebug() << "(K3bAudioDoc::~K3bAudioDoc) deleting track " << i << " of " << cnt << endl;
    delete m_firstTrack->take();
    kdDebug() << "(K3bAudioDoc::~K3bAudioDoc) deleted." << endl;
    ++i;
  }
}

bool K3bAudioDoc::newDocument()
{
  // delete all tracks
  while( m_firstTrack )
    delete m_firstTrack->take();

  m_normalize = false;
  m_hideFirstTrack = false;
  m_cdText = false;
  m_cdTextData.clear();
  m_audioRippingParanoiaMode = 0;
  m_audioRippingRetries = 128;
  m_audioRippingIgnoreReadErrors = false;

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
  KURL::List::iterator end( allUrls.end());
  for( KURL::List::iterator it = allUrls.begin(); it != end; it++, position++ ) {
    KURL& url = *it;
    if( url.path().right(3).lower() == "cue" ) {
      // try adding a cue file
      if( K3bAudioTrack* newAfter = importCueFile( url.path(), getTrack(position) ) ) {
	position = newAfter->trackNumber();
	continue;
      }
    }
    
    if( K3bAudioTrack* track = createTrack( url ) ) {
      addTrack( track, position );

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
  KURL::List::const_iterator end(allUrls.end());
  for( KURL::List::const_iterator it = allUrls.begin(); it != end; ++it ) {
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
  if( !after )
    after = m_lastTrack;

  kdDebug() << "(K3bAudioDoc::importCueFile( " << cuefile << ", " << after << ")" << endl;
  K3bCueFileParser parser( cuefile );
  if( parser.isValid() && parser.toc().contentType() == K3bDevice::AUDIO ) {

    kdDebug() << "(K3bAudioDoc::importCueFile) parsed with image: " << parser.imageFilename() << endl;

    // global cd-text
    if( !parser.cdText().title().isEmpty() )
      setTitle( parser.cdText().title() );
    if( !parser.cdText().performer().isEmpty() )
      setPerformer( parser.cdText().performer() );

    K3bAudioDecoder* decoder = getDecoderForUrl( KURL::fromPathOrURL(parser.imageFilename()) );
    if( decoder ) {
      K3bAudioFile* newFile = 0;
      unsigned int i = 0;
      for( K3bDevice::Toc::const_iterator it = parser.toc().begin();
	   it != parser.toc().end(); ++it ) {
	const K3bDevice::Track& track = *it;

	newFile = new K3bAudioFile( decoder, this );
	newFile->setStartOffset( track.firstSector() );
	newFile->setEndOffset( track.lastSector()+1 );

	K3bAudioTrack* newTrack = new K3bAudioTrack( this );
	newTrack->addSource( newFile );
	newTrack->moveAfter( after );

	// we do not know the length of the source yet so we have to force the index value
	if( track.index0() > 0 )
	  newTrack->m_index0Offset = track.length() - track.index0();
	else
	  newTrack->m_index0Offset = 0;

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
  if( m_decoderPresenceMap.contains( url.path() ) ) {
    decoder = m_decoderPresenceMap[url.path()];
  }
  else if( (decoder = K3bAudioDecoderFactory::createDecoder( url )) ) {
    kdDebug() << "(K3bAudioDoc) using " << decoder->className()
	      << " for decoding of " << url.path() << endl;
    
    decoder->setFilename( url.path() );
    
    //
    // start a thread to analyse the file
    //
    AudioFileAnalyzerThread* thread = new AudioFileAnalyzerThread( decoder );
    thread->start();
    m_audioTrackStatusThreads.append( thread );
    QTimer::singleShot( 500, this, SLOT(slotHouseKeeping()) );
  }
  
  return decoder;
}


K3bAudioFile* K3bAudioDoc::createAudioFile( const KURL& url )
{
  if( !QFile::exists( url.path() ) ) {
    m_notFoundFiles.append( url.path() );
    return 0;
  }
  
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



K3bAudioTrack* K3bAudioDoc::getTrack( unsigned int trackNum )
{
  K3bAudioTrack* track = m_firstTrack;
  unsigned int i = 1;
  while( track ) {
    if( i == trackNum )
      return track;
    track = track->next();
    ++i;
  }

  return 0;
}


void K3bAudioDoc::addTrack( K3bAudioTrack* track, uint position )
{
  kdDebug() << "(K3bAudioDoc::addTrack( " << track << ", " << position << " )" << endl;
  track->m_parent = this;
  if( !m_firstTrack )
    m_firstTrack = m_lastTrack = track;
  else if( position == 0 )
    track->moveAhead( m_firstTrack );
  else {
    K3bAudioTrack* after = getTrack( position );
    if( after )
      track->moveAfter( after );
    else
      track->moveAfter( m_lastTrack );  // just to be sure it's anywhere...
  }

  emit changed();
}


void K3bAudioDoc::removeTrack( K3bAudioTrack* track )
{
  delete track;
}


void K3bAudioDoc::moveTrack( K3bAudioTrack* track, K3bAudioTrack* after )
{
  track->moveAfter( after );
}


QString K3bAudioDoc::typeString() const
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
    
    else if( e.nodeName() == "audio_ripping" ) {
      QDomNodeList ripNodes = e.childNodes();
      for( uint j = 0; j < ripNodes.length(); j++ ) {
	if( ripNodes.item(j).nodeName() == "paranoia_mode" )
	  setAudioRippingParanoiaMode( ripNodes.item(j).toElement().text().toInt() );
	else if( ripNodes.item(j).nodeName() == "read_retries" )
	  setAudioRippingRetries( ripNodes.item(j).toElement().text().toInt() );
	else if( ripNodes.item(j).nodeName() == "ignore_read_errors" )
	  setAudioRippingIgnoreReadErrors( ripNodes.item(j).toElement().text() == "yes" );
      }
    }

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
	K3bAudioTrack* track = new K3bAudioTrack();


	// backwards compatibility
	// -----------------------------------------------------------------------------------------------------
	QDomAttr oldUrlAttr = trackElem.attributeNode( "url" );
	if( !oldUrlAttr.isNull() ) {
	  if( K3bAudioFile* file = 
	      createAudioFile( KURL::fromPathOrURL( oldUrlAttr.value() ) ) ) {
	    track->addSource( file );
	  }
	}
	// -----------------------------------------------------------------------------------------------------


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
		K3bAudioZeroData* zero = new K3bAudioZeroData();
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
	else {
	  kdDebug() << "(K3bAudioDoc) no sources. deleting track " << track << endl;
	  delete track;
	}
      }
    }
  }

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

  // save the audio cd ripping settings
  // paranoia mode, read retries, and ignore read errors
  // ------------------------------------------------------------
  QDomElement ripMain = doc.createElement( "audio_ripping" );
  docElem->appendChild( ripMain );

  QDomElement ripElem = doc.createElement( "paranoia_mode" );
  ripElem.appendChild( doc.createTextNode( QString::number( audioRippingParanoiaMode() ) ) );
  ripMain.appendChild( ripElem );

  ripElem = doc.createElement( "read_retries" );
  ripElem.appendChild( doc.createTextNode( QString::number( audioRippingRetries() ) ) );
  ripMain.appendChild( ripElem );

  ripElem = doc.createElement( "ignore_read_errors" );
  ripElem.appendChild( doc.createTextNode( audioRippingIgnoreReadErrors() ? "yes" : "no" ) );
  ripMain.appendChild( ripElem );
  // ------------------------------------------------------------

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
      // TODO: save a source element with a type attribute and start- and endoffset
      //       then distict between the different source types.
      if( K3bAudioFile* file = dynamic_cast<K3bAudioFile*>(source) ) {
	QDomElement sourceElem = doc.createElement( "file" );
	sourceElem.setAttribute( "url", file->filename() );
	sourceElem.setAttribute( "start_offset", file->startOffset().toString() );
	sourceElem.setAttribute( "end_offset", file->endOffset().toString() );
	sourcesParent.appendChild( sourceElem );
      }
      else if( K3bAudioZeroData* zero = dynamic_cast<K3bAudioZeroData*>(source) ) {
	QDomElement sourceElem = doc.createElement( "silence" );
	sourceElem.setAttribute( "length", zero->length().toString() );
	sourcesParent.appendChild( sourceElem );
      }
      else {
	kdError() << "(K3bAudioDoc) saving sources other than file or zero not supported yet." << endl;
	return false;
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
  return ( m_lastTrack ? m_lastTrack->trackNumber() : 0 );
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


void K3bAudioDoc::slotTrackChanged( K3bAudioTrack* track )
{
  kdDebug() << "(K3bAudioDoc::slotTrackChanged " << track << endl;
  setModified( true );
  emit changed();
  // if the track is empty now we simply delete it
  if( track->firstSource() )
    emit trackChanged(track);
  else {
    kdDebug() << "(K3bAudioDoc::slotTrackChanged) track " << track << " empty. Deleting." << endl;
    delete track; // this will emit the proper signal
  }
  kdDebug() << "(K3bAudioDoc::slotTrackChanged done" << track << endl;
}


void K3bAudioDoc::slotTrackRemoved( K3bAudioTrack* track )
{
  setModified( true );
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
	track->setPerformer( thread->m_decoder->metaInfo( K3bAudioDecoder::META_ARTIST ) );
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
    emit changed(); // inform about the changed length

  kdDebug() << "(K3bAudioDoc::slotHouseKeeping) finished" << endl;
}


K3bDevice::CdText K3bAudioDoc::cdTextData() const
{
  K3bDevice::CdText text( m_cdTextData );
  text.reserve( numOfTracks() );
  K3bAudioTrack* track = firstTrack();
  while( track ) {
    text.append( track->cdText() );

    track = track->next();
  }
  return text;
}


K3bDevice::Toc K3bAudioDoc::toToc() const
{
  K3bDevice::Toc toc;

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
