/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *                    Klaus-Dieter Krannich <kd@k3b.org>
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


#include "k3bcdcopydialog.h"

#include "k3bdevicecombobox.h"
#include "k3bcdcopyjob.h"
#include "k3bclonejob.h"

#include <k3bwriterselectionwidget.h>
#include <k3btempdirselectionwidget.h>
#include <k3bcore.h>
#include <k3bstdguiitems.h>
#include <k3bdevice.h>
#include <k3bdevicemanager.h>
#include <k3bburnprogressdialog.h>
#include <k3bglobals.h>
#include <k3bexternalbinmanager.h>
#include <k3bthememanager.h>
#include <k3bwritingmodewidget.h>

#include <kguiitem.h>
#include <klocale.h>
#include <kstdguiitem.h>
#include <kstandarddirs.h>
#include <kmessagebox.h> 
#include <kconfig.h>
#include <kapplication.h>
#include <kiconloader.h>

#include <qcheckbox.h>
#include <qspinbox.h>
#include <qcombobox.h>
#include <qlayout.h>
#include <qgroupbox.h>
#include <qptrlist.h>
#include <qlabel.h>
#include <qtooltip.h>
#include <qtabwidget.h>
#include <qwhatsthis.h>
#include <qhbox.h>
#include <qpushbutton.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <qsizepolicy.h>
#include <qfile.h>
#include <qfileinfo.h>


K3bCdCopyDialog::K3bCdCopyDialog( QWidget *parent, const char *name, bool modal )
  : K3bInteractionDialog( parent, name, i18n("CD Copy"), i18n("and CD Cloning"),
			  START_BUTTON|CANCEL_BUTTON,
			  START_BUTTON,
			  "CD Copy",
			  modal )
{
  QWidget* main = mainWidget();

  QGridLayout* mainGrid = new QGridLayout( main );
  mainGrid->setSpacing( spacingHint() );
  mainGrid->setMargin( 0 );

  QGroupBox* groupSource = new QGroupBox( 1, Qt::Vertical, i18n("CD Reader Device"), main );
  groupSource->setInsideSpacing( spacingHint() );
  groupSource->setInsideMargin( marginHint() );
  m_comboSourceDevice = new K3bDeviceComboBox( groupSource );
  m_comboSourceDevice->addDevices( k3bcore->deviceManager()->cdReader() );

  m_writerSelectionWidget = new K3bWriterSelectionWidget( false, main );
  m_writerSelectionWidget->setSupportedWritingApps( K3b::CDRECORD );

  // tab widget --------------------
  QTabWidget* tabWidget = new QTabWidget( main );

  //
  // option tab --------------------
  //
  QWidget* optionTab = new QWidget( tabWidget );
  QGridLayout* optionTabGrid = new QGridLayout( optionTab );
  optionTabGrid->setSpacing( spacingHint() );
  optionTabGrid->setMargin( marginHint() );

  QGroupBox* groupCopyMode = new QGroupBox( 1, Qt::Vertical, i18n("Copy Mode"), optionTab );
  groupCopyMode->setInsideMargin( marginHint() );
  groupCopyMode->setInsideSpacing( spacingHint() );
  m_comboCopyMode = new QComboBox( groupCopyMode );
  m_comboCopyMode->insertItem( i18n("Normal Copy") );
  m_comboCopyMode->insertItem( i18n("Clone Copy") );
//   m_groupCopyMode->setExclusive( true );
//   m_radioNormalCopy = new QRadioButton( i18n("Normal copy"), m_groupCopyMode );
//   m_radioCloneCopy = new QRadioButton( i18n("Clone copy"), m_groupCopyMode );

  QGroupBox* groupWritingMode = new QGroupBox( 1, Qt::Vertical, i18n("Writing Mode"), optionTab );
  groupWritingMode->setInsideMargin( marginHint() );
  m_writingModeWidget = new K3bWritingModeWidget( groupWritingMode );

  QGroupBox* groupCopies = new QGroupBox( 2, Qt::Horizontal, i18n("Copies"), optionTab );
  groupCopies->setInsideSpacing( spacingHint() );
  groupCopies->setInsideMargin( marginHint() );
  QLabel* pixLabel = new QLabel( groupCopies );
  pixLabel->setPixmap( SmallIcon( "cdcopy", KIcon::SizeMedium ) );
  pixLabel->setScaledContents( false );
  m_spinCopies = new QSpinBox( 1, 99, 1, groupCopies );

  QGroupBox* groupOptions = new QGroupBox( 5, Qt::Vertical, i18n("Options"), optionTab );
  groupOptions->setInsideSpacing( spacingHint() );
  groupOptions->setInsideMargin( marginHint() );
  m_checkSimulate = K3bStdGuiItems::simulateCheckbox( groupOptions );
  m_checkOnTheFly = K3bStdGuiItems::onTheFlyCheckbox( groupOptions );
  m_checkOnlyCreateImage = K3bStdGuiItems::onlyCreateImagesCheckbox( groupOptions );
  m_checkDeleteImages = K3bStdGuiItems::removeImagesCheckbox( groupOptions );

  optionTabGrid->addWidget( groupCopyMode, 0, 0 );
  optionTabGrid->addWidget( groupWritingMode, 1, 0 );
  optionTabGrid->addMultiCellWidget( groupOptions, 0, 2, 1, 1 );
  optionTabGrid->addWidget( groupCopies, 2, 0 );
  optionTabGrid->setRowStretch( 2, 1 );
  optionTabGrid->setColStretch( 1, 1 );

  tabWidget->addTab( optionTab, i18n("&Options") );


  //
  // image tab ------------------
  //
  QWidget* imageTab = new QWidget( tabWidget );
  QGridLayout* imageTabGrid = new QGridLayout( imageTab );
  imageTabGrid->setSpacing( spacingHint() );
  imageTabGrid->setMargin( marginHint() );

  m_tempDirSelectionWidget = new K3bTempDirSelectionWidget( imageTab );

  imageTabGrid->addWidget( m_tempDirSelectionWidget, 0, 0 );

  tabWidget->addTab( imageTab, i18n("&Image") );


  //
  // advanced tab ------------------
  //
  QWidget* advancedTab = new QWidget( tabWidget );
  QGridLayout* advancedTabGrid = new QGridLayout( advancedTab );
  advancedTabGrid->setSpacing( spacingHint() );
  advancedTabGrid->setMargin( marginHint() );

  QGroupBox* groupGeneral = new QGroupBox( 2, Qt::Vertical, i18n("General"), advancedTab ); 
  groupGeneral->setInsideSpacing( spacingHint() );
  groupGeneral->setInsideMargin( marginHint() );
  QHBox* box = new QHBox( groupGeneral );
  box->setSpacing( spacingHint() );
  box->setStretchFactor( new QLabel( i18n("Read retries:"), box ), 1 );
  m_spinRetries = new QSpinBox( 1, 128, 1, box );

  m_checkIgnoreReadErrors = new QCheckBox( i18n("Ignore read errors"), groupGeneral );

  QGroupBox* groupData = new QGroupBox( 1, Qt::Vertical, i18n("Data"), advancedTab ); 
  groupData->setInsideSpacing( spacingHint() );
  groupData->setInsideMargin( marginHint() );
  m_checkNoCorrection = new QCheckBox( i18n("No error correction"), groupData );

  QGroupBox* groupAudio = new QGroupBox( 3, Qt::Vertical, i18n("Audio"), advancedTab ); 
  groupAudio->setInsideSpacing( spacingHint() );
  groupAudio->setInsideMargin( marginHint() );
  box = new QHBox( groupAudio );
  box->setSpacing( spacingHint() );
  box->setStretchFactor(new QLabel( i18n("Paranoia mode:"), box ), 1 );
  m_comboParanoiaMode = K3bStdGuiItems::paranoiaModeComboBox( box );
  m_checkReadCdText = new QCheckBox( i18n("Copy CD-Text"), groupAudio );
  m_checkPrefereCdText = new QCheckBox( i18n("Prefer CD-Text"), groupAudio );

  advancedTabGrid->addWidget( groupGeneral, 0, 0 );
  advancedTabGrid->addWidget( groupData, 1, 0 );
  advancedTabGrid->addMultiCellWidget( groupAudio, 0, 1, 1, 1 );  
  advancedTabGrid->setRowStretch( 1, 1 );

  tabWidget->addTab( advancedTab, i18n("&Advanced") );

  mainGrid->addWidget( groupSource, 0, 0  );
  mainGrid->addWidget( m_writerSelectionWidget, 1, 0  );
  mainGrid->addWidget( tabWidget, 2, 0 );
  mainGrid->setRowStretch( 2, 1 );


  connect( m_comboSourceDevice, SIGNAL(selectionChanged(K3bDevice::Device*)), this, SLOT(slotToggleAll()) );
  connect( m_writerSelectionWidget, SIGNAL(writerChanged()), this, SLOT(slotToggleAll()) );
  connect( m_writingModeWidget, SIGNAL(writingModeChanged(int)), this, SLOT(slotToggleAll()) );
  connect( m_checkOnTheFly, SIGNAL(toggled(bool)), this, SLOT(slotToggleAll()) );
  connect( m_checkSimulate, SIGNAL(toggled(bool)), this, SLOT(slotToggleAll()) );
  connect( m_checkOnlyCreateImage, SIGNAL(toggled(bool)), this, SLOT(slotToggleAll()) );
  connect( m_comboCopyMode, SIGNAL(activated(int)), this, SLOT(slotToggleAll()) );
  connect( m_checkReadCdText, SIGNAL(toggled(bool)), this, SLOT(slotToggleAll()) );


  QToolTip::add( m_checkIgnoreReadErrors, i18n("Skip unreadable sectors") );
  QToolTip::add( m_checkNoCorrection, i18n("Disable the source drive's error correction") );
  QToolTip::add( m_checkPrefereCdText, i18n("Use CD-Text instead of cddb if available.") );
  QToolTip::add( m_checkReadCdText, i18n("Copy CD-Text from the source CD if available.") );

  QWhatsThis::add( m_checkNoCorrection, i18n("<p>If this option is checked K3b will disable the "
					     "source drive's ECC/EDC error correction. This way sectors "
					     "that are unreadable by intention can be read."
					     "<p>This may be useful for cloning CDs with copy "
					     "protection based on corrupted sectors.") );
  QWhatsThis::add( m_checkReadCdText, i18n("<p>If this option is checked K3b will search for CD-Text on the source CD. "
					   "Disable it if your CD drive has problems with reading CD-Text or you want "
					   "to stick to Cddb info.") );
  QWhatsThis::add( m_checkPrefereCdText, i18n("<p>If this option is checked and K3b finds CD-Text on the source media it will be "
					      "copied to the resulting CD ignoring any potentially existing Cddb entries.") );
  QWhatsThis::add( m_checkIgnoreReadErrors, i18n("<p>If this option is checked and K3b is not able to read a sector from the "
						 "source CD/DVD it will be replaced with zeros on the resulting copy.") );


  QWhatsThis::add( m_comboCopyMode, 
		   "<p><b>" + i18n("Normal Copy") + "</b>"
		   + i18n("<p>This is the normal copy mode recommended for most CD types. "
			  "It allows copying Audio CDs, multi and single session Data CDs, and "
			  "Enhanced Audio CDs (an Audio CD containing an additional data session)."
			  "<p>For VideoCDs please use the CD Cloning mode.")
		   + "<p><b>" + i18n("Clone Copy") + "</b>"
		   + i18n("<p>In CD Cloning mode K3b performs a raw copy of the CD. That means it does "
			  "not care about the content but simply copies the CD bit by bit. It may be used "
			  "to copy VideoCDs or CDs which contain erroneous sectors."
			  "<p><b>Caution:</b> Only single session CDs can be cloned.") );
}


K3bCdCopyDialog::~K3bCdCopyDialog()
{
}


K3bDevice::Device* K3bCdCopyDialog::readingDevice() const
{
  return m_comboSourceDevice->selectedDevice();
}


void K3bCdCopyDialog::slotStartClicked()
{
  K3bJobProgressDialog* dlg = 0;
  if( m_checkOnlyCreateImage->isChecked() ) {
    dlg = new K3bJobProgressDialog( kapp->mainWidget() );
  }
  else {
    dlg = new K3bBurnProgressDialog( kapp->mainWidget() );
  }

  K3bBurnJob* burnJob = 0;

  if( m_comboCopyMode->currentItem() == 1 ) {

    //
    // check for m_tempDirSelectionWidget->tempPath() and
    // m_tempDirSelectionWidget-tempPath() + ".toc"
    //
    if( QFileInfo( m_tempDirSelectionWidget->tempPath() ).isFile() ) {
      if( KMessageBox::warningYesNo( this,
				     i18n("Do you want to overwrite %1?").arg(m_tempDirSelectionWidget->tempPath()),
				     i18n("File Exists") )
	  != KMessageBox::Yes )
	return;
    }
    
    if( QFileInfo( m_tempDirSelectionWidget->tempPath() + ".toc" ).isFile() ) {
      if( KMessageBox::warningYesNo( this,
				     i18n("Do you want to overwrite %1?").arg(m_tempDirSelectionWidget->tempPath() + ".toc"),
				     i18n("File Exists") )
	  != KMessageBox::Yes )
	return;
    }
    
    K3bCloneJob* job = new K3bCloneJob( dlg, this );

    job->setWriterDevice( m_writerSelectionWidget->writerDevice() );
    job->setReaderDevice( m_comboSourceDevice->selectedDevice() );
    job->setImagePath( m_tempDirSelectionWidget->tempPath() );
    job->setNoCorrection( m_checkNoCorrection->isChecked() );
    job->setRemoveImageFiles( m_checkDeleteImages->isChecked() );
    job->setOnlyCreateImage( m_checkOnlyCreateImage->isChecked() );
    job->setSimulate( m_checkSimulate->isChecked() );
    job->setWriteSpeed( m_writerSelectionWidget->writerSpeed() );
    job->setCopies( m_checkSimulate->isChecked() ? 1 : m_spinCopies->value() );
    job->setReadRetries( m_spinRetries->value() );

    burnJob = job;
  }
  else {
    K3bCdCopyJob* job = new K3bCdCopyJob( dlg, this );
    
    job->setWriterDevice( m_writerSelectionWidget->writerDevice() );
    job->setReaderDevice( m_comboSourceDevice->selectedDevice() );
    job->setSpeed( m_writerSelectionWidget->writerSpeed() );
    job->setSimulate( m_checkSimulate->isChecked() );
    job->setOnTheFly( m_checkOnTheFly->isChecked() );
    job->setKeepImage( !m_checkDeleteImages->isChecked() );
    job->setOnlyCreateImage( m_checkOnlyCreateImage->isChecked() );
    job->setTempPath( m_tempDirSelectionWidget->plainTempPath() );
    job->setCopies( m_checkSimulate->isChecked() ? 1 : m_spinCopies->value() );
    job->setParanoiaMode( m_comboParanoiaMode->currentText().toInt() );
    job->setReadRetries( m_spinRetries->value() );
    job->setCopyCdText( m_checkReadCdText->isChecked() );
    job->setPreferCdText( m_checkPrefereCdText->isChecked() );
    job->setIgnoreReadErrors( m_checkIgnoreReadErrors->isChecked() );
    job->setNoCorrection( m_checkNoCorrection->isChecked() );
    job->setWritingMode( m_writingModeWidget->writingMode() );

    burnJob = job;
  }

  
  hide();
  dlg->startJob( burnJob );
  delete dlg;
  delete burnJob;
}


void K3bCdCopyDialog::slotToggleAll()
{
  K3bDevice::Device* dev = m_writerSelectionWidget->writerDevice();

  m_checkSimulate->setEnabled( !m_checkOnlyCreateImage->isChecked() );
  m_checkDeleteImages->setEnabled( !m_checkOnlyCreateImage->isChecked() && !m_checkOnTheFly->isChecked() );
  m_spinCopies->setDisabled( m_checkSimulate->isChecked() || m_checkOnlyCreateImage->isChecked() );
  m_tempDirSelectionWidget->setDisabled( m_checkOnTheFly->isChecked() );
  m_checkOnlyCreateImage->setEnabled( !m_checkOnTheFly->isChecked() );
  m_writerSelectionWidget->setDisabled( m_checkOnlyCreateImage->isChecked() );
  m_checkOnTheFly->setEnabled( !m_checkOnlyCreateImage->isChecked() );

  if ( m_checkOnlyCreateImage->isChecked() )
    m_checkDeleteImages->setChecked( false );

   if( m_comboCopyMode->currentItem() == 1 ) {
     // cdrecord does not support cloning on-the-fly
     m_checkOnTheFly->setChecked(false);
     m_checkOnTheFly->setEnabled(false);

     m_writingModeWidget->setSupportedModes( K3b::RAW );
   }
   else {
     if( dev == m_comboSourceDevice->selectedDevice() ) {
       m_checkOnTheFly->setChecked( false );
       m_checkOnTheFly->setEnabled( false );
     }
     else
       m_checkOnTheFly->setEnabled( !m_checkOnlyCreateImage->isChecked() );

     m_writingModeWidget->setSupportedModes( K3b::TAO|K3b::DAO|K3b::RAW );
   }

   m_comboParanoiaMode->setDisabled( m_comboCopyMode->currentItem() == 1 );

   // no CD-TEXT in TAO mode
   m_checkReadCdText->setDisabled( m_writingModeWidget->writingMode() == K3b::TAO ||
				   m_comboCopyMode->currentItem() == 1 );
   m_checkPrefereCdText->setDisabled( !m_checkReadCdText->isChecked() || 
				      m_writingModeWidget->writingMode() == K3b::TAO ||
				      m_comboCopyMode->currentItem() == 1 );

   m_checkIgnoreReadErrors->setDisabled( m_comboCopyMode->currentItem() == 1 );

   //   m_checkNoCorrection->setEnabled( m_comboCopyMode->currentItem() == 1 );

   m_writingModeWidget->setEnabled( !m_checkOnlyCreateImage->isChecked() );

   m_buttonStart->setEnabled( dev || m_checkOnlyCreateImage->isChecked() );
}


void K3bCdCopyDialog::loadUserDefaults( KConfigBase* c )
{
  m_writerSelectionWidget->loadConfig( c );
  m_comboSourceDevice->setSelectedDevice( k3bcore->deviceManager()->findDevice( c->readEntry( "source_device" ) ) );
  m_writingModeWidget->loadConfig( c );
  m_checkSimulate->setChecked( c->readBoolEntry( "simulate", false ) );
  m_checkOnTheFly->setChecked( c->readBoolEntry( "on_the_fly", false ) );
  m_checkDeleteImages->setChecked( c->readBoolEntry( "delete_images", true ) );
  m_checkOnlyCreateImage->setChecked( c->readBoolEntry( "only_create_image", false ) );
  m_comboParanoiaMode->setCurrentItem( c->readNumEntry( "paranoia_mode", 0 ) );

  m_spinCopies->setValue( c->readNumEntry( "copies", 1 ) );

  m_tempDirSelectionWidget->setTempPath( K3b::defaultTempPath() );

  if( c->readEntry( "copy mode", "normal" ) == "normal" )
    m_comboCopyMode->setCurrentItem( 0 );
  else
    m_comboCopyMode->setCurrentItem( 1 );

  m_checkReadCdText->setChecked( c->readBoolEntry( "copy cdtext", true ) );
  m_checkPrefereCdText->setChecked( c->readBoolEntry( "prefer cdtext", false ) );
  m_checkIgnoreReadErrors->setChecked( c->readBoolEntry( "ignore read errors", false ) );
  m_checkNoCorrection->setChecked( c->readBoolEntry( "no correction", false ) );

  m_spinRetries->setValue( c->readNumEntry( "retries", 128 ) );

  slotToggleAll();
}

void K3bCdCopyDialog::saveUserDefaults( KConfigBase* c )
{
  m_writingModeWidget->saveConfig( c );
  c->writeEntry( "simulate", m_checkSimulate->isChecked() );
  c->writeEntry( "on_the_fly", m_checkOnTheFly->isChecked() );
  c->writeEntry( "delete_images", m_checkDeleteImages->isChecked() );
  c->writeEntry( "only_create_image", m_checkOnlyCreateImage->isChecked() );
  c->writeEntry( "paranoia_mode", m_comboParanoiaMode->currentText().toInt() );
  c->writeEntry( "copies", m_spinCopies->value() );

  m_writerSelectionWidget->saveConfig( c );

  if( m_tempDirSelectionWidget->isEnabled() )
    m_tempDirSelectionWidget->saveConfig();

  c->writeEntry( "source_device", m_comboSourceDevice->selectedDevice()->devicename() );

  c->writeEntry( "copy cdtext", m_checkReadCdText->isChecked() );
  c->writeEntry( "prefer cdtext", m_checkPrefereCdText->isChecked() );
  c->writeEntry( "ignore read errors", m_checkIgnoreReadErrors->isChecked() );
  c->writeEntry( "no correction", m_checkNoCorrection->isChecked() );
  c->writeEntry( "retries", m_spinRetries->value() );

  QString s;
  if( m_comboCopyMode->currentItem() == 1 )
    s = "clone";
  else
    s = "normal";
  c->writeEntry( "copy mode", s );
}

void K3bCdCopyDialog::loadK3bDefaults()
{
  m_writingModeWidget->setWritingMode( K3b::WRITING_MODE_AUTO );
  m_writerSelectionWidget->loadDefaults();
  m_checkSimulate->setChecked( false );
  m_checkOnTheFly->setChecked( false );
  m_checkDeleteImages->setChecked( true );
  m_checkOnlyCreateImage->setChecked( false );
  m_comboParanoiaMode->setCurrentItem(0);
  m_spinCopies->setValue(1);
  m_checkReadCdText->setChecked(true);
  m_checkPrefereCdText->setChecked(false);
  m_checkIgnoreReadErrors->setChecked(false);
  m_checkNoCorrection->setChecked(false);
  m_comboCopyMode->setCurrentItem( 0 ); // normal
  m_spinRetries->setValue(128);

  slotToggleAll();
}

#include "k3bcdcopydialog.moc"
