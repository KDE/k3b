/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bprojectmanager.h"

#include "k3bprojectinterface.h"
#include "k3bdataprojectinterface.h"
#include "k3baudioprojectinterface.h"
#include "k3bmixedprojectinterface.h"
#include "kostore/koStore.h"
#include "kostore/koStoreDevice.h"
#include "k3bapplication.h"
#include "k3binteractiondialog.h"

#include <k3baudiodoc.h>
#include <k3baudiodatasourceiterator.h>
#include <k3baudiocdtracksource.h>
#include <k3bdatadoc.h>
#include <k3bdvddoc.h>
#include <k3bvideodvddoc.h>
#include <k3bmixeddoc.h>
#include <k3bvcddoc.h>
#include <k3bmovixdoc.h>
#include <k3bmovixdvddoc.h>
#include <k3bglobals.h>
#include <k3bisooptions.h>
#include <k3bdevicemanager.h>

#include <qptrlist.h>
#include <qmap.h>
#include <qtextstream.h>
#include <qdom.h>
#include <qfile.h>
#include <qapplication.h>
#include <qcursor.h>

#include <kurl.h>
#include <kdebug.h>
#include <kconfig.h>
#include <kio/netaccess.h>
#include <klocale.h>
#include <kmessagebox.h>


class K3bProjectManager::Private
{
public:
  QPtrList<K3bDoc> projects;
  K3bDoc* activeProject;
  QMap<K3bDoc*, K3bProjectInterface*> projectInterfaceMap;

  int audioUntitledCount;
  int dataUntitledCount;
  int mixedUntitledCount;
  int vcdUntitledCount;
  int movixUntitledCount;
  int movixDvdUntitledCount;
  int dvdUntitledCount;
  int videoDvdUntitledCount;
};




K3bProjectManager::K3bProjectManager( QObject* parent, const char* name )
  : QObject( parent, name )
{
  d = new Private();
  d->activeProject = 0;

  d->audioUntitledCount = 0;
  d->dataUntitledCount = 0;
  d->mixedUntitledCount = 0;
  d->vcdUntitledCount = 0;
  d->movixUntitledCount = 0;
  d->movixDvdUntitledCount = 0;
  d->dvdUntitledCount = 0;
  d->videoDvdUntitledCount = 0;
}

K3bProjectManager::~K3bProjectManager()
{
  delete d;
}


const QPtrList<K3bDoc>& K3bProjectManager::projects() const
{
  return d->projects;
}


void K3bProjectManager::addProject( K3bDoc* doc )
{
  if( !d->projects.containsRef( doc ) ) {
    kdDebug() << "(K3bProjectManager) adding doc " << doc->URL().path() << endl;
    
    d->projects.append(doc);

    connect( doc, SIGNAL(changed(K3bDoc*)),
	     this, SLOT(slotProjectChanged(K3bDoc*)) );

    emit newProject( doc );
  }
}


void K3bProjectManager::removeProject( K3bDoc* doc )
{
  //
  // QPtrList.findRef seems to be buggy. Everytime we search for the
  // first added item it is not found!
  //
  for( QPtrListIterator<K3bDoc> it( d->projects );
       it.current(); ++it ) {
    if( it.current() == doc ) {

      // remove the DCOP interface
      QMap<K3bDoc*, K3bProjectInterface*>::iterator it = d->projectInterfaceMap.find( doc );
      if( it != d->projectInterfaceMap.end() ) {
	// delete the interface
	delete it.data();
	d->projectInterfaceMap.remove( it );
      }

      d->projects.removeRef(doc);
      emit closingProject(doc);

      return;
    }
  }
  kdDebug() << "(K3bProjectManager) unable to find doc: " << doc->URL().path() << endl;
}


K3bDoc* K3bProjectManager::findByUrl( const KURL& url )
{
  for( QPtrListIterator<K3bDoc> it( d->projects );
       it.current(); ++it ) {
    K3bDoc* doc = it.current();
    if( doc->URL() == url )
      return doc;
  }
  return 0;
}


bool K3bProjectManager::isEmpty() const
{
  return d->projects.isEmpty();
}


void K3bProjectManager::setActive( K3bDoc* doc )
{
  if( !doc ) {
    d->activeProject = 0L;
    emit activeProjectChanged( 0L );
    return;
  }

  //
  // QPtrList.findRef seems to be buggy. Everytime we search for the
  // first added item it is not found!
  //
  for( QPtrListIterator<K3bDoc> it( d->projects );
       it.current(); ++it ) {
    if( it.current() == doc ) {
      d->activeProject = doc;
      emit activeProjectChanged(doc);
    }
  }
}


K3bDoc* K3bProjectManager::activeProject() const
{
  return d->activeProject;
}


K3bDoc* K3bProjectManager::createEmptyProject( K3bDoc::DocType type )
{
  K3bDoc* doc = 0;
  QString fileName;

  switch( type ) {
  case K3bDoc::AUDIO: {
    doc = new K3bAudioDoc( this );
    fileName = i18n("AudioCD%1").arg(d->audioUntitledCount++);
    break;
  }

  case K3bDoc::DATA: {
    doc = new K3bDataDoc( this );
    fileName = i18n("DataCD%1").arg(d->dataUntitledCount++);
    break;
  }

  case K3bDoc::MIXED: {
    doc = new K3bMixedDoc( this );
    fileName=i18n("MixedCD%1").arg(d->mixedUntitledCount++);
    break;
  }

  case K3bDoc::VCD: {
    doc = new K3bVcdDoc( this );
    fileName=i18n("VideoCD%1").arg(d->vcdUntitledCount++);
    break;
  }

  case K3bDoc::MOVIX: {
    doc = new K3bMovixDoc( this );
    fileName=i18n("eMovixCD%1").arg(d->movixUntitledCount++);
    break;
  }

  case K3bDoc::MOVIX_DVD: {
    doc = new K3bMovixDvdDoc( this );
    fileName=i18n("eMovixDVD%1").arg(d->movixDvdUntitledCount++);
    break;
  }

  case K3bDoc::DVD: {
    doc = new K3bDvdDoc( this );
    fileName = i18n("DataDVD%1").arg(d->dvdUntitledCount++);
    break;
  }
      
  case K3bDoc::VIDEODVD: {
    doc = new K3bVideoDvdDoc( this );
    fileName = i18n("VideoDVD%1").arg(d->videoDvdUntitledCount++);
    break;
  }
  }

  KURL url;
  url.setFileName(fileName);
  doc->setURL(url);

  doc->newDocument();
  
  loadDefaults( doc );

  return doc;
}


K3bDoc* K3bProjectManager::createProject( K3bDoc::DocType type )
{
  K3bDoc* doc = createEmptyProject( type );

  // create the dcop interface
  dcopInterface( doc );

  addProject( doc );

  return doc;
}


void K3bProjectManager::loadDefaults( K3bDoc* doc )
{
  KConfig* c = kapp->config();

  QString oldGroup = c->group();

  QString cg = "default " + doc->typeString() + " settings";

  // earlier K3b versions loaded the saved settings
  // so that is what we do as a default
  int i = KConfigGroup( c, "General Options" ).readNumEntry( "action dialog startup settings", 
							     K3bInteractionDialog::LOAD_SAVED_SETTINGS );
  if( i == K3bInteractionDialog::LOAD_K3B_DEFAULTS )
    return; // the default k3b settings are the ones everyone starts with
  else if( i == K3bInteractionDialog::LOAD_LAST_SETTINGS )
    cg = "last used " + cg;

  c->setGroup( cg );

  QString mode = c->readEntry( "writing_mode" );
  if ( mode == "dao" )
    doc->setWritingMode( K3b::DAO );
  else if( mode == "tao" )
    doc->setWritingMode( K3b::TAO );
  else if( mode == "raw" )
    doc->setWritingMode( K3b::RAW );
  else
    doc->setWritingMode( K3b::WRITING_MODE_AUTO );

  doc->setDummy( c->readBoolEntry( "simulate", false ) );
  doc->setOnTheFly( c->readBoolEntry( "on_the_fly", true ) );
  doc->setRemoveImages( c->readBoolEntry( "remove_image", true ) );
  doc->setOnlyCreateImages( c->readBoolEntry( "only_create_image", false ) );
  doc->setBurner( k3bcore->deviceManager()->findDevice( c->readEntry( "writer_device" ) ) );
  // Default = 0 (Auto)
  doc->setSpeed( c->readNumEntry( "writing_speed", 0 ) );
  doc->setWritingApp( K3b::writingAppFromString( c->readEntry( "writing_app" ) ) );


  switch( doc->type() ) {
  case K3bDoc::AUDIO: {
    K3bAudioDoc* audioDoc = static_cast<K3bAudioDoc*>(doc);

    audioDoc->writeCdText( c->readBoolEntry( "cd_text", true ) );
    audioDoc->setHideFirstTrack( c->readBoolEntry( "hide_first_track", false ) );
    audioDoc->setNormalize( c->readBoolEntry( "normalize", false ) );
    audioDoc->setAudioRippingParanoiaMode( c->readNumEntry( "paranoia mode", 0 ) );
    audioDoc->setAudioRippingRetries( c->readNumEntry( "read retries", 128 ) );
    audioDoc->setAudioRippingIgnoreReadErrors( c->readBoolEntry( "ignore read errors", false ) );

    break;
  }

  case K3bDoc::MOVIX:
  case K3bDoc::MOVIX_DVD: {
    K3bMovixDoc* movixDoc = static_cast<K3bMovixDoc*>(doc);

    movixDoc->setSubtitleFontset( c->readEntry("subtitle_fontset") );

    movixDoc->setLoopPlaylist( c->readNumEntry("loop", 1 ) );
    movixDoc->setAdditionalMPlayerOptions( c->readEntry( "additional_mplayer_options" ) );
    movixDoc->setUnwantedMPlayerOptions( c->readEntry( "unwanted_mplayer_options" ) );

    movixDoc->setBootMessageLanguage( c->readEntry("boot_message_language") );

    movixDoc->setDefaultBootLabel( c->readEntry( "default_boot_label" ) );

    movixDoc->setShutdown( c->readBoolEntry( "shutdown", false) );
    movixDoc->setReboot( c->readBoolEntry( "reboot", false ) );
    movixDoc->setEjectDisk( c->readBoolEntry( "eject", false ) );
    movixDoc->setRandomPlay( c->readBoolEntry( "random_play", false ) );
    movixDoc->setNoDma( c->readBoolEntry( "no_dma", false ) );
    // fallthrough
  }

  case K3bDoc::DATA:
  case K3bDoc::DVD: {
    K3bDataDoc* dataDoc = static_cast<K3bDataDoc*>(doc);

    dataDoc->setIsoOptions( K3bIsoOptions::load( c, false ) );

    QString datamode = c->readEntry( "data_track_mode" );
    if( datamode == "mode1" )
      dataDoc->setDataMode( K3b::MODE1 );
    else if( datamode == "mode2" )
      dataDoc->setDataMode( K3b::MODE2 );
    else
      dataDoc->setDataMode( K3b::DATA_MODE_AUTO );
    
    dataDoc->setVerifyData( c->readBoolEntry( "verify data", false ) );

    QString s = c->readEntry( "multisession mode" );
    if( s == "none" )
      dataDoc->setMultiSessionMode( K3bDataDoc::NONE );
    else if( s == "start" )
      dataDoc->setMultiSessionMode( K3bDataDoc::START );
    else if( s == "continue" )
      dataDoc->setMultiSessionMode( K3bDataDoc::CONTINUE );
    else if( s == "finish" )
      dataDoc->setMultiSessionMode( K3bDataDoc::FINISH );
    else
      dataDoc->setMultiSessionMode( K3bDataDoc::AUTO );

    break;
  }

  case K3bDoc::VIDEODVD: {
    // the only defaults we need here are the volume id and stuff
    K3bDataDoc* dataDoc = static_cast<K3bDataDoc*>(doc);
    dataDoc->setIsoOptions( K3bIsoOptions::load( c, false ) );
    dataDoc->setVerifyData( c->readBoolEntry( "verify data", false ) );
    break;
  }

  case K3bDoc::MIXED: {
    K3bMixedDoc* mixedDoc = static_cast<K3bMixedDoc*>(doc);

    mixedDoc->audioDoc()->writeCdText( c->readBoolEntry( "cd_text", true ) );
    mixedDoc->audioDoc()->setNormalize( c->readBoolEntry( "normalize", false ) );

    // load mixed type
    if( c->readEntry( "mixed_type" ) == "last_track" )
      mixedDoc->setMixedType( K3bMixedDoc::DATA_LAST_TRACK );
    else if( c->readEntry( "mixed_type" ) == "first_track" )
      mixedDoc->setMixedType( K3bMixedDoc::DATA_FIRST_TRACK );
    else
      mixedDoc->setMixedType( K3bMixedDoc::DATA_SECOND_SESSION );

    QString datamode = c->readEntry( "data_track_mode" );
    if( datamode == "mode1" )
      mixedDoc->dataDoc()->setDataMode( K3b::MODE1 );
    else if( datamode == "mode2" )
      mixedDoc->dataDoc()->setDataMode( K3b::MODE2 );
    else
      mixedDoc->dataDoc()->setDataMode( K3b::DATA_MODE_AUTO );

    mixedDoc->dataDoc()->setIsoOptions( K3bIsoOptions::load( c, false ) );

    if( mixedDoc->dataDoc()->isoOptions().volumeID().isEmpty() )
      mixedDoc->dataDoc()->setVolumeID( doc->URL().fileName() );

    break;
  }

  case K3bDoc::VCD: {
    K3bVcdDoc* vcdDoc = static_cast<K3bVcdDoc*>(doc);

    // FIXME: I think we miss a lot here!

    vcdDoc->vcdOptions()->setPbcEnabled( c->readBoolEntry( "Use Playback Control", false ) );
    vcdDoc->vcdOptions()->setPbcNumkeysEnabled( c->readBoolEntry( "Use numeric keys to navigate chapters", false ) );
    vcdDoc->vcdOptions()->setPbcPlayTime( c->readNumEntry( "Play each Sequence/Segment", 1 ) );
    vcdDoc->vcdOptions()->setPbcWaitTime( c->readNumEntry( "Time to wait after each Sequence/Segment", 2 ) );

    if( vcdDoc->vcdOptions()->volumeId().isEmpty() )
      vcdDoc->vcdOptions()->setVolumeId( doc->URL().fileName() );

    break;
  }
  }

  if( doc->type() == K3bDoc::DATA ||
      doc->type() == K3bDoc::MOVIX ||
      doc->type() == K3bDoc::MOVIX_DVD ||
      doc->type() == K3bDoc::VIDEODVD ||
      doc->type() == K3bDoc::DVD ) {
    if( static_cast<K3bDataDoc*>(doc)->isoOptions().volumeID().isEmpty() )
      static_cast<K3bDataDoc*>(doc)->setVolumeID( doc->URL().fileName() );
  }

  doc->setModified( false );

  c->setGroup( oldGroup );
}


K3bProjectInterface* K3bProjectManager::dcopInterface( K3bDoc* doc )
{
  QMap<K3bDoc*, K3bProjectInterface*>::iterator it = d->projectInterfaceMap.find( doc );
  if( it == d->projectInterfaceMap.end() ) {
    K3bProjectInterface* dcopInterface = 0;
    if( doc->type() == K3bDoc::DATA || doc->type() == K3bDoc::DVD )
      dcopInterface = new K3bDataProjectInterface( static_cast<K3bDataDoc*>(doc) );
    else if( doc->type() == K3bDoc::AUDIO )
      dcopInterface = new K3bAudioProjectInterface( static_cast<K3bAudioDoc*>(doc) );
    else if( doc->type() == K3bDoc::MIXED )
      dcopInterface = new K3bMixedProjectInterface( static_cast<K3bMixedDoc*>(doc) );
    else
      dcopInterface = new K3bProjectInterface( doc );
    d->projectInterfaceMap[doc] = dcopInterface;
    return dcopInterface;
  }
  else
    return it.data();
}


K3bDoc* K3bProjectManager::openProject( const KURL& url )
{
  QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

  QString tmpfile;
  KIO::NetAccess::download( url, tmpfile, 0L );

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
	  QApplication::restoreOverrideCursor();
	  return 0;
	}
	f.reset();
      }
      else {
	kdDebug() << "(K3bDoc) could not read from file." << endl;
	QApplication::restoreOverrideCursor();
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
    QApplication::restoreOverrideCursor();
    return 0;
  }

  // check the documents DOCTYPE
  K3bDoc::DocType type = K3bDoc::AUDIO;
  if( xmlDoc.doctype().name() == "k3b_audio_project" )
    type = K3bDoc::AUDIO;
  else if( xmlDoc.doctype().name() == "k3b_data_project" )
    type = K3bDoc::DATA;
  else if( xmlDoc.doctype().name() == "k3b_vcd_project" )
    type = K3bDoc::VCD;
  else if( xmlDoc.doctype().name() == "k3b_mixed_project" )
    type = K3bDoc::MIXED;
  else if( xmlDoc.doctype().name() == "k3b_movix_project" )
    type = K3bDoc::MOVIX;
  else if( xmlDoc.doctype().name() == "k3b_movixdvd_project" )
    type = K3bDoc::MOVIX_DVD;
  else if( xmlDoc.doctype().name() == "k3b_dvd_project" )
    type = K3bDoc::DVD;
  else if( xmlDoc.doctype().name() == "k3b_video_dvd_project" )
    type = K3bDoc::VIDEODVD;
  else {
    kdDebug() << "(K3bDoc) unknown doc type: " << xmlDoc.doctype().name() << endl;
    QApplication::restoreOverrideCursor();
    return 0;
  }

  // we do not know yet if we will be able to actually open the project, so don't inform others yet
  K3bDoc* newDoc = createEmptyProject( type );

  // ---------
  // load the data into the document
  QDomElement root = xmlDoc.documentElement();
  if( newDoc->loadDocumentData( &root ) ) {
    newDoc->setURL( url );
    newDoc->setSaved( true );
    newDoc->setModified( false );

    // ok, finish the doc setup, inform the others about the new project
    dcopInterface( newDoc );
    addProject( newDoc );

    // FIXME: find a better way to tell everyone (especially the projecttabwidget)
    //        that the doc is not changed
    emit projectSaved( newDoc );

    kdDebug() << "(K3bProjectManager) loading project done." << endl;
  }
  else {
    delete newDoc;
    newDoc = 0;
  }

  QApplication::restoreOverrideCursor();

  return newDoc;
}


bool K3bProjectManager::saveProject( K3bDoc* doc, const KURL& url )
{
  QString tmpfile;
  KIO::NetAccess::download( url, tmpfile, 0L );

  bool success = false;

  // create the store
  KoStore* store = KoStore::createStore( tmpfile, KoStore::Write, "application/x-k3b" );
  if( store ) {
    if( store->bad() ) {
      delete store;
    }
    else {
      // open the document inside the store
      store->open( "maindata.xml" );
      
      // save the data in the document
      QDomDocument xmlDoc( "k3b_" + doc->typeString() + "_project" );
      
      xmlDoc.appendChild( xmlDoc.createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"UTF-8\"" ) );
      QDomElement docElem = xmlDoc.createElement( "k3b_" + doc->typeString() + "_project" );
      xmlDoc.appendChild( docElem );
      success = doc->saveDocumentData( &docElem );
      if( success ) {
	KoStoreDevice dev(store);
	dev.open( IO_WriteOnly );
	QTextStream xmlStream( &dev );
	xmlDoc.save( xmlStream, 0 );
	
	doc->setURL( url );
	doc->setModified( false );
      }
      
      // close the document inside the store
      store->close();
      
      // remove the store (destructor writes the store to disk)
      delete store;

      doc->setSaved( success );

      if( success ) {
	emit projectSaved( doc );
      }
    }
  }

  KIO::NetAccess::removeTempFile( tmpfile );

  return success;
}


void K3bProjectManager::slotProjectChanged( K3bDoc* doc )
{
  emit projectChanged( doc );
}

#include "k3bprojectmanager.moc"
