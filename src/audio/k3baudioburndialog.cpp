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
#include "../k3bwriterselectionwidget.h"
#include "../k3btempdirselectionwidget.h"

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
#include <qptrlist.h>
#include <qstringlist.h>
#include <qpoint.h>
#include <qtabwidget.h>

#include <klocale.h>
#include <kstddirs.h>
#include <kconfig.h>
#include <kmessagebox.h>


K3bAudioBurnDialog::K3bAudioBurnDialog(K3bAudioDoc* _doc, QWidget *parent, const char *name, bool modal )
  : K3bProjectBurnDialog( _doc, parent, name, modal )
{
  QTabWidget* tab = new QTabWidget( k3bMainWidget() );
  QFrame* f1 = new QFrame( tab );
  QFrame* f2 = new QFrame( tab );

  setupBurnTab( f1 );
  setupCdTextTab( f2 );

  tab->addTab( f1, i18n("Burning") );
  tab->addTab( f2, i18n("CD-Text") );
	
  connect( m_checkOnTheFly, SIGNAL(toggled(bool)), m_tempDirSelectionWidget, SLOT(setDisabled(bool)) );
  connect( m_checkOnTheFly, SIGNAL(toggled(bool)), m_checkRemoveBufferFiles, SLOT(setDisabled(bool)) );
  connect( m_checkDao, SIGNAL(toggled(bool)), m_checkHideFirstTrack, SLOT(setEnabled(bool)) );
  connect( m_checkDao, SIGNAL(toggled(bool)), m_checkCdText, SLOT(setEnabled(bool)) );

  m_tempDirSelectionWidget->setNeededSize( doc()->size() );
  readSettings();
}

K3bAudioBurnDialog::~K3bAudioBurnDialog(){
}


void K3bAudioBurnDialog::saveSettings()
{
  doc()->setTempDir( m_tempDirSelectionWidget->tempPath() );
  doc()->setDao( m_checkDao->isChecked() );
  doc()->setDummy( m_checkSimulate->isChecked() );
  doc()->setOnTheFly( m_checkOnTheFly->isChecked() );
  ((K3bAudioDoc*)doc())->setPadding( m_checkPadding->isChecked() );
  ((K3bAudioDoc*)doc())->writeCdText( m_checkCdText->isChecked() );
  ((K3bAudioDoc*)doc())->setHideFirstTrack( m_checkHideFirstTrack->isChecked() );
  ((K3bAudioDoc*)doc())->setRemoveBufferFiles( m_checkRemoveBufferFiles->isChecked() );
	
  // -- saving current speed --------------------------------------
  doc()->setSpeed( m_writerSelectionWidget->writerSpeed() );
	
  // -- saving current device --------------------------------------
  doc()->setBurner( m_writerSelectionWidget->writerDevice() );
	
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
  m_checkOnTheFly->setChecked( doc()->onTheFly() );
  m_checkSimulate->setChecked( doc()->dummy() );
  m_checkPadding->setChecked( ((K3bAudioDoc*)doc())->padding() );
  m_checkCdText->setChecked( ((K3bAudioDoc*)doc())->cdText() );
  m_checkHideFirstTrack->setChecked( ((K3bAudioDoc*)doc())->hideFirstTrack() );
  m_checkRemoveBufferFiles->setChecked( ((K3bAudioDoc*)doc())->removeBufferFiles() );

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

  m_checkDao = new QCheckBox( m_groupOptions, "m_checkDao" );
  m_checkDao->setText( i18n( "DiscAtOnce" ) );

  m_checkSimulate = new QCheckBox( m_groupOptions, "m_checkSimulate" );
  m_checkSimulate->setText( i18n( "Simulate Writing" ) );

  m_checkOnTheFly = new QCheckBox( m_groupOptions, "m_checkOnTheFly" );
  m_checkOnTheFly->setText( i18n( "Writing on the fly" ) );

  m_checkPadding = new QCheckBox( m_groupOptions, "m_checkPadding" );
  m_checkPadding->setText( i18n( "Use Padding" ) );

  m_checkHideFirstTrack = new QCheckBox( m_groupOptions, "m_checkHideFirstTrack" );
  m_checkHideFirstTrack->setText( i18n( "Hide first track" ) );

  m_checkRemoveBufferFiles = new QCheckBox( m_groupOptions, "m_checkRemoveBufferFiles" );
  m_checkRemoveBufferFiles->setText( i18n("Remove temp files") );

  m_groupOptionsLayout->addWidget( m_checkSimulate );
  m_groupOptionsLayout->addWidget( m_checkOnTheFly );
  m_groupOptionsLayout->addWidget( m_checkRemoveBufferFiles );
  m_groupOptionsLayout->addWidget( m_checkPadding );
  m_groupOptionsLayout->addWidget( m_checkDao );
  m_groupOptionsLayout->addWidget( m_checkHideFirstTrack );
  // --------------------------------------------------- options group ---

  m_tempDirSelectionWidget = new K3bTempDirSelectionWidget( frame );
  m_writerSelectionWidget = new K3bWriterSelectionWidget( frame );

  frameLayout->addWidget( m_tempDirSelectionWidget, 1, 1 );
  frameLayout->addWidget( m_groupOptions, 1, 0 );
  frameLayout->addMultiCellWidget( m_writerSelectionWidget, 0, 0, 0, 1 );

  frameLayout->setRowStretch( 1, 1 );
  frameLayout->setColStretch( 1, 1 );
}


void K3bAudioBurnDialog::setupCdTextTab( QFrame* frame )
{
  QGridLayout* mainGrid = new QGridLayout( frame );
  mainGrid->setSpacing( spacingHint() );
  mainGrid->setMargin( marginHint() );


  m_checkCdText = new QCheckBox( i18n( "Write CD-Text" ), frame, "m_checkCdText" );

  QLabel* labelDisc_id = new QLabel( i18n( "&Disc ID" ), frame, "labelDisc_id" );
  QLabel* labelMessage = new QLabel( i18n( "&Message" ), frame, "labelMessage" );
  labelMessage->setAlignment( int( QLabel::AlignTop | QLabel::AlignLeft ) );
  QLabel* labelUpc_ean = new QLabel( i18n( "&UPC EAN" ), frame, "labelUpc_ean" );
  QLabel* labelArranger = new QLabel( i18n( "&Arranger" ), frame, "labelArranger" );
  QLabel* labelSongwriter = new QLabel( i18n( "&Songwriter" ), frame, "labelSongwriter" );
  QLabel* labelPerformer = new QLabel( i18n( "&Performer" ), frame, "labelPerformer" );
  QLabel* labelTitle = new QLabel( i18n( "&Title" ), frame, "labelTitle" );

  m_editDisc_id = new QLineEdit( frame, "m_editDisc_id" );
  QToolTip::add(  m_editDisc_id, i18n( "International Standard Recording Code" ) );
  m_editUpc_ean = new QLineEdit( frame, "m_editUpc_ean" );
  m_editMessage = new QMultiLineEdit( frame, "m_editMessage" );
  m_editMessage->setWordWrap( QMultiLineEdit::WidgetWidth );
  m_editPerformer = new QLineEdit( frame, "m_editPerformer" );
  m_editArranger = new QLineEdit( frame, "m_editArranger" );
  m_editTitle = new QLineEdit( frame, "m_editTitle" );
  m_editSongwriter = new QLineEdit( frame, "m_editSongwriter" );


  QFrame* line = new QFrame( frame );
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
  mainGrid->addWidget( labelUpc_ean, 7, 0 );
  mainGrid->addWidget( m_editUpc_ean, 7, 1 );
  mainGrid->addWidget( labelDisc_id, 8, 0 );
  mainGrid->addWidget( m_editDisc_id, 8, 1 );
  mainGrid->addWidget( labelMessage, 9, 0 );
  mainGrid->addWidget( m_editMessage, 9, 1 );

  mainGrid->addRowSpacing( 6, 20 );
  mainGrid->setRowStretch( 9, 1 );

  // buddies
  labelDisc_id->setBuddy( m_editDisc_id );
  labelMessage->setBuddy( m_editMessage );
  labelUpc_ean->setBuddy( m_editUpc_ean );
  labelArranger->setBuddy( m_editArranger );
  labelSongwriter->setBuddy( m_editSongwriter );
  labelPerformer->setBuddy( m_editPerformer );
  labelTitle->setBuddy( m_editTitle );


  // tab order
  setTabOrder( m_editTitle, m_editPerformer );
  setTabOrder( m_editPerformer, m_editArranger);
  setTabOrder( m_editArranger, m_editSongwriter );
  setTabOrder( m_editSongwriter, m_editUpc_ean );
  setTabOrder( m_editUpc_ean, m_editDisc_id );
  setTabOrder( m_editDisc_id, m_editMessage );
}


void K3bAudioBurnDialog::slotOk()
{
  // check if enough space in tempdir if not on-the-fly
  if( !m_checkOnTheFly->isChecked() && doc()->size()/1024 > m_tempDirSelectionWidget->freeTempSpace() )
    KMessageBox::sorry( this, "Not enough space in temp directory." );
  else
    K3bProjectBurnDialog::slotOk();
}


void K3bAudioBurnDialog::loadDefaults()
{
  m_checkSimulate->setChecked( false );
  m_checkDao->setChecked( true );
  m_checkOnTheFly->setChecked( true );
  //  m_checkBurnProof->setChecked( true );

  m_checkCdText->setChecked( true );
  m_checkPadding->setChecked( false );
  m_checkHideFirstTrack->setChecked( false );
  m_checkRemoveBufferFiles->setChecked( true );
}


void K3bAudioBurnDialog::loadUserDefaults()
{
  KConfig* c = k3bMain()->config();

  c->setGroup( "default audio settings" );

  m_checkSimulate->setChecked( c->readBoolEntry( "dummy_mode", false ) );
  m_checkDao->setChecked( c->readBoolEntry( "dao", true ) );
  m_checkOnTheFly->setChecked( c->readBoolEntry( "on_the_fly", true ) );
  //  m_checkBurnProof->setChecked( c->readBoolEntry( "burnproof", true ) );

  m_checkCdText->setChecked( c->readBoolEntry( "cd_text", true ) );
  m_checkPadding->setChecked( c->readBoolEntry( "padding", false ) );
  m_checkHideFirstTrack->setChecked( c->readBoolEntry( "hide_first_track", false ) );
  m_checkRemoveBufferFiles->setChecked( c->readBoolEntry( "remove_buffer_files", true ) );
}


void K3bAudioBurnDialog::saveUserDefaults()
{
  KConfig* c = k3bMain()->config();

  c->setGroup( "default audio settings" );

  c->writeEntry( "dummy_mode", m_checkSimulate->isChecked() );
  c->writeEntry( "dao", m_checkDao->isChecked() );
  c->writeEntry( "on_the_fly", m_checkOnTheFly->isChecked() );

  c->writeEntry( "cd_text", m_checkCdText->isChecked() );
  c->writeEntry( "padding", m_checkPadding->isChecked() );
  c->writeEntry( "hide_first_track", m_checkHideFirstTrack->isChecked() );
  c->writeEntry( "remove_buffer_files", m_checkRemoveBufferFiles->isChecked() );
}


#include "k3baudioburndialog.moc"
