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
#include "k3baudioontheflyjob.h"
#include "input/k3baudiomodulefactory.h"
#include "input/k3baudiomodule.h"


// QT-includes
#include <qstring.h>
#include <qstringlist.h>
#include <qfile.h>
#include <qdom.h>
#include <qdatetime.h>
#include <qtextstream.h>
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
	
  // setting defaults that should come from k3bmain
  testFiles = false;

  m_cdText = true;
  m_padding = false;
  m_hideFirstTrack = false;
	
  return K3bDoc::newDocument();
}



long K3bAudioDoc::size() const {
  long size = 0;
  for( K3bAudioTrack* _t = m_tracks->first(); _t; _t = m_tracks->next() ) {
    size += _t->size();
  }	

  return size;
}


int K3bAudioDoc::length() const
{
  int size = 0;
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
    PrivateUrlToAdd* _item = urlsToAdd.dequeue();
    lastAddedPosition = _item->position;
		
    // append at the end by default
    if( lastAddedPosition > m_tracks->count() )
      lastAddedPosition = m_tracks->count();
	
    addedFile = KURL( _item->url );
    delete _item;
		
    if( !addedFile.isValid() ) {
      qDebug( addedFile.path() + " not valid" );
      return;
    }
	
    if( !QFile::exists( addedFile.path() ) ) {
      KMessageBox::information( kapp->mainWidget(), "File not found: " + addedFile.fileName(), 
				"Error", QString::null, false );
      return;
    }
	
    if( K3bAudioModuleFactory::moduleAvailable( addedFile ) ) {
      K3bAudioTrack* newTrack =  new K3bAudioTrack( m_tracks, addedFile.path() );
      newTrack->module()->init();   // read special data like id3-tags
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



void K3bAudioDoc::addTrack( K3bAudioTrack* _track, uint position )
{
  lastAddedPosition = position;
	
  if( !m_tracks->insert( position, _track ) ) {
    lastAddedPosition = m_tracks->count();
    m_tracks->insert( m_tracks->count(), _track );
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
  cdTextElem.appendChild( doc->createTextNode(title()) );
  cdTextMain.appendChild( cdTextElem );

  cdTextElem = doc->createElement( "artist" );
  cdTextElem.appendChild( doc->createTextNode(artist()) );
  cdTextMain.appendChild( cdTextElem );

  cdTextElem = doc->createElement( "arranger" );
  cdTextElem.appendChild( doc->createTextNode(arranger()) );
  cdTextMain.appendChild( cdTextElem );

  cdTextElem = doc->createElement( "songwriter" );
  cdTextElem.appendChild( doc->createTextNode(songwriter()) );
  cdTextMain.appendChild( cdTextElem );

  cdTextElem = doc->createElement( "disc_id" );
  cdTextElem.appendChild( doc->createTextNode(disc_id()) );
  cdTextMain.appendChild( cdTextElem );

  cdTextElem = doc->createElement( "upc_ean" );
  cdTextElem.appendChild( doc->createTextNode(upc_ean()) );
  cdTextMain.appendChild( cdTextElem );

  cdTextElem = doc->createElement( "message" );
  cdTextElem.appendChild( doc->createTextNode(cdTextMessage()) );
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
    cdTextElem.appendChild( doc->createTextNode(track->title()) );
    cdTextMain.appendChild( cdTextElem );
    
    cdTextElem = doc->createElement( "artist" );
    cdTextElem.appendChild( doc->createTextNode(track->artist()) );
    cdTextMain.appendChild( cdTextElem );
    
    cdTextElem = doc->createElement( "arranger" );
    cdTextElem.appendChild( doc->createTextNode(track->arranger()) );
    cdTextMain.appendChild( cdTextElem );
    
    cdTextElem = doc->createElement( "songwriter" );
    cdTextElem.appendChild( doc->createTextNode(track->songwriter()) );
    cdTextMain.appendChild( cdTextElem );
    
    cdTextElem = doc->createElement( "isrc" );
    cdTextElem.appendChild( doc->createTextNode(track->isrc()) );
    cdTextMain.appendChild( cdTextElem );
    
    cdTextElem = doc->createElement( "album" );
    cdTextElem.appendChild( doc->createTextNode(track->album()) );
    cdTextMain.appendChild( cdTextElem );
    
    cdTextElem = doc->createElement( "message" );
    cdTextElem.appendChild( doc->createTextNode(track->cdTextMessage()) );
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

  int _trackStart = 0;
	
  QTextStream t(&file);
  // --- writing the TOC -------------------
  // header
  t << "// TOC-file to use with cdrdao created by K3b" << "\n\n";
  t << "CD_DA\n\n";
  if( cdText() ) {
    t << "CD_TEXT {" << "\n";
    t << "  LANGUAGE_MAP { 0: EN }\n";
    t << "  LANGUAGE 0 {\n";
    t << "    TITLE " << "\"" << title() << "\"" << "\n";
    t << "    PERFORMER " << "\"" << artist() << "\"" << "\n";
    t << "    DISC_ID " << "\"" <<	disc_id() << "\"" << "\n";
    t << "    UPC_EAN " << "\"" << "\"" << "\n";
    t << "\n";
    t << "    ARRANGER " << "\"" << arranger() << "\"" << "\n";
    t << "    SONGWRITER " << "\"" << songwriter() << "\"" << "\n";
    t << "    MESSAGE " << "\"" << cdTextMessage() << "\"" << "\n";
    t << "  }" << "\n";
    t << "}" << "\n\n";
  }
	
  // tracks
  K3bAudioTrack* _track = first();
  if( hideFirstTrack() ) {
    K3bAudioTrack* hiddenTrack = _track;
    _track = next();
    if( _track == 0 ) {
      // we cannot hide a lonely track
      _track = first();
    }
    else {
      t << "TRACK AUDIO" << "\n";
      if( cdText() ) {
	t << "CD_TEXT {" << "\n";
	t << "  LANGUAGE 0 {" << "\n";
	t << "    TITLE " << "\"" << _track->title() << "\"" << "\n";
	t << "    PERFORMER " << "\"" << _track->artist() << "\"" << "\n";
	t << "    ISRC " << "\"" << _track->isrc() << "\"" << "\n";
	t << "    ARRANGER " << "\"" << _track->arranger() << "\"" << "\n";
	t << "    SONGWRITER " << "\"" << _track->songwriter() << "\"" << "\n";
	t << "    MESSAGE " << "\"" << _track->cdTextMessage() << "\"" << "\n";
	t << "  }" << "\n";
	t << "}" << "\n";
      }

      t << "FILE ";
      if( onTheFly() ) {
	t << "\"-\" ";   // read from stdin
	t << K3b::framesToString( _trackStart );        // where does the track start in stdin
	t << " " << K3b::framesToString( hiddenTrack->length() );   // here we need the perfect length !!!!!
	t << "\n";
	
	_trackStart += hiddenTrack->length();
      }
      else {
	if( hiddenTrack->bufferFile().isEmpty() ) {
	  t << "\"" << hiddenTrack->absPath() << "\"" << " 0" << "\n";
	  qDebug( "(K3bAudioDoc) not all files buffered. toc-file cannot be used for writing." );
	  success = false;
	}
	else
	  t << "\"" << hiddenTrack->bufferFile() << "\"" << " 0" << "\n";
      }
      t << "START" << "\n"; // use the whole file as pregap

      t << "FILE ";
      if( onTheFly() ) {
	t << "\"-\" ";   // read from stdin
	t << K3b::framesToString( _trackStart );        // where does the track start in stdin
	t << " " << K3b::framesToString( _track->length() );   // here we need the perfect length !!!!!
	t << "\n";
	
	_trackStart += _track->length();
      }
      else {
	if( _track->bufferFile().isEmpty() ) {
	  t << "\"" << _track->absPath() << "\"" << " 0" << "\n";
	  qDebug( "(K3bAudioDoc) not all files buffered. toc-file cannot be used for writing." );
	  success = false;
	}
	else
	  t << "\"" << _track->bufferFile() << "\"" << " 0" << "\n";
      }

      t << "\n";
    }

    _track = next();
  }
  
  for( ; _track != 0; _track = next() ) {
    t << "TRACK AUDIO" << "\n";
    if( cdText() ) {
      t << "CD_TEXT {" << "\n";
      t << "  LANGUAGE 0 {" << "\n";
      t << "    TITLE " << "\"" << _track->title() << "\"" << "\n";
      t << "    PERFORMER " << "\"" << _track->artist() << "\"" << "\n";
      t << "    ISRC " << "\"" << _track->isrc() << "\"" << "\n";
      t << "    ARRANGER " << "\"" << _track->arranger() << "\"" << "\n";
      t << "    SONGWRITER " << "\"" << _track->songwriter() << "\"" << "\n";
      t << "    MESSAGE " << "\"" << _track->cdTextMessage() << "\"" << "\n";
      t << "  }" << "\n";
      t << "}" << "\n";
    }

    if( _track->pregap() > 0 ) {
      t << "PREGAP " << K3b::framesToString( _track->pregap() ) << "\n";
    }

    t << "FILE ";
    if( onTheFly() ) {
      t << "\"-\" ";   // read from stdin
      t << K3b::framesToString( _trackStart );        // where does the track start in stdin
      t << " " << K3b::framesToString( _track->length() );   // here we need the perfect length !!!!!
      t << "\n";

      _trackStart += _track->length();
    }
    else {
      if( _track->bufferFile().isEmpty() ) {
 	t << "\"" << _track->absPath() << "\"" << " 0" << "\n";
	qDebug( "(K3bAudioDoc) not all files buffered. toc-file cannot be used for writing." );
	success = false;
      }
      else
	t << "\"" << _track->bufferFile() << "\"" << " 0" << "\n";
    }

    t << "\n";
  }
  // --------------------------------- TOC --	
	
  file.close();

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
  if( onTheFly() )
    return new K3bAudioOnTheFlyJob( this );
  else
    return new K3bAudioJob( this );
}


#include "k3baudiodoc.moc"
