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


// include files for Qt
#include <qdir.h>
#include <qfileinfo.h>
#include <qwidget.h>
#include <qstring.h>
#include <qdatetime.h>
#include <qfile.h>
#include <qtimer.h>
#include <qdom.h>
#include <qtextstream.h>
#include <qapplication.h>

// include files for KDE
#include <klocale.h>
#include <kmessagebox.h>
#include <kfiledialog.h>
#include <kio/job.h>
#include <kio/netaccess.h>
#include <kprocess.h>
#include <kstandarddirs.h>
#include <kconfig.h>
#include <kdebug.h>


// application specific includes
#include "k3bview.h"
#include "k3bdoc.h"
#include "k3bprojectmanager.h"
//#include "k3bprojectinterface.h"
#include <k3bglobals.h>
#include <k3bdevice.h>
#include <k3bmsf.h>
#include <k3baudiodoc.h>
#include <k3bdatadoc.h>
#include <k3bvcddoc.h>
#include <k3bmixeddoc.h>
#include <k3bmovixdoc.h>
#include <k3bmovixdvddoc.h>
#include <k3bdvddoc.h>
#include <k3bvideodvddoc.h>
#include <k3bcore.h>
#include <k3bdevicemanager.h>

#include "kostore/koStore.h"
#include "kostore/koStoreDevice.h"


K3bDoc::K3bDoc( QObject* parent )
  : QObject( parent ),
    m_view(0)
{
  // register the project with the manager
  k3bprojectmanager->addProject( this );

  m_burner = 0;
  m_onTheFly = true;
  m_overburn = false;
  m_speed = 0;  // Auto

  m_writingApp = K3b::DEFAULT;
  m_writingMode = K3b::WRITING_MODE_AUTO;
  m_saved = false;

  m_copies = 1;

  connect( this, SIGNAL(changed()), this, SLOT(slotChanged()) );
}


K3bDoc::~K3bDoc()
{
  // remove the project from the manager
  k3bprojectmanager->removeProject( this );
}


void K3bDoc::slotChanged()
{
  emit changed( this );
}


void K3bDoc::setDummy( bool b )
{
  m_dummy = b;
}

void K3bDoc::setSpeed( int speed )
{
  m_speed = speed;
}

void K3bDoc::setBurner( K3bDevice::Device* dev )
{
  m_burner = dev;
}


void K3bDoc::addUrl( const KURL& url )
{
  KURL::List urls(url);
  addUrls( urls );
}


K3bView* K3bDoc::createView( QWidget* parent, const char* )
{
  m_view = newView( parent );
  m_view->setCaption( URL().fileName() );

  return m_view;
}


void K3bDoc::setURL( const KURL& url )
{
  doc_url = url;
  if( view() )
    view()->setCaption( url.fileName() );
}

const KURL& K3bDoc::URL() const
{
  return doc_url;
}


bool K3bDoc::newDocument()
{
  setModified( false );
  m_copies = 1;

  return true;
}


K3bDoc* K3bDoc::openDocument(const KURL& url )
{
  QString tmpfile;
  KIO::NetAccess::download( url, tmpfile );

  // ///////////////////////////////////////////////
  // first check if it's a store or an old plain xml file
  bool success = false;
  QDomDocument xmlDoc;

  // try opening a store
  KoStore* store = KoStore::createStore( tmpfile, KoStore::Read );
  if( store ) {
    if( !store->bad() ) {
      // try opening the document inside the store
      if( store->open( "maindata.xml" ) ) {
	QIODevice* dev = store->device();
	dev->open( IO_ReadOnly );
	if( xmlDoc.setContent( dev ) )
	  success = true;
	dev->close();
	store->close();
      }
    }

    delete store;
  }

  if( !success ) {
    // try reading an old plain document
    QFile f( tmpfile );
    if ( f.open( IO_ReadOnly ) ) {
      //
      // First check if this is really an xml file beacuse if this is a very big file
      // the setContent method blocks for a very long time
      //
      char test[5];
      if( f.readBlock( test, 5 ) ) {
	if( ::strncmp( test, "<?xml", 5 ) ) {
	  kdDebug() << "(K3bDoc) " << url.path() << " seems to be no xml file." << endl;
	  return 0;
	}
	f.reset();
      }
      else {
	kdDebug() << "(K3bDoc) could not read from file." << endl;
	return 0;
      }
      if( xmlDoc.setContent( &f ) )
	success = true;
      f.close();
    }
  }

  // ///////////////////////////////////////////////
  KIO::NetAccess::removeTempFile( tmpfile );

  if( !success ) {
    kdDebug() << "(K3bDoc) could not open file " << url.path() << endl;
    return 0;
  }

  // check the documents DOCTYPE
  K3bDoc* newDoc = 0;
  if( xmlDoc.doctype().name() == "k3b_audio_project" )
    newDoc = new K3bAudioDoc( 0 );
  else if( xmlDoc.doctype().name() == "k3b_data_project" )
    newDoc = new K3bDataDoc( 0 );
  else if( xmlDoc.doctype().name() == "k3b_vcd_project" )
    newDoc = new K3bVcdDoc( 0 );
  else if( xmlDoc.doctype().name() == "k3b_mixed_project" )
    newDoc = new K3bMixedDoc( 0 );
  else if( xmlDoc.doctype().name() == "k3b_movix_project" )
    newDoc = new K3bMovixDoc( 0 );
  else if( xmlDoc.doctype().name() == "k3b_movixdvd_project" )
    newDoc = new K3bMovixDvdDoc( 0 );
  else if( xmlDoc.doctype().name() == "k3b_dvd_project" )
    newDoc = new K3bDvdDoc( 0 );
  else if( xmlDoc.doctype().name() == "k3b_video_dvd_project" )
    newDoc = new K3bVideoDvdDoc( 0 );
  else {
    kdDebug() << "(K3bDoc) unknown doc type: " << xmlDoc.doctype().name() << endl;
    return 0;
  }

  // we do this to load the writer and the writing speed
  // since those are not saved in a project file
  // FIXME
  newDoc->loadDefaultSettings( k3bcore->config() );

  // ---------
  // load the data into the document
  if( newDoc != 0 ) {
    newDoc->newDocument();
    QDomElement root = xmlDoc.documentElement();
    if( newDoc->loadDocumentData( &root ) ) {
      newDoc->setURL( url );
      newDoc->m_saved = true;
      return newDoc;
    }
  }


  delete newDoc;
  return 0;
}

bool K3bDoc::saveDocument(const KURL& url )
{
  // create the store
  KoStore* store = KoStore::createStore( url.path(), KoStore::Write, "application/x-k3b" );
  if( !store )
    return false;

  if( store->bad() ) {
    delete store;
    return false;
  }

  // open the document inside the store
  store->open( "maindata.xml" );

  // save the data in the document
  QDomDocument xmlDoc( "k3b_" + documentType() + "_project" );

  xmlDoc.appendChild( xmlDoc.createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"UTF-8\"" ) );
  QDomElement docElem = xmlDoc.createElement( "k3b_" + documentType() + "_project" );
  xmlDoc.appendChild( docElem );
  bool success = saveDocumentData( &docElem );
  if( success ) {
    KoStoreDevice dev(store);
    dev.open( IO_WriteOnly );
    QTextStream xmlStream( &dev );
    xmlDoc.save( xmlStream, 0 );

    setModified( false );
    setURL( url );
  }

  // close the document inside the store
  store->close();

  // remove the store (destructor writes the store to disk)
  delete store;

  m_saved = success;

  if( success ) {
    emit saved();
    emit saved( this );
  }

  return success;
}


bool K3bDoc::saveGeneralDocumentData( QDomElement* part )
{
  QDomDocument doc = part->ownerDocument();
  QDomElement mainElem = doc.createElement( "general" );

  QDomElement propElem = doc.createElement( "writing_mode" );
  switch( writingMode() ) {
  case K3b::DAO:
    propElem.appendChild( doc.createTextNode( "dao" ) );
    break;
  case K3b::TAO:
    propElem.appendChild( doc.createTextNode( "tao" ) );
    break;
  case K3b::RAW:
    propElem.appendChild( doc.createTextNode( "raw" ) );
    break;
  default:
    propElem.appendChild( doc.createTextNode( "auto" ) );
    break;
  }
  mainElem.appendChild( propElem );

  propElem = doc.createElement( "dummy" );
  propElem.setAttribute( "activated", dummy() ? "yes" : "no" );
  mainElem.appendChild( propElem );

  propElem = doc.createElement( "on_the_fly" );
  propElem.setAttribute( "activated", onTheFly() ? "yes" : "no" );
  mainElem.appendChild( propElem );

  propElem = doc.createElement( "only_create_images" );
  propElem.setAttribute( "activated", onlyCreateImages() ? "yes" : "no" );
  mainElem.appendChild( propElem );

  propElem = doc.createElement( "remove_images" );
  propElem.setAttribute( "activated", removeImages() ? "yes" : "no" );
  mainElem.appendChild( propElem );

  part->appendChild( mainElem );

  return true;
}


bool K3bDoc::readGeneralDocumentData( const QDomElement& elem )
{
  if( elem.nodeName() != "general" )
    return false;

  QDomNodeList nodes = elem.childNodes();
  for( uint i = 0; i < nodes.count(); i++ ) {

    QDomElement e = nodes.item(i).toElement();
    if( e.isNull() )
      return false;

    if( e.nodeName() == "writing_mode") {
      QString mode = e.text();
      if( mode == "dao" )
	setWritingMode( K3b::DAO );
      else if( mode == "tao" )
	setWritingMode( K3b::TAO );
      else if( mode == "raw" )
	setWritingMode( K3b::RAW );
      else
	setWritingMode( K3b::WRITING_MODE_AUTO );
    }

    if( e.nodeName() == "dummy")
      setDummy( e.attributeNode( "activated" ).value() == "yes" );

    if( e.nodeName() == "on_the_fly")
      setOnTheFly( e.attributeNode( "activated" ).value() == "yes" );

    if( e.nodeName() == "only_create_images")
      setOnlyCreateImages( e.attributeNode( "activated" ).value() == "yes" );

    if( e.nodeName() == "remove_images")
      setRemoveImages( e.attributeNode( "activated" ).value() == "yes" );
  }


  return true;
}


// TODO: this seems not to be the right place for this. Better put this in K3bView
void K3bDoc::loadDefaultSettings( KConfig* c )
{
  c->setGroup( "default " + documentType() + " settings" );

  QString mode = c->readEntry( "writing_mode" );
  if ( mode == "dao" )
    setWritingMode( K3b::DAO );
  else if( mode == "tao" )
    setWritingMode( K3b::TAO );
  else if( mode == "raw" )
    setWritingMode( K3b::RAW );
  else
    setWritingMode( K3b::WRITING_MODE_AUTO );

  setDummy( c->readBoolEntry( "simulate", false ) );
  setOnTheFly( c->readBoolEntry( "on_the_fly", true ) );
  setRemoveImages( c->readBoolEntry( "remove_image", true ) );
  setOnlyCreateImages( c->readBoolEntry( "only_create_image", false ) );

  setBurner( k3bcore->deviceManager()->findDevice( c->readEntry( "writer_device" ) ) );

  // Default = 0 (Auto)
  setSpeed( c->readNumEntry( "writing_speed", burner() ? burner()->currentWriteSpeed() : 0 ) );

  setWritingApp( K3b::writingAppFromString( c->readEntry( "writing_app" ) ) );
}


#include "k3bdoc.moc"
