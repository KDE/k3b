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


#include "k3binterface.h"
#include "k3bprojectinterface.h"
#include "k3bprojectmanager.h"
#include "k3bapplication.h"
#include "k3bdoc.h"
#include "k3bview.h"
#include "k3bcore.h"
#include "k3b.h"

#include <k3bglobals.h>

#include <q3ptrlist.h>
#include <qtimer.h>
//Added by qt3to4:
#include <QList>



K3b::Interface::Interface()
  : DCOPObject( "K3b::Interface" ),
    m_main( 0 )
{
}

DCOPRef K3b::Interface::createDataProject()
{
    return DCOPRef( kapp->dcopClient()->appId(),
                    k3bappcore->projectManager()->dcopInterface( k3bappcore->projectManager()->createProject( K3b::Doc::DATA ) )->objId() );
}

DCOPRef K3b::Interface::createDataCDProject()
{
    // backward compatibility
    return DCOPRef( kapp->dcopClient()->appId(),
                    k3bappcore->projectManager()->dcopInterface( k3bappcore->projectManager()->createProject( K3b::Doc::DATA ) )->objId() );
}

DCOPRef K3b::Interface::createAudioCDProject()
{
  return DCOPRef( kapp->dcopClient()->appId(),
		  k3bappcore->projectManager()->dcopInterface( k3bappcore->projectManager()->createProject( K3b::Doc::AUDIO ) )->objId() );
}

DCOPRef K3b::Interface::createMixedCDProject()
{
  return DCOPRef( kapp->dcopClient()->appId(),
		  k3bappcore->projectManager()->dcopInterface( k3bappcore->projectManager()->createProject( K3b::Doc::MIXED ) )->objId() );
}

DCOPRef K3b::Interface::createVideoCDProject()
{
  return DCOPRef( kapp->dcopClient()->appId(),
		  k3bappcore->projectManager()->dcopInterface( k3bappcore->projectManager()->createProject( K3b::Doc::VCD ) )->objId() );
}

DCOPRef K3b::Interface::createMovixProject()
{
    return DCOPRef( kapp->dcopClient()->appId(),
                    k3bappcore->projectManager()->dcopInterface( k3bappcore->projectManager()->createProject( K3b::Doc::MOVIX ) )->objId() );
}

DCOPRef K3b::Interface::createMovixCDProject()
{
    // backward compatibility
    return DCOPRef( kapp->dcopClient()->appId(),
                    k3bappcore->projectManager()->dcopInterface( k3bappcore->projectManager()->createProject( K3b::Doc::MOVIX ) )->objId() );
}

DCOPRef K3b::Interface::createDataDVDProject()
{
    // backward compatibility
    return DCOPRef( kapp->dcopClient()->appId(),
                    k3bappcore->projectManager()->dcopInterface( k3bappcore->projectManager()->createProject( K3b::Doc::DATA ) )->objId() );
}

DCOPRef K3b::Interface::createVideoDVDProject()
{
  return DCOPRef( kapp->dcopClient()->appId(),
		  k3bappcore->projectManager()->dcopInterface( k3bappcore->projectManager()->createProject( K3b::Doc::VIDEODVD ) )->objId() );
}

DCOPRef K3b::Interface::createMovixDVDProject()
{
        // backward compatibility
    return DCOPRef( kapp->dcopClient()->appId(),
                    k3bappcore->projectManager()->dcopInterface( k3bappcore->projectManager()->createProject( K3b::Doc::MOVIX ) )->objId() );
}

DCOPRef K3b::Interface::currentProject()
{
  K3b::View* view = m_main->activeView();
  if( view )
    return DCOPRef( kapp->dcopClient()->appId(),
		    k3bappcore->projectManager()->dcopInterface( view->doc() )->objId() );
  else
    return DCOPRef();
}

DCOPRef K3b::Interface::openProject( const KUrl& url )
{
  K3b::Doc* doc = k3bappcore->projectManager()->openProject( url );
  if( doc )
    return DCOPRef( kapp->dcopClient()->appId(),
		    k3bappcore->projectManager()->dcopInterface( doc )->objId() );
  else
    return DCOPRef();
}

QList<DCOPRef> K3b::Interface::projects()
{
  QList<DCOPRef> lst;
  const Q3PtrList<K3b::Doc>& docs = k3bappcore->projectManager()->projects();
  for( Q3PtrListIterator<K3b::Doc> it( docs ); it.current(); ++it )
    lst.append( DCOPRef( kapp->dcopClient()->appId(), k3bappcore->projectManager()->dcopInterface( it.current() )->objId() ) );

  return lst;
}

void K3b::Interface::addUrls( const KUrl::List& urls )
{
  m_main->addUrls( urls );
}

void K3b::Interface::addUrl( const KUrl& url )
{
  KUrl::List l;
  l.append(url);
  addUrls( l );
}


void K3b::Interface::copyCd( const KUrl& dev )
{
    // backward compatibility
    copyMedium( dev );
}


void K3b::Interface::copyDvd( const KUrl& dev )
{
    // backward compatibility
    copyMedium( dev );
}

void K3b::Interface::copyMedium( const KUrl& dev )
{
    m_main->mediaCopy( K3b::urlToDevice( dev ) );
}


void K3b::Interface::copyCd()
{
    // backward compatibility
    copyMedium();
}


void K3b::Interface::copyDvd()
{
    // backward compatibility
    copyMedium();
}


void K3b::Interface::copyMedium()
{
    // HACK since we want this method to return immediately
    QTimer::singleShot( 0, m_main, SLOT(slotMediaCopy()) );
}


void K3b::Interface::eraseCdrw()
{
    // backward compatibility
    formatMedium();
}


void K3b::Interface::formatDvd()
{
    // backward compatibility
    formatMedium();
}


void K3b::Interface::formatMedium()
{
    // HACK since we want this method to return immediately
    QTimer::singleShot( 0, m_main, SLOT(slotFormatMedium()) );
}


void K3b::Interface::burnCdImage( const KUrl& url )
{
  m_main->slotWriteCdImage( url );
}


void K3b::Interface::burnDvdImage( const KUrl& url )
{
  m_main->slotWriteDvdIsoImage( url );
}


bool K3b::Interface::blocked() const
{
  return k3bcore->jobsRunning();
}


void K3b::Interface::cddaRip( const KUrl& dev )
{
  m_main->cddaRip( K3b::urlToDevice( dev ) );
}
