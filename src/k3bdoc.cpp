/***************************************************************************
                          k3bdoc.cpp  -  description
                             -------------------
    begin                : Mon Mar 26 15:30:59 CEST 2001
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

// include files for KDE
#include <klocale.h>
#include <kmessagebox.h>
#include <kfiledialog.h>
#include <kio/job.h>
#include <kio/netaccess.h>
#include <kprocess.h>
#include <kapp.h>
#include <kstddirs.h>
#include <klocale.h>

// application specific includes
#include "k3b.h"
#include "k3bview.h"
#include "k3bdoc.h"
#include "k3bglobals.h"
#include "device/k3bdevice.h"
#include "audio/k3baudiodoc.h"
#include "data/k3bdatadoc.h"


K3bDoc::K3bDoc( QObject* parent )
  : QObject( parent )
{
  pViewList = new QList<K3bView>;
  pViewList->setAutoDelete(false);

  m_burner = 0;
  m_dao = true;
  m_onTheFly = true;
  m_overburn = false;
  m_burnproof = true;
  m_error = K3b::NOT_STARTED;
  m_speed = 1;
}


K3bDoc::~K3bDoc()
{
  delete pViewList;
}

void K3bDoc::setDao( bool b )
{
  m_dao = b;
}

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
    qDebug( QString("(K3bDoc) Setting writer to %1 %2").arg( dev->genericDevice()).arg(dev->description()) );
    if( !dev->burnproof() )
      setBurnproof( false );
  }
}


void K3bDoc::addView(K3bView *view)
{
  pViewList->append(view);
  changedViewList();
}

void K3bDoc::removeView(K3bView *view)
{
  pViewList->remove(view);
  if(!pViewList->isEmpty())
    changedViewList();
}

void K3bDoc::changedViewList(){	
	
  K3bView *w;
  if( (int)pViewList->count() == 1 ) {
    w = pViewList->first();
    w->setCaption( URL().fileName() );
  }
  else{	
    int i;
    for( i = 1, w = pViewList->first(); w != 0; i++, w = pViewList->next() )
      w->setCaption( QString(URL().fileName()+":%1").arg(i) );
  }
}

bool K3bDoc::isLastView() {
  return ((int) pViewList->count() == 1);
}


void K3bDoc::updateAllViews(K3bView *sender)
{
  K3bView *w;
  for(w=pViewList->first(); w!=0; w=pViewList->next())
    {
      w->update(sender);
    }
}

void K3bDoc::setURL(const KURL &url)
{
  doc_url=url;
}

const KURL& K3bDoc::URL() const
{
  return doc_url;
}


bool K3bDoc::newDocument()
{
  m_dummy = false;
  m_dao = true;
  m_onTheFly = true;
  m_overburn = false;
  m_burnproof = true;
	
  modified=false;
  return true;
}

K3bDoc* K3bDoc::openDocument(const KURL& url )
{
  QString tmpfile;
  KIO::NetAccess::download( url, tmpfile );

  /////////////////////////////////////////////////
  QFile f( tmpfile );
  if ( !f.open( IO_ReadOnly ) )
    return 0;

  QDomDocument xmlDoc;
  if( !xmlDoc.setContent( &f ) ) {
    f.close();
    return 0;
  }

  f.close();

  /////////////////////////////////////////////////
  KIO::NetAccess::removeTempFile( tmpfile );


  // check the documents DOCTYPE
  K3bDoc* newDoc = 0;
  if( xmlDoc.doctype().name() == "k3b_audio_project" )
    newDoc = new K3bAudioDoc( k3bMain() );
  else if( xmlDoc.doctype().name() == "k3b_data_project" )
    newDoc = new K3bDataDoc( k3bMain() );
      
  // ---------
  // load the data into the document	
  if( newDoc != 0 ) {
    newDoc->newDocument();
    if( newDoc->loadDocumentData( &xmlDoc ) ) {
      newDoc->setURL( url );
      return newDoc;
    }
  }
  

  delete newDoc;
  return 0;
}

bool K3bDoc::saveDocument(const KURL& url )
{
  QFile f( url.path() );
  if ( !f.open( IO_WriteOnly ) )
    return false;
  
  QDomDocument xmlDoc( documentType() );
  bool success = saveDocumentData( &xmlDoc );
  
  if( success ) {
    QTextStream xmlStream( &f );
    xmlDoc.save( xmlStream, 0 );

    modified = false;
    doc_url = url;
  }

  f.close();

  return success;
}


bool K3bDoc::saveGeneralDocumentData( QDomElement* part )
{
  QDomDocument doc = part->ownerDocument();
  QDomElement mainElem = doc.createElement( "general" );

  QDomElement propElem = doc.createElement( "dao" );
  propElem.setAttribute( "activated", dao() ? "yes" : "no" );
  mainElem.appendChild( propElem );

  propElem = doc.createElement( "dummy" );
  propElem.setAttribute( "activated", dummy() ? "yes" : "no" );
  mainElem.appendChild( propElem );

  propElem = doc.createElement( "on_the_fly" );
  propElem.setAttribute( "activated", onTheFly() ? "yes" : "no" );
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

    if( e.nodeName() == "dao")
      setDao( e.attributeNode( "activated" ).value() == "yes" );

    if( e.nodeName() == "dummy")
      setDummy( e.attributeNode( "activated" ).value() == "yes" );

    if( e.nodeName() == "on_the_fly")
      setOnTheFly( e.attributeNode( "activated" ).value() == "yes" );
  }


  return true;
}


void K3bDoc::disable()
{
  K3bView* view = pViewList->first();
  while( view ) {
    view->setDisabled( true );
    view = pViewList->next();
  }
}


void K3bDoc::enable()
{
  K3bView* view = pViewList->first();
  while( view ) {
    view->setEnabled( true );
    view = pViewList->next();
  }
}


#include "k3bdoc.moc"
