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



unsigned long K3bAudioDoc::size() const {
  unsigned long size = 0;
  for( K3bAudioTrack* _t = m_tracks->first(); _t; _t = m_tracks->next() ) {
    size += _t->size();
  }	

  return size;
}


unsigned long K3bAudioDoc::length() const
{
  unsigned long size = 0;
  for( K3bAudioTrack* _t = m_tracks->first(); _t; _t = m_tracks->next() ) {
    size += _t->length() + _t->pregap();
  }	

  return size;
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
      KMessageBox::information( kapp->mainWidget(), "File not found: " + addedFile.fileName(), 
				"Error", QString::null, false );
      return;
    }

    // TODO: check if it is a textfile and if so try to create a KURL from every line
    //       for now drop all non-local urls
    //       add all existing files
    unsigned long length = isWaveFile( addedFile );
    if( length > 0 || K3bAudioModuleFactory::moduleAvailable( addedFile ) ) {
      K3bAudioTrack* newTrack =  new K3bAudioTrack( m_tracks, addedFile.path() );
      if( length > 0 ) {
	newTrack->setLength( length );  // no module needed for wave files
      }
      else {
	K3bAudioModule* module = K3bAudioModuleFactory::createModule( newTrack );
	newTrack->setModule( module );

	// connect to the finished signal to ensure the calculated length and status of the file 
	// will be displayed properly
	// FIXME: it does not seem to work. The filldisplay is not updated at all
	connect( module, SIGNAL(finished(bool)), this, SLOT(updateAllViews()) );
      }

      K3bSong *song = k3bMain()->songManager()->findSong( addedFile.path() );
      if( song != 0 ){
	newTrack->setArtist( song->getArtist() );
	newTrack->setAlbum( song->getAlbum() );
	newTrack->setTitle( song->getTitle() );
      }
      addTrack( newTrack, lastAddedPosition );
    }
    else {
      KMessageBox::information( kapp->mainWidget(), "Only mp3 and wav audio files are supported!", 
				"Wrong file format", QString::null, false );		
    }
  }

  else {
    m_urlAddingTimer->stop();

    emit newTracks();
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
    // emit signal before deleteing the track to avoid crashes
    // when the view tries to call some of the tracks' methods
    emit newTracks();

    delete track;
  }
}

void K3bAudioDoc::moveTrack( uint oldPos, uint newPos )
{
  K3bAudioTrack* _track = m_tracks->take( oldPos );
  if(_track)
    m_tracks->insert( newPos, _track );
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
    else if( !K3bAudioModuleFactory::moduleAvailable( KURL(url) ) )
      qDebug( "(K3bAudioDoc) No module available for file: " + url );
    else {

      K3bAudioTrack* track = new K3bAudioTrack( m_tracks, url );
      
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
  K3bAudioView* v = (K3bAudioView*)view;
  connect( v, SIGNAL(dropped(const QStringList&, uint)), this, SLOT(addTracks(const QStringList&, uint)) );
	
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
    t << "    TITLE " << "\"" << title() << "\"" << "\n";
    t << "    PERFORMER " << "\"" << artist() << "\"" << "\n";
    if( !disc_id().isEmpty() )
      t << "    DISC_ID " << "\"" << disc_id() << "\"" << "\n";
    if( !upc_ean().isEmpty() )
      t << "    UPC_EAN " << "\"" << upc_ean() << "\"" << "\n";
    t << "\n";
    if( !arranger().isEmpty() )
      t << "    ARRANGER " << "\"" << arranger() << "\"" << "\n";
    if( !songwriter().isEmpty() )
      t << "    SONGWRITER " << "\"" << songwriter() << "\"" << "\n";
    if( !cdTextMessage().isEmpty() )
      t << "    MESSAGE " << "\"" << cdTextMessage() << "\"" << "\n";
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
	t << "    TITLE " << "\"" << _track->title() << "\"" << "\n";
	t << "    PERFORMER " << "\"" << _track->artist() << "\"" << "\n";
	if( !_track->isrc().isEmpty() )
	  t << "    ISRC " << "\"" << _track->isrc() << "\"" << "\n";
	if( !_track->arranger().isEmpty() )
	  t << "    ARRANGER " << "\"" << _track->arranger() << "\"" << "\n";
	if( !_track->songwriter().isEmpty() )
	  t << "    SONGWRITER " << "\"" << _track->songwriter() << "\"" << "\n";
	if( !_track->cdTextMessage().isEmpty() )
	  t << "    MESSAGE " << "\"" << _track->cdTextMessage() << "\"" << "\n";
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
      t << "    TITLE " << "\"" << _track->title() << "\"" << "\n";
      t << "    PERFORMER " << "\"" << _track->artist() << "\"" << "\n";
      if( !_track->isrc().isEmpty() )
	t << "    ISRC " << "\"" << _track->isrc() << "\"" << "\n";
      if( !_track->arranger().isEmpty() )
	t << "    ARRANGER " << "\"" << _track->arranger() << "\"" << "\n";
      if( !_track->songwriter().isEmpty() )
	t << "    SONGWRITER " << "\"" << _track->songwriter() << "\"" << "\n";
      if( !_track->cdTextMessage().isEmpty() )
	t << "    MESSAGE " << "\"" << _track->cdTextMessage() << "\"" << "\n";
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
//   QListIterator<K3bAudioTrack> it(*m_tracks);
//   for( ; it.current(); ++it )
//     if( it.current()->filetype() != K3b::WAV )
//       return true;
	
  return m_padding;
}


K3bBurnJob* K3bAudioDoc::newBurnJob()
{
  return new K3bAudioJob( this );
}


unsigned long K3bAudioDoc::isWaveFile( const KURL& url )
{
  // we take url as a lokal file
  long headerLength;
  unsigned long dataLength;
  int x = K3b::waveLength( url.path().latin1(), 0, &headerLength, &dataLength );
  if( x )
    return 0;
  else
    return dataLength;
}


#include "k3baudiodoc.moc"
