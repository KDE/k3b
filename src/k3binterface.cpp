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


#include "k3binterface.h"
#include "k3bprojectinterface.h"
#include "k3b.h"
#include "k3bdoc.h"

#include <dcopclient.h>
#include <qptrlist.h>



K3bInterface::K3bInterface( K3bMainWindow* w )
  : DCOPObject( "K3bInterface" ),
    m_main( w )
{
}

DCOPRef K3bInterface::createDataCDProject()
{
  return DCOPRef( kapp->dcopClient()->appId(),
		  m_main->dcopInterface( m_main->slotNewDataDoc() )->objId() );
}

DCOPRef K3bInterface::createAudioCDProject()
{
  return DCOPRef( kapp->dcopClient()->appId(),
		  m_main->dcopInterface( m_main->slotNewAudioDoc() )->objId() );
}

DCOPRef K3bInterface::createMixedCDProject()
{
  return DCOPRef( kapp->dcopClient()->appId(),
		  m_main->dcopInterface( m_main->slotNewMixedDoc() )->objId() );
}

DCOPRef K3bInterface::createVideoCDProject()
{
  return DCOPRef( kapp->dcopClient()->appId(),
		  m_main->dcopInterface( m_main->slotNewVcdDoc() )->objId() );
}

DCOPRef K3bInterface::createMovixCDProject()
{
  return DCOPRef( kapp->dcopClient()->appId(),
		  m_main->dcopInterface( m_main->slotNewMovixDoc() )->objId() );
}

DCOPRef K3bInterface::createDataDVDProject()
{
  return DCOPRef( kapp->dcopClient()->appId(),
		  m_main->dcopInterface( m_main->slotNewDvdDoc() )->objId() );
}

DCOPRef K3bInterface::openDocument( const KURL& url )
{
  K3bDoc* doc = m_main->openDocument( url );
  if( doc )
    return DCOPRef( kapp->dcopClient()->appId(),
		    m_main->dcopInterface( doc )->objId() );
  else
    return DCOPRef();
}

QValueList<DCOPRef> K3bInterface::projects()
{
  QValueList<DCOPRef> lst;
  const QPtrList<K3bDoc>& docs = m_main->projects();
  for( QPtrListIterator<K3bDoc> it( docs ); it.current(); ++it )
    lst.append( DCOPRef( kapp->dcopClient()->appId(), m_main->dcopInterface( it.current() )->objId() ) );

  return lst;
}
