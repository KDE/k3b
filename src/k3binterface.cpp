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


#include "k3binterface.h"
#include "k3bprojectinterface.h"
#include "k3bprojectmanager.h"
#include "k3bapplication.h"
#include "k3bdoc.h"
#include "k3bview.h"
#include "k3bcore.h"
#include "k3b.h"

#include <k3bglobals.h>

#include <dcopclient.h>
#include <qptrlist.h>
#include <qtimer.h>



K3bInterface::K3bInterface()
  : DCOPObject( "K3bInterface" ),
    m_main( 0 )
{
}

DCOPRef K3bInterface::createDataCDProject()
{
  return DCOPRef( kapp->dcopClient()->appId(),
		  k3bappcore->projectManager()->dcopInterface( k3bappcore->projectManager()->createProject( K3bDoc::DATA ) )->objId() );
}

DCOPRef K3bInterface::createAudioCDProject()
{
  return DCOPRef( kapp->dcopClient()->appId(),
		  k3bappcore->projectManager()->dcopInterface( k3bappcore->projectManager()->createProject( K3bDoc::AUDIO ) )->objId() );
}

DCOPRef K3bInterface::createMixedCDProject()
{
  return DCOPRef( kapp->dcopClient()->appId(),
		  k3bappcore->projectManager()->dcopInterface( k3bappcore->projectManager()->createProject( K3bDoc::MIXED ) )->objId() );
}

DCOPRef K3bInterface::createVideoCDProject()
{
  return DCOPRef( kapp->dcopClient()->appId(),
		  k3bappcore->projectManager()->dcopInterface( k3bappcore->projectManager()->createProject( K3bDoc::VCD ) )->objId() );
}

DCOPRef K3bInterface::createMovixCDProject()
{
  return DCOPRef( kapp->dcopClient()->appId(),
		  k3bappcore->projectManager()->dcopInterface( k3bappcore->projectManager()->createProject( K3bDoc::MOVIX ) )->objId() );
}

DCOPRef K3bInterface::createDataDVDProject()
{
  return DCOPRef( kapp->dcopClient()->appId(),
		  k3bappcore->projectManager()->dcopInterface( k3bappcore->projectManager()->createProject( K3bDoc::DVD ) )->objId() );
}

DCOPRef K3bInterface::createVideoDVDProject()
{
  return DCOPRef( kapp->dcopClient()->appId(),
		  k3bappcore->projectManager()->dcopInterface( k3bappcore->projectManager()->createProject( K3bDoc::VIDEODVD ) )->objId() );
}

DCOPRef K3bInterface::createMovixDVDProject()
{
  return DCOPRef( kapp->dcopClient()->appId(),
		  k3bappcore->projectManager()->dcopInterface( k3bappcore->projectManager()->createProject( K3bDoc::MOVIX_DVD ) )->objId() );
}

DCOPRef K3bInterface::currentProject()
{
  K3bView* view = m_main->activeView();
  if( view )
    return DCOPRef( kapp->dcopClient()->appId(),
		    k3bappcore->projectManager()->dcopInterface( view->doc() )->objId() );
  else
    return DCOPRef();
}

DCOPRef K3bInterface::openProject( const KURL& url )
{
  K3bDoc* doc = k3bappcore->projectManager()->openProject( url );
  if( doc )
    return DCOPRef( kapp->dcopClient()->appId(),
		    k3bappcore->projectManager()->dcopInterface( doc )->objId() );
  else
    return DCOPRef();
}

QValueList<DCOPRef> K3bInterface::projects()
{
  QValueList<DCOPRef> lst;
  const QPtrList<K3bDoc>& docs = k3bappcore->projectManager()->projects();
  for( QPtrListIterator<K3bDoc> it( docs ); it.current(); ++it )
    lst.append( DCOPRef( kapp->dcopClient()->appId(), k3bappcore->projectManager()->dcopInterface( it.current() )->objId() ) );

  return lst;
}

void K3bInterface::addUrls( const KURL::List& urls )
{
  m_main->addUrls( urls );
}

void K3bInterface::addUrl( const KURL& url )
{
  KURL::List l;
  l.append(url);
  addUrls( l );
}


void K3bInterface::copyCd( const KURL& dev )
{
  m_main->cdCopy( K3b::urlToDevice( dev ) );
}


void K3bInterface::copyDvd( const KURL& dev )
{
  m_main->dvdCopy( K3b::urlToDevice( dev ) );
}


void K3bInterface::copyCd()
{
  // HACK since we want this method to return immediately
  QTimer::singleShot( 0, m_main, SLOT(slotCdCopy()) );
}


void K3bInterface::copyDvd()
{
  // HACK since we want this method to return immediately
  QTimer::singleShot( 0, m_main, SLOT(slotDvdCopy()) );
}


void K3bInterface::eraseCdrw()
{
  // HACK since we want this method to return immediately
  QTimer::singleShot( 0, m_main, SLOT(slotBlankCdrw()) );
}


void K3bInterface::formatDvd()
{
  // HACK since we want this method to return immediately
  QTimer::singleShot( 0, m_main, SLOT(slotFormatDvd()) );
}


void K3bInterface::burnCdImage( const KURL& url )
{
  m_main->slotWriteCdImage( url );
}


void K3bInterface::burnDvdImage( const KURL& url )
{
  m_main->slotWriteDvdIsoImage( url );
}


bool K3bInterface::blocked() const
{
  return k3bcore->jobsRunning();
}


void K3bInterface::cddaRip( const KURL& dev )
{
  m_main->cddaRip( K3b::urlToDevice( dev ) );
}
