/***************************************************************************
                          k3baudioburndialog.cpp  -  description
                             -------------------
    begin                : Sun Apr 1 2001
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

#include "k3baudioburndialog.h"
#include "../k3b.h"
#include "k3baudiodoc.h"
#include "../device/k3bdevice.h"

#include <qcheckbox.h>
#include <qcombobox.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qmultilineedit.h>
#include <qpushbutton.h>
#include <qtabwidget.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qgrid.h>
#include <qtoolbutton.h>
#include <qlist.h>
#include <qstringlist.h>
#include <qpoint.h>

#include <klocale.h>
#include <kstddirs.h>
#include <kconfig.h>
#include <kmessagebox.h>


K3bAudioBurnDialog::K3bAudioBurnDialog(K3bAudioDoc* _doc, QWidget *parent, const char *name, bool modal )
  : K3bProjectBurnDialog( _doc, parent, name, modal )
{
  setupBurnTab( addPage( i18n("Burning") ) );
  setupCdTextTab( addPage( i18n("CD-Text") ) );
	
  readSettings();
}

K3bAudioBurnDialog::~K3bAudioBurnDialog(){
}


void K3bAudioBurnDialog::saveSettings()
{
  // save temp dir
  k3bMain()->config()->setGroup( "General Options" );
  k3bMain()->config()->writeEntry( "Temp Dir", tempDir() );
  k3bMain()->config()->sync();

  doc()->setDao( m_checkDao->isChecked() );
  doc()->setDummy( m_checkSimulate->isChecked() );
  doc()->setOnTheFly( m_checkOnTheFly->isChecked() );
  ((K3bAudioDoc*)doc())->setPadding( m_checkPadding->isChecked() );
  ((K3bAudioDoc*)doc())->writeCdText( m_checkCdText->isChecked() );
	
  // -- saving current speed --------------------------------------
  doc()->setSpeed( writerSpeed() );
	
  // -- saving current device --------------------------------------
  doc()->setBurner( writerDevice() );
	
  // -- save Cd-Text ------------------------------------------------
  ((K3bAudioDoc*)doc())->setTitle( m_editTitle->text() );
  ((K3bAudioDoc*)doc())->setArtist( m_editPerformer->text() );
  ((K3bAudioDoc*)doc())->setDisc_id( m_editDisc_id->text() );
  ((K3bAudioDoc*)doc())->setUpc_ean( m_editUpc_ean->text() );
  ((K3bAudioDoc*)doc())->setArranger( m_editArranger->text() );
  ((K3bAudioDoc*)doc())->setSongwriter( m_editSongwriter->text() );
}


void K3bAudioBurnDialog::readSettings()
{
  m_checkDao->setChecked( doc()->dao() );
  m_checkSimulate->setChecked( doc()->dummy() );
  m_checkPadding->setChecked( ((K3bAudioDoc*)doc())->padding() );
  m_checkCdText->setChecked( ((K3bAudioDoc*)doc())->cdText() );

  // read CD-Text ------------------------------------------------------------
  m_editTitle->setText( ((K3bAudioDoc*)doc())->title() );
  m_editPerformer->setText( ((K3bAudioDoc*)doc())->artist() );
  m_editDisc_id->setText( ((K3bAudioDoc*)doc())->disc_id() );
  m_editUpc_ean->setText( ((K3bAudioDoc*)doc())->upc_ean() );
  m_editArranger->setText( ((K3bAudioDoc*)doc())->arranger() );
  m_editSongwriter->setText( ((K3bAudioDoc*)doc())->songwriter() );
	
  K3bProjectBurnDialog::readSettings();
}


void K3bAudioBurnDialog::setupBurnTab( QFrame* frame )
{
  QGridLayout* frameLayout = new QGridLayout( frame );
  frameLayout->setSpacing( spacingHint() );
  frameLayout->setMargin( marginHint() );


  // ---- options group ------------------------------------------------
  QGroupBox* m_groupOptions = new QGroupBox( frame, "m_groupOptions" );
  m_groupOptions->setTitle( i18n( "Options" ) );
  m_groupOptions->setColumnLayout(0, Qt::Vertical );
  m_groupOptions->layout()->setSpacing( 0 );
  m_groupOptions->layout()->setMargin( 0 );
  QVBoxLayout* m_groupOptionsLayout = new QVBoxLayout( m_groupOptions->layout() );
  m_groupOptionsLayout->setAlignment( Qt::AlignTop );
  m_groupOptionsLayout->setSpacing( spacingHint() );
  m_groupOptionsLayout->setMargin( marginHint() );

  m_checkCdText = new QCheckBox( m_groupOptions, "m_checkCdText" );
  m_checkCdText->setText( i18n( "Write CD-Text" ) );

  m_checkDao = new QCheckBox( m_groupOptions, "m_checkDao" );
  m_checkDao->setText( i18n( "DiscAtOnce" ) );

  m_checkSimulate = new QCheckBox( m_groupOptions, "m_checkSimulate" );
  m_checkSimulate->setText( i18n( "Simulate Writing" ) );

  m_checkOnTheFly = new QCheckBox( m_groupOptions, "m_checkOnTheFly" );
  m_checkOnTheFly->setText( i18n( "Writing on the fly" ) );
  m_groupOptionsLayout->addWidget( m_checkOnTheFly );

  m_checkPadding = new QCheckBox( m_groupOptions, "m_checkPadding" );
  m_checkPadding->setText( i18n( "Use Padding" ) );

  m_groupOptionsLayout->addWidget( m_checkSimulate );
  m_groupOptionsLayout->addWidget( m_checkCdText );
  m_groupOptionsLayout->addWidget( m_checkPadding );
  m_groupOptionsLayout->addWidget( m_checkDao );
  // --------------------------------------------------- options group ---

  frameLayout->addWidget( tempDirBox( frame ), 1, 1 );
  frameLayout->addWidget( m_groupOptions, 1, 0 );
  frameLayout->addMultiCellWidget( writerBox( frame ), 0, 0, 0, 1 );

  frameLayout->setRowStretch( 1, 1 );
  frameLayout->setColStretch( 1, 1 );
}


void K3bAudioBurnDialog::setupCdTextTab( QFrame* frame )
{
  QHBoxLayout* frameLayout = new QHBoxLayout( frame );
  frameLayout->setSpacing( spacingHint() );
  frameLayout->setMargin( marginHint() );

  QFrame* _mainGroup = new QFrame( frame, "_mainGroup" );
  _mainGroup->setFrameShape( QFrame::Box );
  _mainGroup->setFrameShadow( QFrame::Sunken );
  QGridLayout* _mainGroupLayout = new QGridLayout( _mainGroup );
  _mainGroupLayout->setSpacing( spacingHint() );
  _mainGroupLayout->setMargin( marginHint() );

  QGridLayout* _layout3 = new QGridLayout;
  _layout3->setSpacing( spacingHint() );
  _layout3->setMargin( 0 );

  m_editDisc_id = new QLineEdit( _mainGroup, "m_editDisc_id" );
  QToolTip::add(  m_editDisc_id, i18n( "International Standard Recording Code" ) );

  _layout3->addWidget( m_editDisc_id, 2, 1 );

  QLabel* _labelDisc_id = new QLabel( _mainGroup, "_labelDisc_id" );
  _labelDisc_id->setText( i18n( "&Disc ID" ) );
  QToolTip::add(  _labelDisc_id, i18n( "" ) );

  _layout3->addWidget( _labelDisc_id, 2, 0 );

  m_editUpc_ean = new QLineEdit( _mainGroup, "m_editUpc_ean" );

  _layout3->addWidget( m_editUpc_ean, 1, 1 );

  m_editMessage = new QMultiLineEdit( _mainGroup, "m_editMessage" );
  m_editMessage->setWordWrap( QMultiLineEdit::WidgetWidth );

  _layout3->addWidget( m_editMessage, 0, 1 );

  QLabel* _labelMessage = new QLabel( _mainGroup, "_labelMessage" );
  _labelMessage->setText( i18n( "&Message" ) );
  _labelMessage->setAlignment( int( QLabel::AlignTop | QLabel::AlignLeft ) );

  _layout3->addWidget( _labelMessage, 0, 0 );

  QLabel* _labelUpc_ean = new QLabel( _mainGroup, "_labelUpc_ean" );
  _labelUpc_ean->setText( i18n( "&UPC EAN" ) );

  _layout3->addWidget( _labelUpc_ean, 1, 0 );

  _mainGroupLayout->addMultiCellLayout( _layout3, 0, 1, 2, 2 );
  QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );
  _mainGroupLayout->addMultiCell( spacer, 1, 2, 0, 0 );

  QFrame* _line1 = new QFrame( _mainGroup, "_line1" );
  _line1->setMargin( 0 );
  _line1->setFrameStyle( QFrame::VLine | QFrame::Sunken );

  _mainGroupLayout->addMultiCellWidget( _line1, 0, 2, 1, 1 );

  QGridLayout* _layout2 = new QGridLayout;
  _layout2->setSpacing( spacingHint() );
  _layout2->setMargin( 0 );

  m_editPerformer = new QLineEdit( _mainGroup, "m_editPerformer" );

  _layout2->addWidget( m_editPerformer, 1, 1 );

  QLabel* _labelArranger = new QLabel( _mainGroup, "_labelArranger" );
  _labelArranger->setText( i18n( "&Arranger" ) );

  _layout2->addWidget( _labelArranger, 2, 0 );

  m_editArranger = new QLineEdit( _mainGroup, "m_editArranger" );

  _layout2->addWidget( m_editArranger, 2, 1 );

  QLabel* _labelSongwriter = new QLabel( _mainGroup, "_labelSongwriter" );
  _labelSongwriter->setText( i18n( "&Songwriter" ) );

  _layout2->addWidget( _labelSongwriter, 3, 0 );

  QLabel* _labelPerformer = new QLabel( _mainGroup, "_labelPerformer" );
  _labelPerformer->setText( i18n( "&Performer" ) );

  _layout2->addWidget( _labelPerformer, 1, 0 );

  m_editTitle = new QLineEdit( _mainGroup, "m_editTitle" );
  QToolTip::add(  m_editTitle, i18n( "" ) );

  _layout2->addWidget( m_editTitle, 0, 1 );

  QLabel* _labelTitle = new QLabel( _mainGroup, "_labelTitle" );
  _labelTitle->setText( i18n( "&Title" ) );

  _layout2->addWidget( _labelTitle, 0, 0 );

  m_editSongwriter = new QLineEdit( _mainGroup, "m_editSongwriter" );

  _layout2->addWidget( m_editSongwriter, 3, 1 );

  _mainGroupLayout->addLayout( _layout2, 0, 0 );
  QSpacerItem* spacer_2 = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );
  _mainGroupLayout->addItem( spacer_2, 2, 2 );
  frameLayout->addWidget( _mainGroup );

  // buddies
  _labelDisc_id->setBuddy( m_editDisc_id );
  _labelMessage->setBuddy( m_editMessage );
  _labelUpc_ean->setBuddy( m_editUpc_ean );
  _labelArranger->setBuddy( m_editArranger );
  _labelSongwriter->setBuddy( m_editSongwriter );
  _labelPerformer->setBuddy( m_editPerformer );
  _labelTitle->setBuddy( m_editTitle );


  // tab order
  setTabOrder( m_editTitle, m_editPerformer );
  setTabOrder( m_editPerformer, m_editArranger);
  setTabOrder( m_editArranger, m_editSongwriter );
  setTabOrder( m_editSongwriter, m_editUpc_ean );
  setTabOrder( m_editUpc_ean, m_editDisc_id );
  setTabOrder( m_editDisc_id, m_editMessage );
}


void K3bAudioBurnDialog::slotUser1()
{
  // check if enough space in tempdir if not on-the-fly
  if( !m_checkOnTheFly->isChecked() && doc()->size()/1024 > freeTempSpace() )
    KMessageBox::sorry( this, "Not enough space in temp directory." );
  else
    K3bProjectBurnDialog::slotUser1();
}
