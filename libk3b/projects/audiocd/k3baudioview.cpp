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


#include "k3baudioview.h"
#include "k3baudiodoc.h"
#include "k3baudiotrackview.h"
#include "k3baudioburndialog.h"
#include "k3baudiotrackplayer.h"

#include <k3bfillstatusdisplay.h>
#include <k3bmsf.h>
#include <k3btoolbox.h>
#include <kactionclasses.h>

// QT-includes
#include <qlayout.h>
#include <qstring.h>
#include <qvbox.h>

// KDE-includes
#include <klocale.h>
#include <kapplication.h>
#include <kdebug.h>


K3bAudioView::K3bAudioView( K3bAudioDoc* pDoc, QWidget* parent, const char *name )
  : K3bView( pDoc, parent, name )
{
  m_doc = pDoc;
  m_player = new K3bAudioTrackPlayer( m_doc, this );

  QVBox* box = new QVBox( this );
  K3bToolBox* toolBox = new K3bToolBox( box );
  m_songlist = new K3bAudioTrackView( m_doc, box );
  setMainWidget( box );
  fillStatusDisplay()->showTime();

  toolBox->addButton( m_player->action( K3bAudioTrackPlayer::ACTION_PLAY ) );
  toolBox->addButton( m_player->action( K3bAudioTrackPlayer::ACTION_PAUSE ) );
  toolBox->addButton( m_player->action( K3bAudioTrackPlayer::ACTION_STOP ) );
  toolBox->addSpacing();
  toolBox->addButton( m_player->action( K3bAudioTrackPlayer::ACTION_PREV ) );
  toolBox->addButton( m_player->action( K3bAudioTrackPlayer::ACTION_NEXT ) );
  toolBox->addSpacing();
  toolBox->addWidget( ((KWidgetAction*)m_player->action( K3bAudioTrackPlayer::ACTION_SEEK ))->widget() );
  toolBox->addStretch();

  connect( m_player, SIGNAL(playingTrack(K3bAudioTrack*)), this, SLOT(slotPlayerPlayingTrack(K3bAudioTrack*)) );
  connect( m_player, SIGNAL(stopped()), this, SLOT(slotPlayerStopped()) );
}

K3bAudioView::~K3bAudioView(){
}


void K3bAudioView::slotPlayerPlayingTrack( K3bAudioTrack* track )
{
  // TODO: display icon in the list
}


void K3bAudioView::slotPlayerStopped()
{
  // FIXME
}

#include "k3baudioview.moc"
