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


#include "k3bmovixdoc.h"
#include "k3bmovixview.h"
#include "k3bmovixjob.h"
#include "k3bmovixfileitem.h"

#include <data/k3bdiritem.h>
#include <data/k3bfileitem.h>

#include <klocale.h>
#include <kdebug.h>
#include <kurl.h>
#include <klineeditdlg.h>
#include <kmessagebox.h>
#include <kconfig.h>
#include <kapplication.h>

#include <qdom.h>
#include <qfileinfo.h>


K3bMovixDoc::K3bMovixDoc( QObject* parent )
  : K3bDataDoc( parent )
{
  connect( this, SIGNAL(itemRemoved(K3bDataItem*)),
	   this, SLOT(slotDataItemRemoved(K3bDataItem*)) );
}


K3bMovixDoc::~K3bMovixDoc()
{
}


K3bView* K3bMovixDoc::newView( QWidget* parent )
{
  return new K3bMovixView( this, parent );
}


K3bBurnJob* K3bMovixDoc::newBurnJob()
{
  return new K3bMovixJob(this);
}


bool K3bMovixDoc::newDocument()
{
  m_loopPlaylist = 1;
  m_ejectDisk = false;
  m_reboot = false;
  m_shutdown = false;
  m_randomPlay = false;

  return K3bDataDoc::newDocument();
}


void K3bMovixDoc::addUrls( const KURL::List& urls )
{
  for( KURL::List::ConstIterator it = urls.begin(); it != urls.end(); ++it ) {
    addMovixFile( *it );
  }

  emit newMovixFileItems();
  emit newFileItems();
}


void K3bMovixDoc::addMovixFile( const KURL& url, int pos )
{
  QFileInfo f( url.path() );
  if( !f.isFile() || !url.isLocalFile() )
    return;

  QString newName = f.fileName();
  if( nameAlreadyInDir( newName, root() ) ) {
    kapp->config()->setGroup("Data project settings");
    bool dropDoubles = kapp->config()->readBoolEntry( "Drop doubles", false );
    if( dropDoubles )
      return;
    
    bool ok = true;
    do {
      newName = KLineEditDlg::getText( i18n("A file with that name already exists. Please enter a new name."), 
				       newName, &ok, 0 );
    } while( ok && nameAlreadyInDir( newName, root() ) );
    
    if( !ok )
      return;
  }

  K3bMovixFileItem* newK3bItem = new K3bMovixFileItem( f.absFilePath(), this, root(), newName );
  if( pos < 0 || pos > (int)m_movixFiles.count() )
    pos = m_movixFiles.count();

  m_movixFiles.insert( pos, newK3bItem );

  emit newMovixFileItems();
  emit newFileItems();

  setModified(true);
}


bool K3bMovixDoc::loadDocumentData( QDomElement* rootElem )
{
  if( !root() )
    newDocument();

  QDomNodeList nodes = rootElem->childNodes();

  if( nodes.item(0).nodeName() != "general" ) {
    kdDebug() << "(K3bMovixDoc) could not find 'general' section." << endl;
    return false;
  }
  if( !readGeneralDocumentData( nodes.item(0).toElement() ) )
    return false;


  // parse options
  // -----------------------------------------------------------------
  if( nodes.item(1).nodeName() != "data_options" ) {
    kdDebug() << "(K3bMovixDoc) could not find 'data_options' section." << endl;
    return false;
  }
  if( !loadDocumentDataOptions( nodes.item(1).toElement() ) )
    return false;
  // -----------------------------------------------------------------



  // parse header
  // -----------------------------------------------------------------
  if( nodes.item(2).nodeName() != "data_header" ) {
    kdDebug() << "(K3bMovixDoc) could not find 'data_header' section." << endl;
    return false;
  }
  if( !loadDocumentDataHeader( nodes.item(2).toElement() ) )
    return false;
  // -----------------------------------------------------------------



  // parse movix options
  // -----------------------------------------------------------------
  if( nodes.item(3).nodeName() != "movix_options" ) {
    kdDebug() << "(K3bMovixDoc) could not find 'movix_options' section." << endl;
    return false;
  }

  // load the options
  QDomNodeList optionList = nodes.item(3).childNodes();
  for( uint i = 0; i < optionList.count(); i++ ) {

    QDomElement e = optionList.item(i).toElement();
    if( e.isNull() )
      return false;

    if( e.nodeName() == "shutdown")
      setShutdown( e.attributeNode( "activated" ).value() == "yes" );
    else if( e.nodeName() == "reboot")
      setReboot( e.attributeNode( "activated" ).value() == "yes" );
    else if( e.nodeName() == "eject_disk")
      setEjectDisk( e.attributeNode( "activated" ).value() == "yes" );
    else if( e.nodeName() == "random_play")
      setRandomPlay( e.attributeNode( "activated" ).value() == "yes" );
    else if( e.nodeName() == "no_dma")
      setNoDma( e.attributeNode( "activated" ).value() == "yes" );
    else if( e.nodeName() == "subtitle_fontset")
      setSubtitleFontset( e.text() );
    else if( e.nodeName() == "boot_message_language")
      setBootMessageLanguage( e.text() );
    else if( e.nodeName() == "default_boot_label")
      setDefaultBootLabel( e.text() );
    else if( e.nodeName() == "additional_mplayer_options")
      setAdditionalMPlayerOptions( e.text() );
    else if( e.nodeName() == "unwanted_mplayer_options")
      setUnwantedMPlayerOptions( e.text() );
    else if( e.nodeName() == "loop_playlist")
      setLoopPlaylist( e.text().toInt() );
    else
      kdDebug() << "(K3bMovixDoc) unknown movix option: " << e.nodeName() << endl;
  }
  // -----------------------------------------------------------------

  // parse files
  // -----------------------------------------------------------------
  if( nodes.item(4).nodeName() != "movix_files" ) {
    kdDebug() << "(K3bMovixDoc) could not find 'movix_files' section." << endl;
    return false;
  }

  // load file items
  QDomNodeList fileList = nodes.item(4).childNodes();
  for( uint i = 0; i < fileList.count(); i++ ) {

    QDomElement e = fileList.item(i).toElement();
    if( e.isNull() )
      return false;

    if( e.nodeName() == "file" ) {
      if( !e.hasAttribute( "name" ) ) {
	kdDebug() << "(K3bMovixDoc) found file tag without name attribute." << endl;
	return false;
      }

      QDomElement urlElem = e.firstChild().toElement();
      if( urlElem.isNull() ) {
	kdDebug() << "(K3bMovixDoc) found file tag without url child." << endl;
	return false;
      }

      // create the item
      K3bMovixFileItem* newK3bItem = new K3bMovixFileItem( urlElem.text(), 
							   this, 
							   root(), 
							   e.attributeNode("name").value() );
      m_movixFiles.append( newK3bItem );

      // subtitle file?
      QDomElement subTitleElem = e.childNodes().item(1).toElement();
      if( !subTitleElem.isNull() && subTitleElem.nodeName() == "subtitle_file" ) {
	urlElem = subTitleElem.firstChild().toElement();
	if( urlElem.isNull() ) {
	  kdDebug() << "(K3bMovixDoc) found subtitle_file tag without url child." << endl;
	  return false;
	}

	QString name = K3bMovixFileItem::subTitleFileName( newK3bItem->k3bName() );
	K3bFileItem* subItem = new K3bFileItem( urlElem.text(), this, root(), name );
	newK3bItem->setSubTitleItem( subItem );
      }
    }
    else {
      kdDebug() << "(K3bMovixDoc) found " << e.nodeName() << " node where 'file' was expected." << endl;
      return false;
    }
  }
  // -----------------------------------------------------------------


  emit newMovixFileItems();
  emit newFileItems();

  return true;
}


bool K3bMovixDoc::saveDocumentData( QDomElement* docElem )
{
  QDomDocument doc = docElem->ownerDocument();

  saveGeneralDocumentData( docElem );

  QDomElement optionsElem = doc.createElement( "data_options" );
  saveDocumentDataOptions( optionsElem );

  QDomElement headerElem = doc.createElement( "data_header" );
  saveDocumentDataHeader( headerElem );

  QDomElement movixOptElem = doc.createElement( "movix_options" );
  QDomElement movixFilesElem = doc.createElement( "movix_files" );


  // save the movix options
  QDomElement propElem = doc.createElement( "shutdown" );
  propElem.setAttribute( "activated", shutdown() ? "yes" : "no" );
  movixOptElem.appendChild( propElem );

  propElem = doc.createElement( "reboot" );
  propElem.setAttribute( "activated", reboot() ? "yes" : "no" );
  movixOptElem.appendChild( propElem );

  propElem = doc.createElement( "eject_disk" );
  propElem.setAttribute( "activated", ejectDisk() ? "yes" : "no" );
  movixOptElem.appendChild( propElem );

  propElem = doc.createElement( "random_play" );
  propElem.setAttribute( "activated", randomPlay() ? "yes" : "no" );
  movixOptElem.appendChild( propElem );

  propElem = doc.createElement( "no_dma" );
  propElem.setAttribute( "activated", noDma() ? "yes" : "no" );
  movixOptElem.appendChild( propElem );

  propElem = doc.createElement( "subtitle_fontset" );
  propElem.appendChild( doc.createTextNode( subtitleFontset() ) );
  movixOptElem.appendChild( propElem );

  propElem = doc.createElement( "boot_message_language" );
  propElem.appendChild( doc.createTextNode( bootMessageLanguage() ) );
  movixOptElem.appendChild( propElem );

  propElem = doc.createElement( "default_boot_label" );
  propElem.appendChild( doc.createTextNode( defaultBootLabel() ) );
  movixOptElem.appendChild( propElem );

  propElem = doc.createElement( "additional_mplayer_options" );
  propElem.appendChild( doc.createTextNode( additionalMPlayerOptions() ) );
  movixOptElem.appendChild( propElem );

  propElem = doc.createElement( "unwanted_mplayer_options" );
  propElem.appendChild( doc.createTextNode( unwantedMPlayerOptions() ) );
  movixOptElem.appendChild( propElem );

  propElem = doc.createElement( "loop_playlist" );
  propElem.appendChild( doc.createTextNode( QString::number(loopPlaylist()) ) );
  movixOptElem.appendChild( propElem );


  // save the movix items
  for( QPtrListIterator<K3bMovixFileItem> it( m_movixFiles );
       *it; ++it ) {
    K3bMovixFileItem* item = *it;

    QDomElement topElem = doc.createElement( "file" );
    topElem.setAttribute( "name", item->k3bName() );
    QDomElement urlElem = doc.createElement( "url" );
    urlElem.appendChild( doc.createTextNode( item->localPath() ) );
    topElem.appendChild( urlElem );
    if( item->subTitleItem() ) {
      QDomElement subElem = doc.createElement( "subtitle_file" );
      urlElem = doc.createElement( "url" );
      urlElem.appendChild( doc.createTextNode( item->subTitleItem()->localPath() ) );
      subElem.appendChild( urlElem );
      topElem.appendChild( subElem );
    }

    movixFilesElem.appendChild( topElem );
  }

  docElem->appendChild( optionsElem );
  docElem->appendChild( headerElem );
  docElem->appendChild( movixOptElem );
  docElem->appendChild( movixFilesElem );

  return true;
}


void K3bMovixDoc::slotDataItemRemoved( K3bDataItem* item )
{
  // check if it's a movix item
  if( K3bMovixFileItem* fi = dynamic_cast<K3bMovixFileItem*>(item) )
    if( m_movixFiles.containsRef( fi ) ) {
      emit movixItemRemoved( fi );
      m_movixFiles.removeRef( fi );
      setModified(true);
    }
}


int K3bMovixDoc::indexOf( K3bMovixFileItem* item )
{
  return m_movixFiles.findRef(item)+1;
}


void K3bMovixDoc::moveMovixItem( K3bMovixFileItem* item, K3bMovixFileItem* itemAfter )
{
  if( item == itemAfter )
    return;

  // set the current item to track
  m_movixFiles.findRef( item );
  // take the current item
  item = m_movixFiles.take();

  // if after == 0 findRef returnes -1
  int pos = m_movixFiles.findRef( itemAfter );
  m_movixFiles.insert( pos+1, item );

  emit newMovixFileItems();

  setModified(true);
}


void K3bMovixDoc::addSubTitleItem( K3bMovixFileItem* item, const KURL& url )
{
  if( item->subTitleItem() )
    removeSubTitleItem( item );

  QFileInfo f( url.path() );
  if( !f.isFile() || !url.isLocalFile() )
    return;

  // check if there already is a file named like we want to name the subTitle file
  QString name = K3bMovixFileItem::subTitleFileName( item->k3bName() );

  if( nameAlreadyInDir( name, root() ) ) {
    KMessageBox::error( 0, i18n("Could not rename subtitle file. File with requested name %1 already exists.").arg(name) );
    return;
  }

  K3bFileItem* subItem = new K3bFileItem( f.absFilePath(), this, root(), name );
  item->setSubTitleItem( subItem );

  emit newMovixFileItems();
  emit newFileItems();

  setModified(true);
}


void K3bMovixDoc::removeSubTitleItem( K3bMovixFileItem* item )
{
  if( item->subTitleItem() ) {
    emit subTitleItemRemoved( item );

    delete item->subTitleItem();
    item->setSubTitleItem(0);

    setModified(true);
  }
}


void K3bMovixDoc::loadDefaultSettings( KConfig* c )
{
  K3bDataDoc::loadDefaultSettings(c);

  setSubtitleFontset( c->readEntry("subtitle_fontset") );

  setLoopPlaylist( c->readNumEntry("loop", 1 ) );
  setAdditionalMPlayerOptions( c->readEntry( "additional_mplayer_options" ) );
  setUnwantedMPlayerOptions( c->readEntry( "unwanted_mplayer_options" ) );

  setBootMessageLanguage( c->readEntry("boot_message_language") );

  setDefaultBootLabel( c->readEntry( "default_boot_label" ) );

  setShutdown( c->readBoolEntry( "shutdown", false) );
  setReboot( c->readBoolEntry( "reboot", false ) );
  setEjectDisk( c->readBoolEntry( "eject", false ) );
  setRandomPlay( c->readBoolEntry( "random_play", false ) );
  setNoDma( c->readBoolEntry( "no_dma", false ) );
}

#include "k3bmovixdoc.moc"
