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

#include "k3baudioprojectcddbplugin.h"

// the k3b stuff we need
#include <k3bcore.h>
#include <k3bglobals.h>
#include <k3baudiodoc.h>
#include <k3baudiotrack.h>
#include <k3btoc.h>
#include <k3btrack.h>
#include <k3bmsf.h>
#include <k3bcddb.h>
#include <k3bcddbresult.h>
#include <k3bcddbquery.h>
#include <k3bprojectmanager.h>
#include <k3bprogressdialog.h>

#include <kdebug.h>
#include <kaction.h>
#include <kinstance.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kconfig.h>
#include <kgenericfactory.h>
#include <kapplication.h>

#include <qstring.h>


K3bAudioProjectCddbPlugin::K3bAudioProjectCddbPlugin( QObject* parent, 
						      const char* name,
						      const QStringList& )
  : KParts::Plugin( parent, name ),
    m_cddb(0),
    m_progress(0)
{
  KAction* a = new KAction( i18n("&Query Cddb for Audio project"),
			    0, 0,
			    this, SLOT(slotQuery()),
			    actionCollection(), "audio_project_cddb_plugin" );
  a->setToolTip( i18n("Query a cddb entry for the current audio project.") );
}


K3bAudioProjectCddbPlugin::~K3bAudioProjectCddbPlugin()
{
  delete m_progress;
}


void K3bAudioProjectCddbPlugin::slotQuery()
{
  m_doc = dynamic_cast<K3bAudioDoc*>( K3bProjectManager::instance()->activeDoc() );

  if( !m_doc ) {
    KMessageBox::sorry( 0, i18n("Please select an audio project for a cddb query.") );
  }
  else {
    if( !m_cddb ) {
      m_cddb = new K3bCddb( this );
      connect( m_cddb, SIGNAL(queryFinished(int)),
	       this, SLOT(slotCddbQueryFinished(int)) );
    }
    if( !m_progress )
      m_progress = new K3bProgressDialog( i18n("Query Cddb"), kapp->mainWidget(), i18n("Audio Project") );

    // read the k3b config :)
    KConfig* c = k3bcore->config();
    c->setGroup("Cddb");
    m_cddb->readConfig( c );

    // start the query
    m_cddb->query( m_doc->toToc() );

    m_progress->exec(false);
  }
}


void K3bAudioProjectCddbPlugin::slotCddbQueryFinished( int error )
{
  m_progress->hide();

  if( error == K3bCddbQuery::SUCCESS ) {
    K3bCddbResultEntry cddbInfo = m_cddb->result();

    // save the entry locally
    KConfig* c = k3bcore->config();
    c->setGroup( "Cddb" );
    if( c->readBoolEntry( "save cddb entries locally", true ) )
      m_cddb->saveEntry( cddbInfo );

    // save the entry to the doc
    m_doc->setTitle( cddbInfo.cdTitle );
    m_doc->setPerformer( cddbInfo.cdArtist );
    m_doc->setCdTextMessage( cddbInfo.cdExtInfo );

    int i = 0;
    for( K3bAudioTrack* track = m_doc->firstTrack(); track; track = track->next() ) {
      track->setTitle( cddbInfo.titles[i] );
      track->setPerformer( cddbInfo.artists[i] );
      track->setCdTextMessage( cddbInfo.extInfos[i] );

      ++i;
    }
  }
  else if( error == K3bCddbQuery::NO_ENTRY_FOUND ) {
    KMessageBox::information( kapp->mainWidget(), i18n("No CDDB entry found."), i18n("CDDB") );
  }
  else {
    KMessageBox::information( kapp->mainWidget(), m_cddb->errorString(), i18n("Cddb error") );
  }
}


K_EXPORT_COMPONENT_FACTORY( libk3baudioprojectcddbplugin, KGenericFactory<K3bAudioProjectCddbPlugin> )

#include "k3baudioprojectcddbplugin.moc"
