/* 
 *
 * $Id$
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include <config.h>

#if HAVE_MUSICBRAINZ

#include "k3baudiotracktrmlookupdialog.h"
#include "k3btrm.h"

#include <k3baudiotrack.h>
#include <k3baudiofile.h>
#include <k3bbusywidget.h>

#include <kmessagebox.h>
#include <kinputdialog.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kglobal.h>
#include <kdebug.h>

#include <qlabel.h>
#include <qlayout.h>
#include <qtimer.h>
#include <qframe.h>


K3bAudioTrackTRMLookupDialog::K3bAudioTrackTRMLookupDialog( QWidget* parent, const char* name )
  : KDialog( parent, name, false, WStyle_Customize | WStyle_NoBorder )
{
  setCaption( i18n("MusicBrainz Query") );

  QHBoxLayout* lay = new QHBoxLayout( this );
  lay->setAutoAdd( true );

  QFrame* frame = new QFrame( this );
  frame->setFrameStyle( QFrame::Box|QFrame::Plain );
  frame->setLineWidth( 2 );

  QGridLayout* grid = new QGridLayout( frame );
  grid->setMargin( marginHint() );
  grid->setSpacing( spacingHint() );

  m_infoLabel = new QLabel( frame );
  QLabel* pixLabel = new QLabel( frame );
  pixLabel->setPixmap( KGlobal::iconLoader()->loadIcon( "musicbrainz", KIcon::NoGroup, 64 ) );
  pixLabel->setScaledContents( false );

  m_busyWidget = new K3bBusyWidget( frame );

  grid->addMultiCellWidget( pixLabel, 0, 1, 0, 0 );
  grid->addWidget( m_infoLabel, 0, 1 );
  grid->addWidget( m_busyWidget, 1, 1 );

  // TODO: add cancel button
}


K3bAudioTrackTRMLookupDialog::~K3bAudioTrackTRMLookupDialog()
{
}


void K3bAudioTrackTRMLookupDialog::lookup( const QPtrList<K3bAudioTrack>& tracks )
{
  m_tracks = tracks;

  m_busyWidget->showBusy(true);

  lookup( m_tracks.first() );
}


void K3bAudioTrackTRMLookupDialog::lookup( K3bAudioTrack* track )
{
  if( track ) {
    if( dynamic_cast<K3bAudioFile*>( track->firstSource() ) ) {
      m_infoLabel->setText( i18n("Querying MusicBrainz for track %1").arg(track->index()+1) );
  
      K3bTRMLookup* l = new K3bTRMLookup( static_cast<K3bAudioFile*>(track->firstSource())->filename() );
      connect( l, SIGNAL(lookupFinished(K3bTRMLookup*)), this, SLOT(slotLookupFinished(K3bTRMLookup*)) );
    }
    else
      lookup( m_tracks.next() );
  }
  else {
    m_busyWidget->showBusy(false);
    done(0);
  }
}


void K3bAudioTrackTRMLookupDialog::slotLookupFinished( K3bTRMLookup* l )
{
  KTRMResultList results = l->results();

  switch( l->resultState() ) {

  case K3bTRMLookup::RECOGNIZED:
    m_tracks.current()->setTitle( results.first().title() );
    m_tracks.current()->setArtist( results.first().artist() );
    break;

  case K3bTRMLookup::COLLISION: {
    // ask the user which one to use
    QStringList resultStrings, resultStringsUnique;
    for( KTRMResultList::iterator it = results.begin();
	 it != results.end(); ++it )
      resultStrings.append( (*it).artist() + " - " + (*it).title() );

    // since we are only using the title and the artist a lot of entries are alike to us
    // so to not let the user have to choose between two equal entries we trim the list down
    for( QStringList::const_iterator it = resultStrings.begin();
	 it != resultStrings.end(); ++it )
      if( resultStringsUnique.find( *it ) == resultStringsUnique.end() )
	resultStringsUnique.append( *it );

    QString s;
    bool ok = true;
    if( resultStringsUnique.count() > 1 )
      s = KInputDialog::getItem( i18n("MusicBrainz Query"),
				 i18n("Found multiple matches for track %1 (%2). Please select one.")
				 .arg(m_tracks.current()->index()+1)
				 .arg(static_cast<K3bAudioFile*>(m_tracks.current()->firstSource())->filename().section( '/', -1 )),
				 resultStringsUnique,
				 0,
				 false,
				 &ok,
				 this );
    else
      s = resultStringsUnique.first();

    if( ok ) {
      int i = resultStrings.findIndex( s );
      m_tracks.current()->setTitle( results[i].title() );
      m_tracks.current()->setArtist( results[i].artist() );
    }
    break;
  }

  case K3bTRMLookup::UNRECOGNIZED:
    KMessageBox::error( this, i18n("Track %1 was not found in the MusicBrainz database.")
			.arg( m_tracks.current()->index()+1) );
    break;

  case K3bTRMLookup::ERROR:
    KMessageBox::error( this, i18n("Failed to query track %1. Be aware that tag guessing with MusicBrainz "
				   "does not work with all audio formats supported by K3b.")
			.arg(m_tracks.current()->index()+1) );
    break;
  }

  delete l;

  lookup( m_tracks.next() );
}

#include "k3baudiotracktrmlookupdialog.moc"

#endif
