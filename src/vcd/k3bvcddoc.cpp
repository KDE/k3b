/***************************************************************************
                          k3bvcddoc.cpp  -  description
                             -------------------
    begin                : Mon Nov 4 2002
    copyright            : (C) 2002 by Sebastian Trueg
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
#include "../tools/k3bglobals.h"
#include "k3bvcddoc.h"
#include "k3bvcdview.h"
#include "k3bvcdtrack.h"
#include "k3bvcdburndialog.h"
#include "k3bvcdmpegfactory.h"
#include "k3bvcdjob.h"
#include "../tools/kstringlistdialog.h"

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
#include <kmimemagic.h>


K3bVcdDoc::K3bVcdDoc( QObject* parent )
  : K3bDoc( parent )
{
  m_tracks = 0L;
  m_vcdOptions = 0L;
  
  m_docType = VCD;
  m_vcdType = NONE;

  m_urlAddingTimer = new QTimer( this );
  connect( m_urlAddingTimer, SIGNAL(timeout()), this, SLOT(slotWorkUrlQueue()) );
}

K3bVcdDoc::~K3bVcdDoc()
{
  if( m_tracks ) {
    m_tracks->setAutoDelete( true );
    delete m_tracks;
  }

  if ( m_vcdOptions )
    delete m_vcdOptions;
}

bool K3bVcdDoc::newDocument()
{
  if( m_tracks )
    m_tracks->setAutoDelete( true );

  delete m_tracks;

  m_tracks = new QPtrList<K3bVcdTrack>;
  m_tracks->setAutoDelete( false );

  m_vcdOptions = new K3bVcdOptions();

  return K3bDoc::newDocument();
}

unsigned long long K3bVcdDoc::calcTotalSize() const
{
  unsigned long long sum = 0;
  if ( m_tracks ) {
    for ( K3bVcdTrack* track = m_tracks->first(); track; track = m_tracks->next() ) {
      sum += track->size();
    }  
  }
  return sum;
}

unsigned long long K3bVcdDoc::size() const
{
  return calcTotalSize();
}

unsigned long long K3bVcdDoc::length() const
{
  return size();
}

void K3bVcdDoc::addUrl( const KURL& url )
{
  addTrack( url, m_tracks->count() );
}

void K3bVcdDoc::addUrls( const KURL::List& urls )
{
  addTracks( urls, m_tracks->count() );
}

void K3bVcdDoc::addTracks( const KURL::List& urls, uint position )
{
  for( KURL::List::ConstIterator it = urls.begin(); it != urls.end(); it++ ) {
    urlsToAdd.enqueue( new PrivateUrlToAdd( *it, position++ ) );
  }

  m_urlAddingTimer->start(0);
}

void K3bVcdDoc::slotWorkUrlQueue()
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

    if( K3bVcdTrack* newTrack = createTrack( item->url ) )
      addTrack( newTrack, lastAddedPosition );

    delete item;

    emit newTracks();
  }
  else {
    m_urlAddingTimer->stop();

    emit newTracks();

    informAboutNotFoundFiles();
  }
}


K3bVcdTrack* K3bVcdDoc::createTrack( const KURL& url )
{
  // QString mimetype = KMimeMagic::self()->findFileType(url.path())->mimeType();
  // if ( mimetype.contains( "video" ) ) {
  kdDebug() << QString("(K3bVcdDoc) createTrack url.path = ").arg(url.path()) << endl;
  if (int mpeg = identifyMpegFile( url ) > 0) {
    if (vcdType() == NONE) {
      setVcdType(vcdTypes(mpeg+1));
    }

    if (vcdType() != vcdTypes(mpeg+1)) {
      KMessageBox::information( kapp->mainWidget(), "(" + url.path() + ")\n" +
        i18n("You can't mix MPEG1 and MPEG2 video files."),
        i18n("Please start a new project for this filetype"),
        i18n("Resample not implemented yet :(") );
      return 0;
    }
    
    K3bVcdTrack* newTrack =  new K3bVcdTrack( m_tracks, url.path() );
    newTrack->setMimeType(QString("Mpeg%1").arg(mpeg));
    return newTrack;
  }
  else {
    KMessageBox::error( kapp->mainWidget(), "(" + url.path() + ")\n" +
      i18n("Only MPEG1 and MPEG2 video files are supported."),
      i18n("Wrong File Format") );
    return 0;
  }
}


void K3bVcdDoc::addTrack(const KURL& url, uint position )
{
  urlsToAdd.enqueue( new PrivateUrlToAdd( url, position ) );

  m_urlAddingTimer->start(0);
}


void K3bVcdDoc::addTrack( K3bVcdTrack* track, uint position )
{
  if( m_tracks->count() >= 98 ) {
    kdDebug() << "(K3bVcdDoc) VCD Green Book (Red Book) only allows 98 tracks." << endl;
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


void K3bVcdDoc::removeTrack( K3bVcdTrack* track )
{
  if( !track ) {
    return;
  }

  // set the current item to track
  if( m_tracks->findRef( track ) >= 0 ) {
    // take the current item
    track = m_tracks->take();

    // emit signal before deleting the track to avoid crashes
    // when the view tries to call some of the tracks' methods
    emit newTracks();

    delete track;
    kdDebug() << QString("(K3bVcdDoc) removeTrack count = %1").arg(numOfTracks()) << endl;
    if (numOfTracks() == 0)
      this->setVcdType(NONE);
  }
}

void K3bVcdDoc::moveTrack( const K3bVcdTrack* track, const K3bVcdTrack* after )
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
}


K3bView* K3bVcdDoc::newView( QWidget* parent )
{
  return new K3bVcdView( this, parent );
}


QString K3bVcdDoc::documentType() const
{
  return "k3b_vcd_project";
}

void K3bVcdDoc::addView(K3bView* view)
{
  K3bDoc::addView( view );
}

K3bBurnJob* K3bVcdDoc::newBurnJob()
{
  return new K3bVcdJob( this );
}

unsigned int K3bVcdDoc::identifyMpegFile( const KURL& url )
{
  // return 0 = Unknown file type, 1 = Mpeg 1, 2 = Mpeg 2
  return K3bVcdMpegFactory::self()->getMpegFileType( url );
}

void K3bVcdDoc::informAboutNotFoundFiles()
{
  if( !m_notFoundFiles.isEmpty() ) {
    KStringListDialog d( m_notFoundFiles, i18n("Not found"), i18n("Could not find the following files:"),
      true, k3bMain(), "notFoundFilesInfoDialog" );
    d.exec();

    m_notFoundFiles.clear();
  }
}

void K3bVcdDoc::setVcdType( int type )
{
  m_vcdType = type;
}

void K3bVcdDoc::loadDefaultSettings()
{
  KConfig* c = k3bMain()->config();

  c->setGroup( "default vcd settings" );

  setDummy( c->readBoolEntry( "dummy_mode", false ) );
  setDao( c->readBoolEntry( "dao", true ) );
  setOnTheFly( c->readBoolEntry( "on_the_fly", false ) );

  // m_removeBufferFiles = c->readBoolEntry( "remove_buffer_files", true );
}


bool K3bVcdDoc::loadDocumentData( QDomDocument* doc )
{
}


bool K3bVcdDoc::saveDocumentData( QDomDocument* )
{
}


#include "k3bvcddoc.moc"
