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
#include <kaction.h>


// application specific includes
#include "k3bview.h"
#include "k3bdoc.h"
#include "k3bprojectburndialog.h"
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

#include <kostore/koStore.h>
#include <kostore/koStoreDevice.h>


K3bDoc::K3bDoc( QObject* parent )
  : QObject( parent ),
    m_view(0)
    //    m_dcopInterface( 0 )
{
  // register the project with the manager
  k3bprojectmanager->addProject( this );

  m_burner = 0;
  m_onTheFly = true;
  m_overburn = false;
  m_burnproof = true;
  m_speed = 1;

  m_writingApp = K3b::DEFAULT;
  m_writingMode = K3b::WRITING_MODE_AUTO;
  m_saved = false;

  m_actionCollection = new KActionCollection( this );

  (void)new KAction( i18n("&Burn..."), "cdburn", CTRL + Key_B, this, SLOT(slotBurn()),
		     actionCollection(), "project_burn");
  (void)new KAction( i18n("&Properties..."), "edit", CTRL + Key_P, this, SLOT(slotProperties()),
		     actionCollection(), "project_properties");
}


K3bDoc::~K3bDoc()
{
  // remove the project from the manager
  k3bprojectmanager->removeProject( this );

//   if( m_dcopInterface )
//     delete m_dcopInterface;
}


// K3bProjectInterface* K3bDoc::dcopInterface()
// {
//   if( !m_dcopInterface )
//     m_dcopInterface = new K3bProjectInterface( this );
//   return m_dcopInterface;
// }


// QCString K3bDoc::dcopId()
// {
//   return dcopInterface()->objId();
// }


void K3bDoc::setDummy( bool b )
{
  m_dummy = b;
}

void K3bDoc::setSpeed( int speed )
{
  m_speed = speed;
}

void K3bDoc::setBurner( K3bDevice* dev )
{
  m_burner = dev;
  if( dev ) {
    if( !dev->burnproof() )
      setBurnproof( false );
  }
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
      newDoc->setSaved(true);
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


// void K3bDoc::disable()
// {
//   K3bView* view = pViewList->first();
//   while( view ) {
//     view->setDisabled( true );
//     view = pViewList->next();
//   }
// }


// void K3bDoc::enable()
// {
//   K3bView* view = pViewList->first();
//   while( view ) {
//     view->setEnabled( true );
//     view = pViewList->next();
//   }
// }


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

  setDummy( c->readBoolEntry( "dummy_mode", false ) );
  setOnTheFly( c->readBoolEntry( "on_the_fly", true ) );
  setBurnproof( c->readBoolEntry( "burnproof", true ) );
  setRemoveImages( c->readBoolEntry( "remove_image", true ) );
  setOnlyCreateImages( c->readBoolEntry( "only_create_image", false ) );

  setBurner( k3bcore->deviceManager()->findDevice( c->readEntry( "writer_device" ) ) );
  setSpeed( c->readNumEntry( "writing_speed", burner() ? burner()->currentWriteSpeed() : 1 ) );
}


void K3bDoc::slotBurn()
{
  if( numOfTracks() == 0 || size() == 0 ) {
    KMessageBox::information( qApp->activeWindow(), i18n("Please add files to your project first!"),
			      i18n("No Data to Burn"), QString::null, false );
  }
  else {
    K3bProjectBurnDialog* dlg = newBurnDialog( qApp->activeWindow() );
    if( dlg ) {
      dlg->exec(true);
      delete dlg;
    }
    else {
      kdDebug() << "(K3bDoc) Error: no burndialog available." << endl;
    }
  }
}


void K3bDoc::slotProperties()
{
  K3bProjectBurnDialog* dlg = newBurnDialog( qApp->activeWindow() );
  if( dlg ) {
    dlg->exec(false);
    delete dlg;
  }
  else {
    kdDebug() << "(K3bDoc) Error: no burndialog available." << endl;
  }
}

#include "k3bdoc.moc"
