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
#include "../k3bprogressdialog.h"
#include "k3baudioburndialog.h"
#include "k3baudiojob.h"
#include "k3baudioontheflyjob.h"
#include "input/k3baudiomodulefactory.h"


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
	
  return K3bDoc::newDocument();
}



int K3bAudioDoc::size() const {
  int size = 0;
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
      addTrack( new K3bAudioTrack( m_tracks, addedFile.path() ), lastAddedPosition );
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
    //    emit newTracks();

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


bool K3bAudioDoc::loadDocumentData( QDomDocument* )
{
  // TODO: so what? load the shit! ;-)
  return true;
}

bool K3bAudioDoc::saveDocumentData( QDomDocument* )
{
  // TODO: some saving work...
  return true;
}


void K3bAudioDoc::addView(K3bView* view)
{
  K3bAudioView* v = (K3bAudioView*)view;
  connect( v, SIGNAL(dropped(const QStringList&, uint)), this, SLOT(addTracks(const QStringList&, uint)) );
	
  K3bDoc::addView( view );
}


QString K3bAudioDoc::writeTOC( const QString& filename )
{
  QFile file( filename );
  if( !file.open( IO_WriteOnly ) ) {
    qDebug( "(K3bAudioDoc) Could not open toc-file %s", filename.latin1() );
    return QString::null;
  }

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
  for( K3bAudioTrack* _track = at(0); _track != 0; _track = next() ) {
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
      }
      else
	t << "\"" << _track->bufferFile() << "\"" << " 0" << "\n";
    }

    t << "\n";
  }
  // --------------------------------- TOC --	
	
  file.close();
  return filename;
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
