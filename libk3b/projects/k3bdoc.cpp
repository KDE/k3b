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


// include files for Qt
#include <qwidget.h>
#include <qstring.h>
#include <qdom.h>

// include files for KDE
#include <klocale.h>
#include <kdebug.h>

// application specific includes
#include "k3bdoc.h"
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


K3bDoc::K3bDoc( QObject* parent )
  : QObject( parent ),
    m_modified(false),
    m_view(0)
{
  connect( this, SIGNAL(changed()), this, SLOT(slotChanged()) );
}


K3bDoc::~K3bDoc()
{
}


void K3bDoc::slotChanged()
{
  setModified( true );
  emit changed( this );
}


void K3bDoc::setModified( bool m )
{
  if( m != m_modified ) {
    m_modified = m;
    if( m )    
      emit changed();
  }
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


void K3bDoc::setURL( const KURL& url )
{
  doc_url = url;

  emit changed();
}

const KURL& K3bDoc::URL() const
{
  return doc_url;
}


QString K3bDoc::name() const
{
  return URL().path().section( '/', -1 );
}


bool K3bDoc::newDocument()
{
  setModified( false );

  m_copies = 1;
  m_burner = 0;
  m_onTheFly = true;
  m_speed = 0;  // Auto
  m_onlyCreateImages = false;
  m_removeImages = true;
  m_dummy = false;
  m_writingApp = K3b::DEFAULT;
  m_writingMode = K3b::WRITING_MODE_AUTO;
  m_saved = false;

  return true;
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


#include "k3bdoc.moc"
