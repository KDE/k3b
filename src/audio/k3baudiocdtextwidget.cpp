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

#include "k3baudiocdtextwidget.h"

#include <audio/k3baudiodoc.h>
#include <k3bstdguiitems.h>

#include <qlayout.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qframe.h>
#include <qtextedit.h>

#include <klocale.h>
#include <kdialog.h>


K3bAudioCdTextWidget::K3bAudioCdTextWidget( QWidget* parent, const char* name )
  : QWidget( parent, name )
{
  QGridLayout* mainGrid = new QGridLayout( this );
  mainGrid->setSpacing( KDialog::spacingHint() );
  mainGrid->setMargin( KDialog::marginHint() );

  m_checkCdText = K3bStdGuiItems::cdTextCheckbox(this);

  QLabel* labelDisc_id = new QLabel( i18n( "&Disk ID:" ), this, "labelDisc_id" );
  QLabel* labelMessage = new QLabel( i18n( "&Message:" ), this, "labelMessage" );
  QLabel* labelUpc_ean = new QLabel( i18n( "&UPC EAN:" ), this, "labelUpc_ean" );
  QLabel* labelArranger = new QLabel( i18n( "&Arranger:" ), this, "labelArranger" );
  QLabel* labelSongwriter = new QLabel( i18n( "&Songwriter:" ), this, "labelSongwriter" );
  QLabel* labelComposer = new QLabel( i18n( "&Composer:" ), this, "labelComposer" );
  QLabel* labelPerformer = new QLabel( i18n( "&Performer:" ), this, "labelPerformer" );
  QLabel* labelTitle = new QLabel( i18n( "&Title:" ), this, "labelTitle" );

  m_editDisc_id = new QLineEdit( this, "m_editDisc_id" );
  m_editUpc_ean = new QLineEdit( this, "m_editUpc_ean" );
  m_editMessage = new QTextEdit( this, "m_editMessage" );
  m_editPerformer = new QLineEdit( this, "m_editPerformer" );
  m_editArranger = new QLineEdit( this, "m_editArranger" );
  m_editTitle = new QLineEdit( this, "m_editTitle" );
  m_editSongwriter = new QLineEdit( this, "m_editSongwriter" );
  m_editComposer = new QLineEdit( this, "m_editComposer" );

  QFrame* line = new QFrame( this );
  line->setFrameStyle( QFrame::HLine | QFrame::Sunken );

  mainGrid->addMultiCellWidget( m_checkCdText, 0, 0, 0, 1 );
  mainGrid->addMultiCellWidget( line, 1, 1, 0, 1 );
  mainGrid->addWidget( labelTitle, 2, 0 );
  mainGrid->addWidget( m_editTitle, 2, 1 );
  mainGrid->addWidget( labelPerformer, 3, 0 );
  mainGrid->addWidget( m_editPerformer, 3, 1 );
  mainGrid->addWidget( labelArranger, 4, 0 );
  mainGrid->addWidget( m_editArranger, 4, 1 );
  mainGrid->addWidget( labelSongwriter, 5, 0 );
  mainGrid->addWidget( m_editSongwriter, 5, 1 );
  mainGrid->addWidget( labelComposer, 6, 0 );
  mainGrid->addWidget( m_editComposer, 6, 1 );
  mainGrid->addWidget( labelUpc_ean, 8, 0 );
  mainGrid->addWidget( m_editUpc_ean, 8, 1 );
  mainGrid->addWidget( labelDisc_id, 9, 0 );
  mainGrid->addWidget( m_editDisc_id, 9, 1 );
  mainGrid->addWidget( labelMessage, 10, 0 );
  mainGrid->addMultiCellWidget( m_editMessage, 10, 11, 1, 1 );

  mainGrid->addRowSpacing( 7, 20 );
  mainGrid->setRowStretch( 11, 1 );

  // buddies
  labelDisc_id->setBuddy( m_editDisc_id );
  labelMessage->setBuddy( m_editMessage );
  labelUpc_ean->setBuddy( m_editUpc_ean );
  labelArranger->setBuddy( m_editArranger );
  labelSongwriter->setBuddy( m_editSongwriter );
  labelComposer->setBuddy( m_editComposer );
  labelPerformer->setBuddy( m_editPerformer );
  labelTitle->setBuddy( m_editTitle );


  // tab order
  setTabOrder( m_editTitle, m_editPerformer );
  setTabOrder( m_editPerformer, m_editArranger);
  setTabOrder( m_editArranger, m_editSongwriter );
  setTabOrder( m_editSongwriter, m_editComposer );
  setTabOrder( m_editComposer, m_editUpc_ean );
  setTabOrder( m_editUpc_ean, m_editDisc_id );
  setTabOrder( m_editDisc_id, m_editMessage );

  QToolTip::add( m_editDisc_id, i18n( "International Standard Recording Code" ) );
  QToolTip::add( m_editUpc_ean, i18n("CD-TEXT information field") );
  QToolTip::add( m_editMessage, i18n("CD-TEXT information field") );
  QToolTip::add( m_editPerformer, i18n("CD-TEXT information field") );
  QToolTip::add( m_editArranger, i18n("CD-TEXT information field") );
  QToolTip::add( m_editTitle, i18n("CD-TEXT information field") );
  QToolTip::add( m_editSongwriter, i18n("CD-TEXT information field") );
  QToolTip::add( m_editComposer, i18n("CD-TEXT information field") );

//   QWhatsThis::add( m_editDisc_id, i18n("") );
//   QWhatsThis::add( m_editUpc_ean, i18n("") );
//   QWhatsThis::add( m_editMessage, i18n("") );
//   QWhatsThis::add( m_editPerformer, i18n("") );
//   QWhatsThis::add( m_editArranger, i18n("") );
//   QWhatsThis::add( m_editTitle, i18n("") );
//   QWhatsThis::add( m_editSongwriter, i18n("") );
}


K3bAudioCdTextWidget::~K3bAudioCdTextWidget()
{
}

void K3bAudioCdTextWidget::load( K3bAudioDoc* doc )
{
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

#include "k3baudiocdtextwidget.moc"
