/***************************************************************************
                          k3baudiodoc.cpp  -  description
                             -------------------
    begin                : Tue Mar 27 2001
    copyright            : (C) 2001 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "../k3b.h"
#include "../k3bglobals.h"
#include "k3baudiodoc.h"
#include "k3baudioview.h"
#include "k3baudiotrack.h"
#include "k3baudioburndialog.h"
#include "k3baudiojob.h"
#include "input/k3baudiomodulefactory.h"
#include "input/k3baudiomodule.h"
#include "../rip/songdb/k3bsong.h"
#include "../rip/songdb/k3bsongmanager.h"


// QT-includes
#include <qstring.h>
#include <qstringlist.h>
#include <qfile.h>
#include <qdatastream.h>
#include <qdom.h>
#include <qdatetime.h>
#include <qtimer.h>

// KDE-includes
#include <kprocess.h>
#include <kurl.h>
#include <kapp.h>
#include <kmessagebox.h>
#include <kconfig.h>
#include <klocale.h>
#include <kstddirs.h>

#include <iostream>


K3bAudioDoc::K3bAudioDoc( QObject* parent )
  : K3bDoc( parent )
{
  m_tracks = 0L;
  m_cdText = true;
	
  m_docType = AUDIO;

  m_urlAddingTimer = new QTimer( this );
  connect( m_urlAddingTimer, SIGNAL(timeout()), this, SLOT(slotWorkUrlQueue()) );
}

K3bAudioDoc::~K3bAudioDoc()
{
  if( m_tracks )
    m_tracks->setAutoDelete( true );
		
  delete m_tracks;
}

bool K3bAudioDoc::newDocument()
{
  if( m_tracks )
    m_tracks->setAutoDelete( true );

  delete m_tracks;
	
  m_tracks = new QList<K3bAudioTrack>;
  m_tracks->setAutoDelete( false );
	
  m_cdText = true;
  m_padding = false;
  m_hideFirstTrack = false;
  m_removeBufferFiles = true;
	
  return K3bDoc::newDocument();
}



unsigned long K3bAudioDoc::size() const 
{
  unsigned long size = 0;
  for( QListIterator<K3bAudioTrack> it(*m_tracks); it.current(); ++it ) {
    size += it.current()->size();
  }	

  return size;
}


unsigned long K3bAudioDoc::length() const
{
  unsigned long length = 0;
  for( QListIterator<K3bAudioTrack> it(*m_tracks); it.current(); ++it ) {
    length += it.current()->length() + it.current()->pregap();
  }	

  return length;
}


void K3bAudioDoc::addUrl( const QString& url )
{
  addTrack( url, m_tracks->count() );
}


void K3bAudioDoc::addUrls( const QStringList& urls )
{
  addTracks( urls, m_tracks->count() );
}


void K3bAudioDoc::addTracks(const QStringList& urls, uint position )
{
  for( QStringList::ConstIterator it = urls.begin(); it != urls.end(); it++ ) {
    urlsToAdd.enqueue( new PrivateUrlToAdd( *it, position++ ) );
    //cerr <<  "adding url to queue: " << *it;
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
	
    addedFile = KURL( item->url );
    delete item;
		
    if( !addedFile.isValid() ) {
      qDebug( addedFile.path() + " not valid" );
      return;
    }
	
    if( !QFile::exists( addedFile.path() ) ) {
      KMessageBox::error( kapp->mainWidget(), i18n("File not found: ") + addedFile.fileName() );
      return;
    }

    // TODO: check if it is a textfile and if so try to create a KURL from every line
    //       for now drop all non-local urls
    //       add all existing files
    if( K3bAudioTrack* newTrack = createTrack( addedFile.path() ) )
      addTrack( newTrack, lastAddedPosition );
  }

  else {
    m_urlAddingTimer->stop();

    emit newTracks();
  }
}


K3bAudioTrack* K3bAudioDoc::createTrack( const QString& url )
{
  unsigned long length = identifyWaveFile( url );
  if( length > 0 || K3bAudioModuleFactory::moduleAvailable( url ) ) {
    K3bAudioTrack* newTrack =  new K3bAudioTrack( m_tracks, url );
    if( length > 0 ) {
      newTrack->setLength( length );  // no module needed for wave files
      newTrack->setStatus( K3bAudioTrack::OK );
    }
    else {
      K3bAudioModule* module = K3bAudioModuleFactory::createModule( newTrack );
      newTrack->setModule( module );

      // connect to the finished signal to ensure the calculated length and status of the file 
      // will be displayed properly
      // FIXME: it does not seem to work. The filldisplay is not updated at all
      //connect( module, SIGNAL(finished(bool)), this, SLOT(updateAllViews()) );
    }

    K3bSong *song = k3bMain()->songManager()->findSong( url );
    if( song != 0 ){
      newTrack->setArtist( song->getArtist() );
      newTrack->setAlbum( song->getAlbum() );
      newTrack->setTitle( song->getTitle() );
    }

    return newTrack;
  }
  else {
    KMessageBox::error( kapp->mainWidget(), "(" + url + ")\n" + i18n("Only mp3 and wav audio files are supported."), 
			i18n("Wrong file format") );		
    return 0;
  }
}


void K3bAudioDoc::addTrack(const QString& url, uint position )
{
  urlsToAdd.enqueue( new PrivateUrlToAdd( url, position ) );

  m_urlAddingTimer->start(0);
}



void K3bAudioDoc::addTrack( K3bAudioTrack* track, uint position )
{
  if( m_tracks->count() >= 99 ) {
    qDebug( "(K3bAudioDoc) Red Book only allows 99 tracks." );
    // TODO: show some messagebox
    delete track;
    return;
  }

  lastAddedPosition = position;
	
  if( !m_tracks->insert( position, track ) ) {
    lastAddedPosition = m_tracks->count();
    m_tracks->insert( m_tracks->count(), track );
  }
  
  setModified( true );
}


void K3bAudioDoc::removeTrack( int position )
{
  if( position < 0 ) {
    qDebug( "(K3bAudioDoc) tried to remove track with index < 0!" );
    return;
  }
	
  K3bAudioTrack* track = take( position );
  if( track ) {
    // emit signal before deleting the track to avoid crashes
    // when the view tries to call some of the tracks' methods
    emit newTracks();

    delete track;
  }
}

void K3bAudioDoc::moveTrack( int oldPos, int newPos )
{
  // newPos is the nwe position in the current list,
  // so if oldPos < newPos we have to add the track at newPos-1
  // after it was removed

  K3bAudioTrack* track = m_tracks->take( oldPos );
  if(track) {
    m_tracks->insert( (oldPos < newPos) ? newPos-1 : newPos, track );
  }
}

K3bView* K3bAudioDoc::newView( QWidget* parent )
{
  return new K3bAudioView( this, parent );
}


QString K3bAudioDoc::documentType() const
{
  return "k3b_audio_project";
}


bool K3bAudioDoc::loadDocumentData( QDomDocument* doc )
{
  newDocument();

  // we will parse the dom-tree and create a K3bAudioTrack for all entries immediately
  // this should not take long and so not block the gui

  if( doc->doctype().name() != documentType() )
    return false;

  QDomNodeList nodes = doc->documentElement().childNodes();

  if( nodes.length() < 4 )
    return false;

  if( nodes.item(0).nodeName() != "general" )
    return false;
  if( !readGeneralDocumentData( nodes.item(0).toElement() ) )
    return false;

  // parse padding
  if( nodes.item(1).nodeName() != "padding" ) 
    return false;
  else {
    QDomElement e = nodes.item(1).toElement();
    if( e.isNull() )
      return false;
    else
      setPadding( e.text() == "yes" );
  }

  // parse cd-text
  if( nodes.item(2).nodeName() != "cd-text" )
    return false;
  else {
    QDomElement e = nodes.item(2).toElement();
    if( e.isNull() )
      return false;
    if( !e.hasAttribute( "activated" ) )
      return false;
	
    writeCdText( e.attributeNode( "activated" ).value() == "yes" );
  }

  QDomNodeList cdTextNodes = nodes.item(2).childNodes();
  setTitle( cdTextNodes.item(0).toElement().text() );
  setArtist( cdTextNodes.item(1).toElement().text() );
  setArranger( cdTextNodes.item(2).toElement().text() );
  setSongwriter( cdTextNodes.item(3).toElement().text() );
  setDisc_id( cdTextNodes.item(4).toElement().text() );
  setUpc_ean( cdTextNodes.item(5).toElement().text() );
  setCdTextMessage( cdTextNodes.item(6).toElement().text() );

  if( nodes.item(3).nodeName() != "contents" )
    return false;

  QDomNodeList trackNodes = nodes.item(3).childNodes();

  for( uint i = 0; i< trackNodes.length(); i++ ) {

    // check if url is available
    QDomElement trackElem = trackNodes.item(i).toElement();
    QString url = trackElem.attributeNode( "url" ).value();
    if( !QFile::exists( url ) )
      qDebug( "(K3bAudioDoc) Could not find file: " + url );
    else {

      if( K3bAudioTrack* track = createTrack( url ) ) {
      
	QDomNodeList trackNodes = trackElem.childNodes();

	// set cd-text
	QDomElement cdTextElem = trackNodes.item(0).toElement();

	cdTextNodes = cdTextElem.childNodes();
	track->setTitle( cdTextNodes.item(0).toElement().text() );
	track->setArtist( cdTextNodes.item(1).toElement().text() );
	track->setArranger( cdTextNodes.item(2).toElement().text() );
	track->setSongwriter( cdTextNodes.item(3).toElement().text() );
	track->setIsrc( cdTextNodes.item(4).toElement().text() );
	track->setAlbum( cdTextNodes.item(5).toElement().text() );
	track->setCdTextMessage( cdTextNodes.item(6).toElement().text() );
      

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


  emit newTracks();

  return true;
}

bool K3bAudioDoc::saveDocumentData( QDomDocument* doc )
{
  doc->appendChild( doc->createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"ISO 8859-1\"" ) );

  QDomElement docElem = doc->createElement( documentType() );

  saveGeneralDocumentData( &docElem );

  // add padding
  QDomElement paddingElem = doc->createElement( "padding" );
  paddingElem.appendChild( doc->createTextNode( padding() ? "yes" : "no" ) );
  docElem.appendChild( paddingElem );


  // save disc cd-text
  // -------------------------------------------------------------
  QDomElement cdTextMain = doc->createElement( "cd-text" );
  cdTextMain.setAttribute( "activated", cdText() ? "yes" : "no" );
  QDomElement cdTextElem = doc->createElement( "title" );
  cdTextElem.appendChild( doc->createTextNode( (title())) );
  cdTextMain.appendChild( cdTextElem );

  cdTextElem = doc->createElement( "artist" );
  cdTextElem.appendChild( doc->createTextNode( (artist())) );
  cdTextMain.appendChild( cdTextElem );

  cdTextElem = doc->createElement( "arranger" );
  cdTextElem.appendChild( doc->createTextNode( (arranger())) );
  cdTextMain.appendChild( cdTextElem );

  cdTextElem = doc->createElement( "songwriter" );
  cdTextElem.appendChild( doc->createTextNode( (songwriter())) );
  cdTextMain.appendChild( cdTextElem );

  cdTextElem = doc->createElement( "disc_id" );
  cdTextElem.appendChild( doc->createTextNode( (disc_id())) );
  cdTextMain.appendChild( cdTextElem );

  cdTextElem = doc->createElement( "upc_ean" );
  cdTextElem.appendChild( doc->createTextNode( (upc_ean())) );
  cdTextMain.appendChild( cdTextElem );

  cdTextElem = doc->createElement( "message" );
  cdTextElem.appendChild( doc->createTextNode( (cdTextMessage())) );
  cdTextMain.appendChild( cdTextElem );

  docElem.appendChild( cdTextMain );
  // -------------------------------------------------------------

  // save the tracks
  // -------------------------------------------------------------
  QDomElement contentsElem = doc->createElement( "contents" );

  for( K3bAudioTrack* track = first(); track != 0; track = next() ) {

    QDomElement trackElem = doc->createElement( "track" );
    trackElem.setAttribute( "url", track->absPath() );

    // add cd-text
    cdTextMain = doc->createElement( "cd-text" );
    cdTextElem = doc->createElement( "title" );
    cdTextElem.appendChild( doc->createTextNode( (track->title())) );
    cdTextMain.appendChild( cdTextElem );
    
    cdTextElem = doc->createElement( "artist" );
    cdTextElem.appendChild( doc->createTextNode( (track->artist())) );
    cdTextMain.appendChild( cdTextElem );
    
    cdTextElem = doc->createElement( "arranger" );
    cdTextElem.appendChild( doc->createTextNode( (track->arranger()) ) );
    cdTextMain.appendChild( cdTextElem );
    
    cdTextElem = doc->createElement( "songwriter" );
    cdTextElem.appendChild( doc->createTextNode( (track->songwriter()) ) );
    cdTextMain.appendChild( cdTextElem );
    
    cdTextElem = doc->createElement( "isrc" );
    cdTextElem.appendChild( doc->createTextNode( ( track->isrc()) ) );
    cdTextMain.appendChild( cdTextElem );
    
    cdTextElem = doc->createElement( "album" );
    cdTextElem.appendChild( doc->createTextNode( (track->album()) ) );
    cdTextMain.appendChild( cdTextElem );
    
    cdTextElem = doc->createElement( "message" );
    cdTextElem.appendChild( doc->createTextNode( (track->cdTextMessage()) ) );
    cdTextMain.appendChild( cdTextElem );

    trackElem.appendChild( cdTextMain );


    // add pregap
    QDomElement pregapElem = doc->createElement( "pregap" );    
    pregapElem.appendChild( doc->createTextNode( QString::number(track->pregap()) ) );
    trackElem.appendChild( pregapElem );

    // add copy protection
    QDomElement copyElem = doc->createElement( "copy_protection" );    
    copyElem.appendChild( doc->createTextNode( track->copyProtection() ? "yes" : "no" ) );
    trackElem.appendChild( copyElem );

    // add pre emphasis
    copyElem = doc->createElement( "pre_emphasis" );    
    copyElem.appendChild( doc->createTextNode( track->preEmp() ? "yes" : "no" ) );
    trackElem.appendChild( copyElem );


    contentsElem.appendChild( trackElem );
  }
  // -------------------------------------------------------------

  docElem.appendChild( contentsElem );

  doc->appendChild( docElem );

  return true;
}


void K3bAudioDoc::addView(K3bView* view)
{
  K3bDoc::addView( view );
}


bool K3bAudioDoc::writeTOC( const QString& filename )
{
  QFile file( filename );
  if( !file.open( IO_WriteOnly ) ) {
    qDebug( "(K3bAudioDoc) Could not open toc-file %s", filename.latin1() );
    return false;
  }

  bool success = true;

  long stdinDataLength = 0;
	
  QTextStream t(&file);


  // ===========================================================================
  // header
  // ===========================================================================

  // little comment
  t << "// TOC-file to use with cdrdao created by K3b" << "\n\n";

  // we create a CDDA tocfile
  t << "CD_DA\n\n";

  // create the album CD-TEXT entries if needed
  // ---------------------------------------------------------------------------
  if( cdText() ) {
    t << "CD_TEXT {" << "\n";
    t << "  LANGUAGE_MAP { 0: EN }\n";
    t << "  LANGUAGE 0 {\n";
    t << "    TITLE " << "\"" << prepareForTocFile(title()) << "\"" << "\n";
    t << "    PERFORMER " << "\"" << prepareForTocFile(artist()) << "\"" << "\n";
    if( !disc_id().isEmpty() )
      t << "    DISC_ID " << "\"" << prepareForTocFile(disc_id()) << "\"" << "\n";
    if( !upc_ean().isEmpty() )
      t << "    UPC_EAN " << "\"" << prepareForTocFile(upc_ean()) << "\"" << "\n";
    t << "\n";
    if( !arranger().isEmpty() )
      t << "    ARRANGER " << "\"" << prepareForTocFile(arranger()) << "\"" << "\n";
    if( !songwriter().isEmpty() )
      t << "    SONGWRITER " << "\"" << prepareForTocFile(songwriter()) << "\"" << "\n";
    if( !cdTextMessage().isEmpty() )
      t << "    MESSAGE " << "\"" << prepareForTocFile(cdTextMessage()) << "\"" << "\n";
    t << "  }" << "\n";
    t << "}" << "\n\n";
  }
  // ---------------------------------------------------------------------------




  // ===========================================================================
  // the tracks
  // ===========================================================================

  K3bAudioTrack* _track = first();

  // if we need to hide the first song in the first tracks' pregap
  // we process the first two songs at once

  if( hideFirstTrack() ) {
    K3bAudioTrack* hiddenTrack = _track;
    _track = next();
    if( _track == 0 ) {
      // we cannot hide a lonely track
      _track = first();
    }
    else {
      t << "TRACK AUDIO" << "\n";


      // _track is the "real" first track so it's copy and preemp information is used
      if( _track->copyProtection() )
	t << "COPY" << "\n";
      
      if( _track->preEmp() )
	t << "PRE_EMPHASIS" << "\n";

      // CD-TEXT if needed
      // ------------------------------------------------------------------------
      if( cdText() ) {
	t << "CD_TEXT {" << "\n";
	t << "  LANGUAGE 0 {" << "\n";
	t << "    TITLE " << "\"" << prepareForTocFile(_track->title()) << "\"" << "\n";
	t << "    PERFORMER " << "\"" << prepareForTocFile(_track->artist()) << "\"" << "\n";
	if( !_track->isrc().isEmpty() )
	  t << "    ISRC " << "\"" << prepareForTocFile(_track->isrc()) << "\"" << "\n";
	if( !_track->arranger().isEmpty() )
	  t << "    ARRANGER " << "\"" << prepareForTocFile(_track->arranger()) << "\"" << "\n";
	if( !_track->songwriter().isEmpty() )
	  t << "    SONGWRITER " << "\"" << prepareForTocFile(_track->songwriter()) << "\"" << "\n";
	if( !_track->cdTextMessage().isEmpty() )
	  t << "    MESSAGE " << "\"" << prepareForTocFile(_track->cdTextMessage()) << "\"" << "\n";
	t << "  }" << "\n";
	t << "}" << "\n";
      }
      // ------------------------------------------------------------------------


      // the "hidden" file will be used as pregap for the "first" track
      success = success && addTrackToToc( hiddenTrack, t, stdinDataLength );
      t << "START" << "\n"; // use the whole hidden file as pregap


      // now comes the "real" first track
      success = success && addTrackToToc( _track, t, stdinDataLength );
      t << "\n";
    }

    _track = next();
  }


  // now iterate over the rest of the tracks
  
  for( ; _track != 0; _track = next() ) {
    t << "TRACK AUDIO" << "\n";

    if( _track->copyProtection() )
      t << "COPY" << "\n";

    if( _track->preEmp() )
      t << "PRE_EMPHASIS" << "\n";

    // CD-TEXT if needed
    // ------------------------------------------------------------------------
    if( cdText() ) {
      t << "CD_TEXT {" << "\n";
      t << "  LANGUAGE 0 {" << "\n";
      t << "    TITLE " << "\"" << prepareForTocFile(_track->title()) << "\"" << "\n";
      t << "    PERFORMER " << "\"" << prepareForTocFile(_track->artist()) << "\"" << "\n";
      if( !_track->isrc().isEmpty() )
	t << "    ISRC " << "\"" << prepareForTocFile(_track->isrc()) << "\"" << "\n";
      if( !_track->arranger().isEmpty() )
	t << "    ARRANGER " << "\"" << prepareForTocFile(_track->arranger()) << "\"" << "\n";
      if( !_track->songwriter().isEmpty() )
	t << "    SONGWRITER " << "\"" << prepareForTocFile(_track->songwriter()) << "\"" << "\n";
      if( !_track->cdTextMessage().isEmpty() )
	t << "    MESSAGE " << "\"" << prepareForTocFile(_track->cdTextMessage()) << "\"" << "\n";
      t << "  }" << "\n";
      t << "}" << "\n";
    }
    // ------------------------------------------------------------------------

    if( _track->pregap() > 0 ) {
      t << "PREGAP " << K3b::framesToString( _track->pregap() ) << "\n";
    }

    success = success && addTrackToToc( _track, t, stdinDataLength );
    t << "\n";
  }
  // --------------------------------- TOC --	
	
  file.close();

  return success;
}


bool K3bAudioDoc::addTrackToToc( K3bAudioTrack* track, QTextStream& t, long& stdinDataLength )
{
  bool success = true;

  t << "FILE ";
  if( track->isWave() ) {
    t << "\"" << track->absPath() << "\"" << " 0" << "\n";
  }
  else if( onTheFly() ) {
    t << "\"-\" ";   // read from stdin
    t << K3b::framesToString( stdinDataLength );        // where does the track start in stdin
    t << " " << K3b::framesToString( track->length() );   // here we need the perfect length !!!!!
    t << "\n";
    
    stdinDataLength += track->length();
  }
  else {
    if( track->bufferFile().isEmpty() ) {
      qDebug( "(K3bAudioDoc) not all files buffered. toc-file cannot be used for writing." );
      success = false;
    }
    t << "\"" << track->bufferFile() << "\"" << " 0" << "\n";
  }

  return success;
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



QString K3bAudioDoc::prepareForTocFile( const QString& str )
{
  // since "\" is the only special character I now of so far...
  QString newStr = str;
  int pos = str.find('\\');
  while( pos > -1 ) {
    newStr.insert( pos+1, "134" );
    pos = str.find( '\\', pos+3 );
  }

  return newStr;
}


/**
 * Returns the length of the wave file in frames (1/75 second) if
 * it is a 16bit stereo 44100 kHz wave file
 * Otherwise 0 is returned.
 */
unsigned long K3bAudioDoc::identifyWaveFile( const QString& filename )
{
  QFile inputFile( filename );
  if( !inputFile.open(IO_ReadOnly) ) {
    qDebug("Could not open file: " + filename );
    return 0;
  }

  QDataStream inputStream( &inputFile );

  char magic[4];

  inputStream.readRawBytes( magic, 4 );
  if( inputStream.atEnd() || qstrncmp(magic, "RIFF", 4) ) {
    qDebug( filename + ": not a RIFF file.");
    return 0;
  }

  inputFile.at( 8 );
  inputStream.readRawBytes( magic, 4 );
  if( inputStream.atEnd() || qstrncmp(magic, "WAVE", 4) ) {
    qDebug( filename + ": not a wave file.");
    return 0;
  }

  Q_INT32 chunkLen;

  while( qstrncmp(magic, "fmt ", 4) ) {

    inputStream.readRawBytes( magic, 4 );
    if( inputStream.atEnd() ) {
      qDebug( filename + ": could not find format chunk.");
      return 0;
    }

    inputStream >> chunkLen;
    chunkLen = K3b::swapByteOrder( chunkLen );
    chunkLen += chunkLen & 1; // round to multiple of 2

    // skip chunk data of unknown chunk
    if( qstrncmp(magic, "fmt ", 4) )
      if( !inputFile.at( inputFile.at() + chunkLen ) ) {
	qDebug( filename + ": could not seek in file.");
	return 0;
      }
  }

  // found format chunk
  if( chunkLen < 16 )
    return 0;

  Q_INT16 waveFormat;
  inputStream >> waveFormat;
  if (inputStream.atEnd() || K3b::swapByteOrder(waveFormat) != 1) {
    qDebug( filename + ": not in PCM format: %i", waveFormat);
    return 0;
  }

  Q_INT16 waveChannels;
  inputStream >> waveChannels;
  if (inputStream.atEnd() || K3b::swapByteOrder(waveChannels) != 2) {
    qDebug( filename + ": found %d channel(s), require 2 channels.", waveChannels );
    return 0;
  }

  Q_INT32 waveRate;
  inputStream >> waveRate; 
  if (inputStream.atEnd() || K3b::swapByteOrder(waveRate) != 44100) {
     qDebug( filename + ": found sampling rate %ld, require 44100.", waveRate);
     return 0;
  }

  Q_INT16 buffer16;
  Q_INT32 buffer32;
  inputStream >> buffer32; // skip average bytes/second
  inputStream >> buffer16; // skip block align

  Q_INT16 waveBits;
  inputStream >> waveBits;
  if (inputStream.atEnd() || K3b::swapByteOrder(waveBits) != 16) {
    qDebug( filename + ": found %d bits per sample, require 16.", waveBits);
    return 0;
  }

  chunkLen -= 16;
  // skip all other (unknown) format chunk fields
  if( !inputFile.at( inputFile.at() + chunkLen ) ) {
    qDebug( filename + ": could not seek in file.");
    return 0;
  }


  // search data chunk
  while( qstrncmp(magic,"data", 4) ) {

    inputStream.readRawBytes( magic, 4 );
    if( inputStream.atEnd()  ) {
      qDebug( filename + ": could not find data chunk.");
      return 0;
    }

    inputStream >> chunkLen;
    chunkLen = K3b::swapByteOrder( chunkLen );
    chunkLen += chunkLen & 1; // round to multiple of 2

    // skip chunk data of unknown chunk
    if( qstrncmp(magic, "data", 4) )
      if( !inputFile.at( inputFile.at() + chunkLen ) ) {
	qDebug( filename + ": could not seek in file.");
	return 0;
      }
  }

  // found data chunk
  int headerLen = inputFile.at();
  if( headerLen + chunkLen > inputFile.size() ) {
    qDebug( filename + ": file length does not match length from WAVE header - using actual length." );
    return (inputFile.size() - headerLen)/2352;
  }
  else {
    return chunkLen/2352;
  }
}


#include "k3baudiodoc.moc"
