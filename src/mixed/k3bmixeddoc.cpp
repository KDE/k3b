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

#include "k3bmixeddoc.h"
#include "k3bmixedjob.h"
#include "k3bmixedview.h"

#include <data/k3bdatadoc.h>
#include <audio/k3baudiodoc.h>
#include <tools/k3bglobals.h>

#include <qfileinfo.h>
#include <qdom.h>

#include <klocale.h>
#include <kconfig.h>
#include <kapplication.h>


K3bMixedDoc::K3bMixedDoc( QObject* parent )
  : K3bDoc( parent )
{
  m_dataDoc = new K3bDataDoc( this );
  m_audioDoc = new K3bAudioDoc( this );
}


K3bMixedDoc::~K3bMixedDoc()
{
}


bool K3bMixedDoc::newDocument()
{
  m_dataDoc->newDocument();
  m_dataDoc->isoOptions().setVolumeID( i18n("Project name", "Mixed") );
  m_audioDoc->newDocument();

  return K3bDoc::newDocument();
}


KIO::filesize_t K3bMixedDoc::size() const
{
  return m_dataDoc->size() + m_audioDoc->size();
}

K3b::Msf K3bMixedDoc::length() const
{
  return m_dataDoc->length() + m_audioDoc->length();
}


K3bView* K3bMixedDoc::newView( QWidget* parent )
{
  return new K3bMixedView( this, parent );
}


int K3bMixedDoc::numOfTracks() const
{
  return m_audioDoc->numOfTracks() + 1;
}


K3bBurnJob* K3bMixedDoc::newBurnJob()
{
  return new K3bMixedJob( this );
}


void K3bMixedDoc::addUrl( const KURL& url )
{
  KURL::List urls(url);
  addUrls(urls);
}


void K3bMixedDoc::addUrls( const KURL::List& urls )
{
  K3bMixedView* view = (K3bMixedView*)firstView();
  K3bDirItem* dir = view->currentDir();
  if( dir )
    dataDoc()->slotAddUrlsToDir( urls, dir );
  else
    audioDoc()->addUrls( urls );

  setModified( true );
}


bool K3bMixedDoc::loadDocumentData( QDomElement* rootElem )
{
  QDomNodeList nodes = rootElem->childNodes();

  if( nodes.length() < 4 )
    return false;

  if( nodes.item(0).nodeName() != "general" )
    return false;
  if( !readGeneralDocumentData( nodes.item(0).toElement() ) )
    return false;

  if( nodes.item(1).nodeName() != "audio" )
    return false;
  QDomElement audioElem = nodes.item(1).toElement();
  if( !m_audioDoc->loadDocumentData( &audioElem ) )
    return false;

  if( nodes.item(2).nodeName() != "data" )
    return false;
  QDomElement dataElem = nodes.item(2).toElement();
  if( !m_dataDoc->loadDocumentData( &dataElem ) )
    return false;

  if( nodes.item(3).nodeName() != "mixed" )
    return false;

  QDomNodeList optionList = nodes.item(3).childNodes();
  for( uint i = 0; i < optionList.count(); i++ ) {

    QDomElement e = optionList.item(i).toElement();
    if( e.isNull() )
      return false;

    if( e.nodeName() == "remove_buffer_files" )
      setRemoveBufferFiles( e.toElement().text() == "yes" );
    else if( e.nodeName() == "image_path" )
      setImagePath( e.toElement().text() );
    else if( e.nodeName() == "mixed_type" ) {
      QString mt = e.toElement().text();
      if( mt == "last_track" )
	setMixedType( DATA_LAST_TRACK );
      else if( mt == "second_session" )
	setMixedType( DATA_SECOND_SESSION );
      else
	setMixedType( DATA_FIRST_TRACK );
    }
  }

  return true;
}


bool K3bMixedDoc::saveDocumentData( QDomElement* docElem )
{
  QDomDocument doc = docElem->ownerDocument();
  saveGeneralDocumentData( docElem );

  QDomElement audioElem = doc.createElement( "audio" );
  m_audioDoc->saveDocumentData( &audioElem );
  docElem->appendChild( audioElem );

  QDomElement dataElem = doc.createElement( "data" );
  m_dataDoc->saveDocumentData( &dataElem );
  docElem->appendChild( dataElem );

  QDomElement mixedElem = doc.createElement( "mixed" );
  docElem->appendChild( mixedElem );

  QDomElement bufferFilesElem = doc.createElement( "remove_buffer_files" );
  bufferFilesElem.appendChild( doc.createTextNode( removeBufferFiles() ? "yes" : "no" ) );
  mixedElem.appendChild( bufferFilesElem );

  QDomElement imagePathElem = doc.createElement( "image_path" );
  imagePathElem.appendChild( doc.createTextNode( imagePath() ) );
  mixedElem.appendChild( imagePathElem );

  QDomElement mixedTypeElem = doc.createElement( "mixed_type" );
  switch( mixedType() ) {
  case DATA_FIRST_TRACK:
    mixedTypeElem.appendChild( doc.createTextNode( "first_track" ) );
    break;
  case DATA_LAST_TRACK:
    mixedTypeElem.appendChild( doc.createTextNode( "last_track" ) );
    break;
  case DATA_SECOND_SESSION:
    mixedTypeElem.appendChild( doc.createTextNode( "second_session" ) );
    break;
  }
  mixedElem.appendChild( mixedTypeElem );

  setModified( false );

  return true;
}

  
void K3bMixedDoc::loadDefaultSettings()
{
  KConfig* c = kapp->config();
  c->setGroup( "default mixed settings" );

  setDummy( c->readBoolEntry( "dummy_mode", false ) );
  setDao( c->readBoolEntry( "dao", true ) );
  setOnTheFly( c->readBoolEntry( "on_the_fly", true ) );
  setBurnproof( c->readBoolEntry( "burnproof", true ) );
  setRemoveBufferFiles( c->readBoolEntry( "remove_buffer_files", true ) );

  m_audioDoc->writeCdText( c->readBoolEntry( "cd_text", false ) );

  // load mixed type
  if( c->readEntry( "mixed_type" ) == "last_track" )
    m_mixedType = DATA_LAST_TRACK;
  else if( c->readEntry( "mixed_type" ) == "second_session" )
    m_mixedType = DATA_SECOND_SESSION;
  else
    m_mixedType = DATA_FIRST_TRACK;

  QString datamode = c->readEntry( "data_track_mode" );
  if( datamode == "mode1" )
    m_dataDoc->setDataMode( K3b::MODE1 );
  else if( datamode == "mode2" )
    m_dataDoc->setDataMode( K3b::MODE2 );
  else
    m_dataDoc->setDataMode( K3b::AUTO );

  K3bIsoOptions o = K3bIsoOptions::load( c );
  dataDoc()->isoOptions() = o;
}


void K3bMixedDoc::setImagePath( const QString& path )
{
  // check if it's a file and if so just take the dir
  QFileInfo info( path );
  if( info.isDir() )
    m_imagePath = path;
  else
    m_imagePath = info.dirPath(true);
}


#include "k3bmixeddoc.moc"

