/*
 *
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

#include "k3bmixeddoc.h"
#include "k3bmixedjob.h"

#include <k3bdatadoc.h>
#include <k3baudiodoc.h>
#include <k3bglobals.h>
#include <k3bmsf.h>

#include <qfileinfo.h>
#include <qdom.h>

#include <klocale.h>
#include <kconfig.h>
#include <kapplication.h>
#include <kmessagebox.h>



K3bMixedDoc::K3bMixedDoc( QObject* parent )
  : K3bDoc( parent )
{
  m_dataDoc = new K3bDataDoc( this );
  m_audioDoc = new K3bAudioDoc( this );

  connect( m_dataDoc, SIGNAL(changed()),
	   this, SIGNAL(changed()) );
  connect( m_audioDoc, SIGNAL(changed()),
	   this, SIGNAL(changed()) );
}


K3bMixedDoc::~K3bMixedDoc()
{
}


bool K3bMixedDoc::newDocument()
{
  m_dataDoc->newDocument();
  m_audioDoc->newDocument();

  return K3bDoc::newDocument();
}


void K3bMixedDoc::clear()
{
    m_dataDoc->clear();
    m_audioDoc->clear();
}


QString K3bMixedDoc::name() const
{
  return m_dataDoc->name();
}


void K3bMixedDoc::setURL( const KUrl& url )
{
  K3bDoc::setURL( url );
  m_audioDoc->setURL( url );
  m_dataDoc->setURL( url );
}


void K3bMixedDoc::setModified( bool m )
{
  m_audioDoc->setModified( m );
  m_dataDoc->setModified( m );
}


bool K3bMixedDoc::isModified() const
{
  return ( m_audioDoc->isModified() || m_dataDoc->isModified() );
}


KIO::filesize_t K3bMixedDoc::size() const
{
  return m_dataDoc->size() + m_audioDoc->size();
}

K3b::Msf K3bMixedDoc::length() const
{
  return m_dataDoc->length() + m_audioDoc->length();
}


int K3bMixedDoc::numOfTracks() const
{
  return m_audioDoc->numOfTracks() + 1;
}


K3bBurnJob* K3bMixedDoc::newBurnJob( K3bJobHandler* hdl, QObject* parent )
{
  return new K3bMixedJob( this, hdl, parent  );
}


void K3bMixedDoc::addUrls( const KUrl::List& urls )
{
  dataDoc()->addUrls( urls );
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
  for( int i = 0; i < optionList.count(); i++ ) {

    QDomElement e = optionList.item(i).toElement();
    if( e.isNull() )
      return false;

    if( e.nodeName() == "remove_buffer_files" )
      setRemoveImages( e.toElement().text() == "yes" );
    else if( e.nodeName() == "image_path" )
      setTempDir( e.toElement().text() );
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
  bufferFilesElem.appendChild( doc.createTextNode( removeImages() ? "yes" : "no" ) );
  mixedElem.appendChild( bufferFilesElem );

  QDomElement imagePathElem = doc.createElement( "image_path" );
  imagePathElem.appendChild( doc.createTextNode( tempDir() ) );
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


K3bDevice::Toc K3bMixedDoc::toToc( int dataMode, const K3b::Msf& dataTrackLength ) const
{
  // !inaccurate datatrack size!
  K3bDevice::Track dataTrack( 0, dataTrackLength > 0 ? dataTrackLength-1 : m_dataDoc->length()-1,
				K3bDevice::Track::DATA, dataMode );
  K3bDevice::Toc toc = audioDoc()->toToc();
  if( mixedType() == DATA_FIRST_TRACK ) {
    // fix the audio tracks' sectors
    for( K3bDevice::Toc::iterator it = toc.begin(); it != toc.end(); ++it ) {
      (*it).setLastSector( (*it).lastSector() + dataTrack.length() );
      (*it).setFirstSector( (*it).firstSector() + dataTrack.length() );
    }
    toc.insert( toc.begin(), dataTrack );
  }
  else {
    // fix the datatrack's sectors
    dataTrack.setLastSector( dataTrack.lastSector() + toc.back().lastSector()+1 );
    dataTrack.setFirstSector( toc.back().lastSector()+1 );
    toc.append( dataTrack );

    if( mixedType() == DATA_SECOND_SESSION ) {
      // fix the session numbers
      for( K3bDevice::Toc::iterator it = toc.begin(); it != toc.end(); ++it ) {
	if( (*it).type() == K3bDevice::Track::DATA )
	  (*it).setSession( 2 );
	else
	  (*it).setSession( 1 );
      }
    }
  }

  return toc;
}


int K3bMixedDoc::supportedMediaTypes() const
{
    return K3bDevice::MEDIA_WRITABLE_CD;
}

#include "k3bmixeddoc.moc"

