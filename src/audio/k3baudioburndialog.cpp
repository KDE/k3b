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
#include <kstandarddirs.h>
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
  tab->addTab( f2, i18n("CD-text") );

  // create advanced tab
  QWidget* advancedTab = new QWidget( tab );
  QGridLayout* advancedTabGrid = new QGridLayout( advancedTab );
  advancedTabGrid->setSpacing( spacingHint() );
  advancedTabGrid->setMargin( marginHint() );

  //  QGroupBox* advancedOptionGroup = new QGroupBox( 1, Qt::Vertical, i18n("Options"), advancedTab );
  QGroupBox* advancedGimmickGroup = new QGroupBox( 1, Qt::Vertical, i18n("Gimmicks"), advancedTab );

  //  m_checkPadding = new QCheckBox( i18n( "Use padding" ), advancedOptionGroup, "m_checkPadding" );
  m_checkHideFirstTrack = new QCheckBox( i18n( "Hide first track" ), advancedGimmickGroup, "m_checkHideFirstTrack" );

  //  advancedTabGrid->addWidget( advancedOptionGroup, 0, 0 );
  advancedTabGrid->addWidget( advancedGimmickGroup, 0, 0 );
  advancedTabGrid->setRowStretch( 1, 1 );

  tab->addTab( advancedTab, i18n("Advanced") );


	
  connect( m_checkOnTheFly, SIGNAL(toggled(bool)), m_tempDirSelectionWidget, SLOT(setDisabled(bool)) );
  connect( m_checkOnTheFly, SIGNAL(toggled(bool)), m_checkRemoveBufferFiles, SLOT(setDisabled(bool)) );
  connect( m_checkDao, SIGNAL(toggled(bool)), m_checkHideFirstTrack, SLOT(setEnabled(bool)) );
  connect( m_checkDao, SIGNAL(toggled(bool)), m_checkCdText, SLOT(setEnabled(bool)) );

  m_tempDirSelectionWidget->setNeededSize( doc()->size() );
  readSettings();


  // ToolTips
  // -------------------------------------------------------------------------
  QToolTip::add( m_checkCdText, i18n("Create CD-TEXT entries") );
  QToolTip::add( m_checkDao, i18n("Write in 'disk at once' mode") );
  QToolTip::add( m_checkOnTheFly, i18n("Decode audio files while writing") );
  QToolTip::add( m_checkSimulate, i18n("Only simulate the writing process") );
  QToolTip::add( m_checkHideFirstTrack, i18n("Hide the first track in the first pregap") );
  QToolTip::add( m_checkRemoveBufferFiles, i18n("Remove images from harddisk when finished") );
  QToolTip::add( m_editDisc_id, i18n("CD-TEXT information field") );
  QToolTip::add( m_editUpc_ean, i18n("CD-TEXT information field") );
  QToolTip::add( m_editMessage, i18n("CD-TEXT information field") );
  QToolTip::add( m_editPerformer, i18n("CD-TEXT information field") );
  QToolTip::add( m_editArranger, i18n("CD-TEXT information field") );
  QToolTip::add( m_editTitle, i18n("CD-TEXT information field") );
  QToolTip::add( m_editSongwriter, i18n("CD-TEXT information field") );
  QToolTip::add( m_editComposer, i18n("CD-TEXT information field") );

  // What's This info
  // -------------------------------------------------------------------------
  QWhatsThis::add( m_checkCdText, i18n("<p>If this option is checked K3b uses some otherwise unused space on the audio "
				       "CD to store additional information, like the artist or the CD title."
				       "<p>CD-TEXT is an extension to the audio CD standard introduced by Sony."
				       "<p>CD-TEXT will only be usable on CD players that support this extension "
				       "(mostly car CD players)."
				       "<p>Since a CD-TEXT enhanced CD will work in any CD player it is never a bad "
				       "idea to enable this (if you specified the data).") );
  QWhatsThis::add( m_checkDao, i18n("<p>If this option is checked K3b will write the CD in 'disk at once mode' as "
				    "compared to 'track at once' (TAO)."
				    "<p>It is always recommended to use DAO where possible."
				    "<p><b>Caution:</b> Track pregaps other than 2 seconds long are only supported "
				    "in DAO mode.") );
  QWhatsThis::add( m_checkOnTheFly, i18n("<p>If this option is checked K3b will decode the audio files in memory "
					 "while writing. This saves space on the harddisk and time."
					 "<p><b>Caution:</b> Make sure your system is able to decode the files fast enough "
					 "to avoid buffer underruns.")
					 + i18n("<p>It is recommended to try a simulation first.") );
  QWhatsThis::add( m_checkSimulate, i18n("<p>If this option is checked, K3b will perform all writing steps with the "
					 "laser turned off."
					 "<p>This is useful, for example, to test a higher writing speed "
					 "or if your system is able to write on-the-fly.") );
  QWhatsThis::add( m_checkHideFirstTrack, i18n("<p>If this option is checked K3b will <em>hide</em> the first track."
					       "<p>The audio CD standard uses pregaps before every track on the CD. "
					       "By default these last for 2 seconds and are silent. In DAO mode it "
					       "is possible to have longer pregaps that contain some audio. In this case "
					       "the first pregap will contain the complete first track."
					       "<p>You will need to seek back from the beginning of the CD to listen to "
					       "the first track. Try it, it's quite amusing!") );
  QWhatsThis::add( m_checkRemoveBufferFiles, i18n("<p>If this option is checked K3b will remove any created images after the "
						  "writing has finished."
						  "<p>Uncheck this if you want to keep the images.") );
  QWhatsThis::add( m_editDisc_id, i18n("") );
  QWhatsThis::add( m_editUpc_ean, i18n("") );
  QWhatsThis::add( m_editMessage, i18n("") );
  QWhatsThis::add( m_editPerformer, i18n("") );
  QWhatsThis::add( m_editArranger, i18n("") );
  QWhatsThis::add( m_editTitle, i18n("") );
  QWhatsThis::add( m_editSongwriter, i18n("") );
}

K3bAudioBurnDialog::~K3bAudioBurnDialog(){
}


void K3bAudioBurnDialog::saveSettings()
{
  doc()->setTempDir( m_tempDirSelectionWidget->tempPath() );
  doc()->setDao( m_checkDao->isChecked() );
  doc()->setDummy( m_checkSimulate->isChecked() );
  doc()->setOnTheFly( m_checkOnTheFly->isChecked() );
  //  ((K3bAudioDoc*)doc())->setPadding( m_checkPadding->isChecked() );
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
  ((K3bAudioDoc*)doc())->setComposer( m_editComposer->text() );
}


void K3bAudioBurnDialog::readSettings()
{
  m_checkDao->setChecked( doc()->dao() );
  m_checkOnTheFly->setChecked( doc()->onTheFly() );
  m_checkSimulate->setChecked( doc()->dummy() );
  //  m_checkPadding->setChecked( ((K3bAudioDoc*)doc())->padding() );
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
  m_editComposer->setText( ((K3bAudioDoc*)doc())->composer() );
	
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
  m_checkDao->setText( i18n( "Disk at once" ) );

  m_checkSimulate = new QCheckBox( m_groupOptions, "m_checkSimulate" );
  m_checkSimulate->setText( i18n( "Simulate Writing" ) );

  m_checkOnTheFly = new QCheckBox( m_groupOptions, "m_checkOnTheFly" );
  m_checkOnTheFly->setText( i18n( "Writing on the fly" ) );

  m_checkRemoveBufferFiles = new QCheckBox( m_groupOptions, "m_checkRemoveBufferFiles" );
  m_checkRemoveBufferFiles->setText( i18n("Remove image files") );

  m_groupOptionsLayout->addWidget( m_checkSimulate );
  m_groupOptionsLayout->addWidget( m_checkOnTheFly );
  m_groupOptionsLayout->addWidget( m_checkRemoveBufferFiles );
  m_groupOptionsLayout->addWidget( m_checkDao );
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


  m_checkCdText = new QCheckBox( i18n( "Write CD-text" ), frame, "m_checkCdText" );

  QLabel* labelDisc_id = new QLabel( i18n( "&Disk ID:" ), frame, "labelDisc_id" );
  QLabel* labelMessage = new QLabel( i18n( "&Message:" ), frame, "labelMessage" );
  QLabel* labelUpc_ean = new QLabel( i18n( "&UPC EAN:" ), frame, "labelUpc_ean" );
  QLabel* labelArranger = new QLabel( i18n( "&Arranger:" ), frame, "labelArranger" );
  QLabel* labelSongwriter = new QLabel( i18n( "&Songwriter:" ), frame, "labelSongwriter" );
  QLabel* labelComposer = new QLabel( i18n( "&Composer:" ), frame, "labelComposer" );
  QLabel* labelPerformer = new QLabel( i18n( "&Performer:" ), frame, "labelPerformer" );
  QLabel* labelTitle = new QLabel( i18n( "&Title:" ), frame, "labelTitle" );

  m_editDisc_id = new QLineEdit( frame, "m_editDisc_id" );
  QToolTip::add(  m_editDisc_id, i18n( "International Standard Recording Code" ) );
  m_editUpc_ean = new QLineEdit( frame, "m_editUpc_ean" );
  m_editMessage = new QLineEdit( frame, "m_editMessage" );
  m_editPerformer = new QLineEdit( frame, "m_editPerformer" );
  m_editArranger = new QLineEdit( frame, "m_editArranger" );
  m_editTitle = new QLineEdit( frame, "m_editTitle" );
  m_editSongwriter = new QLineEdit( frame, "m_editSongwriter" );
  m_editComposer = new QLineEdit( frame, "m_editComposer" );


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
  mainGrid->addWidget( labelComposer, 6, 0 );
  mainGrid->addWidget( m_editComposer, 6, 1 );
  mainGrid->addWidget( labelUpc_ean, 8, 0 );
  mainGrid->addWidget( m_editUpc_ean, 8, 1 );
  mainGrid->addWidget( labelDisc_id, 9, 0 );
  mainGrid->addWidget( m_editDisc_id, 9, 1 );
  mainGrid->addWidget( labelMessage, 10, 0 );
  mainGrid->addWidget( m_editMessage, 10, 1 );

  mainGrid->addRowSpacing( 7, 20 );
  mainGrid->setRowStretch( 10, 1 );

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
}


void K3bAudioBurnDialog::slotOk()
{
  // check if enough space in tempdir if not on-the-fly
  if( !m_checkOnTheFly->isChecked() && doc()->size()/1024 > m_tempDirSelectionWidget->freeTempSpace() )
    KMessageBox::sorry( this, i18n("Not enough space in temporary directory.") );
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
  //  m_checkPadding->setChecked( true );
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
  //  m_checkPadding->setChecked( c->readBoolEntry( "padding", false ) );
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
  //  c->writeEntry( "padding", m_checkPadding->isChecked() );
  c->writeEntry( "hide_first_track", m_checkHideFirstTrack->isChecked() );
  c->writeEntry( "remove_buffer_files", m_checkRemoveBufferFiles->isChecked() );
}


#include "k3baudioburndialog.moc"
