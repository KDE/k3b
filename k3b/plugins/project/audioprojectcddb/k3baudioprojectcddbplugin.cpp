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

#include <config.h>

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
#include <k3bprogressdialog.h>
#include <k3bpluginfactory.h>

#include <kdebug.h>
#include <kaction.h>
#include <kinstance.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kconfig.h>

#include <qstring.h>


K_EXPORT_COMPONENT_FACTORY( libk3baudioprojectcddbplugin, K3bPluginFactory<K3bAudioProjectCddbPlugin>( "libk3baudioprojectcddbplugin" ) )


K3bAudioProjectCddbPlugin::K3bAudioProjectCddbPlugin( QObject* parent, 
						      const char* name )
  : K3bProjectPlugin( AUDIO_CD, false, parent, name ),
    m_cddb(0),
    m_progress(0)
{
  setText( i18n("Query Cddb") );
  setToolTip( i18n("Query a cddb entry for the current audio project.") );
}


K3bAudioProjectCddbPlugin::~K3bAudioProjectCddbPlugin()
{
  delete m_progress;
}


void K3bAudioProjectCddbPlugin::activate( K3bDoc* doc, QWidget* parent )
{
  m_doc = dynamic_cast<K3bAudioDoc*>( doc );
  m_parentWidget = parent;
  m_canceled = false;

  if( !m_doc || m_doc->numOfTracks() == 0 ) {
    KMessageBox::sorry( parent, i18n("Please select a non-empty audio project for a cddb query.") );
  }
  else {
    if( !m_cddb ) {
      m_cddb = new K3bCddb( this );
      connect( m_cddb, SIGNAL(queryFinished(int)),
	       this, SLOT(slotCddbQueryFinished(int)) );
    }
    if( !m_progress ) {
      m_progress = new K3bProgressDialog( i18n("Query Cddb"), parent, i18n("Audio Project") );
      connect( m_progress, SIGNAL(cancelClicked()),
	       this, SLOT(slotCancelClicked()) );
    }

    // read the k3b config :)
    KConfig* c = k3bcore->config();
    c->setGroup("Cddb");
    m_cddb->readConfig( c );

    // start the query
    m_cddb->query( m_doc->toToc() );

    m_progress->exec(false);
  }
}


void K3bAudioProjectCddbPlugin::slotCancelClicked()
{
  m_canceled = true;
  m_progress->close();
}


void K3bAudioProjectCddbPlugin::slotCddbQueryFinished( int error )
{
  if( !m_canceled ) {
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

      // and enable cd-text
      m_doc->writeCdText( true );
    }
    else if( error == K3bCddbQuery::NO_ENTRY_FOUND ) {
      KMessageBox::information( m_parentWidget, i18n("No CDDB entry found."), i18n("CDDB") );
    }
    else if( error != K3bCddbQuery::CANCELED ) {
      KMessageBox::information( m_parentWidget, m_cddb->errorString(), i18n("Cddb error") );
    }
  }

  // make sure the progress dialog does not get deleted by it's parent
  delete m_progress;
  m_doc = 0;
  m_parentWidget = 0;
  m_progress = 0;
}

#include "k3baudioprojectcddbplugin.moc"
