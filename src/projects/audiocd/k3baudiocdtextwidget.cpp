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

#include "k3baudiocdtextwidget.h"

#include "k3baudiodoc.h"
#include "k3baudiotrack.h"
#include <k3bcdtextvalidator.h>

#include <qcheckbox.h>
#include <qtoolbutton.h>
#include <qptrlist.h>
#include <qlayout.h>

#include <klineedit.h>
#include <klocale.h>
#include <kdialog.h>
#include <kiconloader.h>


K3bAudioCdTextWidget::K3bAudioCdTextWidget( QWidget* parent, const char* name )
  : base_K3bAudioCdTextWidget( parent, name ),
    m_doc(0)
{
  layout()->setSpacing( KDialog::spacingHint() );
  layout()->setMargin( KDialog::marginHint() );

  m_buttonCopyTitle->setPixmap( SmallIcon( "copy" ) );
  m_buttonCopyPerformer->setPixmap( SmallIcon( "copy" ) );
  m_buttonCopySongwriter->setPixmap( SmallIcon( "copy" ) );
  m_buttonCopyComposer->setPixmap( SmallIcon( "copy" ) );
  m_buttonCopyArranger->setPixmap( SmallIcon( "copy" ) );

  QValidator* cdTextVal = new K3bCdTextValidator( this );
  m_editTitle->setValidator( cdTextVal );
  m_editPerformer->setValidator( cdTextVal );
  m_editDisc_id->setValidator( cdTextVal );
  m_editUpc_ean->setValidator( cdTextVal );
  m_editMessage->setValidator( cdTextVal );
  m_editArranger->setValidator( cdTextVal );
  m_editSongwriter->setValidator( cdTextVal );
  m_editComposer->setValidator( cdTextVal );
}


K3bAudioCdTextWidget::~K3bAudioCdTextWidget()
{
}

void K3bAudioCdTextWidget::load( K3bAudioDoc* doc )
{
  m_doc = doc;
  m_checkCdText->setChecked( doc->cdText() );

  m_editTitle->setText( doc->title() );
  m_editPerformer->setText( doc->artist() );
  m_editDisc_id->setText( doc->disc_id() );
  m_editUpc_ean->setText( doc->upc_ean() );
  m_editArranger->setText( doc->arranger() );
  m_editSongwriter->setText( doc->songwriter() );
  m_editComposer->setText( doc->composer() );
  m_editMessage->setText( doc->cdTextMessage() );
}

void K3bAudioCdTextWidget::save( K3bAudioDoc* doc )
{
  m_doc = doc;
  doc->writeCdText( m_checkCdText->isChecked() );

  doc->setTitle( m_editTitle->text() );
  doc->setArtist( m_editPerformer->text() );
  doc->setDisc_id( m_editDisc_id->text() );
  doc->setUpc_ean( m_editUpc_ean->text() );
  doc->setArranger( m_editArranger->text() );
  doc->setSongwriter( m_editSongwriter->text() );
  doc->setComposer( m_editComposer->text() );
  doc->setCdTextMessage( m_editMessage->text() );
}

void K3bAudioCdTextWidget::setChecked( bool b )
{
  m_checkCdText->setChecked( b );
}

bool K3bAudioCdTextWidget::isChecked() const
{
  return m_checkCdText->isChecked();
}


void K3bAudioCdTextWidget::slotCopyTitle()
{
  QPtrListIterator<K3bAudioTrack> it( *m_doc->tracks() );
  for( ; it.current(); ++it ) {
    K3bAudioTrack* track = it.current();
    track->setTitle( m_editTitle->text() );
  }
}

void K3bAudioCdTextWidget::slotCopyPerformer()
{
  QPtrListIterator<K3bAudioTrack> it( *m_doc->tracks() );
  for( ; it.current(); ++it ) {
    K3bAudioTrack* track = it.current();
    track->setPerformer( m_editPerformer->text() );
  }
}

void K3bAudioCdTextWidget::slotCopyArranger()
{
  QPtrListIterator<K3bAudioTrack> it( *m_doc->tracks() );
  for( ; it.current(); ++it ) {
    K3bAudioTrack* track = it.current();
    track->setArranger( m_editArranger->text() );
  }
}

void K3bAudioCdTextWidget::slotCopySongwriter()
{
  QPtrListIterator<K3bAudioTrack> it( *m_doc->tracks() );
  for( ; it.current(); ++it ) {
    K3bAudioTrack* track = it.current();
    track->setSongwriter( m_editSongwriter->text() );
  }
}

void K3bAudioCdTextWidget::slotCopyComposer()
{
  QPtrListIterator<K3bAudioTrack> it( *m_doc->tracks() );
  for( ; it.current(); ++it ) {
    K3bAudioTrack* track = it.current();
    track->setComposer( m_editComposer->text() );
  }
}


#include "k3baudiocdtextwidget.moc"
