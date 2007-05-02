/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *                    Klaus-Dieter Krannich <kd@k3b.org>
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


#include "k3bcdcopydialog.h"

#include "k3bmediaselectioncombobox.h"
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
#include <k3bapplication.h>
#include <k3bmediacache.h>

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

  QGroupBox* groupSource = new QGroupBox( 1, Qt::Vertical, i18n("Source Medium"), main );
  groupSource->setInsideSpacing( spacingHint() );
  groupSource->setInsideMargin( marginHint() );
  m_comboSourceDevice = new K3bMediaSelectionComboBox( groupSource );
  m_comboSourceDevice->setWantedMediumType( K3bDevice::MEDIA_WRITABLE_CD|K3bDevice::MEDIA_CD_ROM );
  m_comboSourceDevice->setWantedMediumState( K3bDevice::STATE_COMPLETE|K3bDevice::STATE_INCOMPLETE );

  m_writerSelectionWidget = new K3bWriterSelectionWidget( main );
  m_writerSelectionWidget->setWantedMediumType( K3bDevice::MEDIA_WRITABLE_CD );
  m_writerSelectionWidget->setWantedMediumState( K3bDevice::STATE_EMPTY );
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

  QGroupBox* groupWritingMode = new QGroupBox( 1, Qt::Vertical, i18n("Writing Mode"), optionTab );
  groupWritingMode->setInsideMargin( marginHint() );
  m_writingModeWidget = new K3bWritingModeWidget( groupWritingMode );

  QGroupBox* groupCopies = new QGroupBox( 2, Qt::Horizontal, i18n("Copies"), optionTab );
  groupCopies->setInsideSpacing( spacingHint() );
  groupCopies->setInsideMargin( marginHint() );
  QLabel* pixLabel = new QLabel( groupCopies );
  pixLabel->setPixmap( SmallIcon( "cdcopy", KIcon::SizeMedium ) );
  pixLabel->setScaledContents( false );
  m_spinCopies = new QSpinBox( 1, 999, 1, groupCopies );

  QGroupBox* groupOptions = new QGroupBox( 5, Qt::Vertical, i18n("Settings"), optionTab );
  groupOptions->setInsideSpacing( spacingHint() );
  groupOptions->setInsideMargin( marginHint() );
  m_checkSimulate = K3bStdGuiItems::simulateCheckbox( groupOptions );
  m_checkCacheImage = K3bStdGuiItems::createCacheImageCheckbox( groupOptions );
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

  m_groupAdvancedDataOptions = new QGroupBox( 3, Qt::Vertical, i18n("Data"), advancedTab, "data_options" ); 
  m_groupAdvancedDataOptions->setInsideSpacing( spacingHint() );
  m_groupAdvancedDataOptions->setInsideMargin( marginHint() );
  QHBox* box = new QHBox( m_groupAdvancedDataOptions );
  box->setSpacing( spacingHint() );
  box->setStretchFactor( new QLabel( i18n("Read retries:"), box ), 1 );
  m_spinDataRetries = new QSpinBox( 1, 128, 1, box );
  m_checkIgnoreDataReadErrors = K3bStdGuiItems::ignoreAudioReadErrorsCheckBox( m_groupAdvancedDataOptions );
  m_checkNoCorrection = new QCheckBox( i18n("No error correction"), m_groupAdvancedDataOptions );

  m_groupAdvancedAudioOptions = new QGroupBox( 5, Qt::Vertical, i18n("Audio"), advancedTab, "audio_options" ); 
  m_groupAdvancedAudioOptions->setInsideSpacing( spacingHint() );
  m_groupAdvancedAudioOptions->setInsideMargin( marginHint() );
  box = new QHBox( m_groupAdvancedAudioOptions );
  box->setSpacing( spacingHint() );
  box->setStretchFactor( new QLabel( i18n("Read retries:"), box ), 1 );
  m_spinAudioRetries = new QSpinBox( 1, 128, 1, box );
  m_checkIgnoreAudioReadErrors = K3bStdGuiItems::ignoreAudioReadErrorsCheckBox( m_groupAdvancedAudioOptions );
  box = new QHBox( m_groupAdvancedAudioOptions );
  box->setSpacing( spacingHint() );
  box->setStretchFactor(new QLabel( i18n("Paranoia mode:"), box ), 1 );
  m_comboParanoiaMode = K3bStdGuiItems::paranoiaModeComboBox( box );
  m_checkReadCdText = new QCheckBox( i18n("Copy CD-Text"), m_groupAdvancedAudioOptions );
  m_checkPrefereCdText = new QCheckBox( i18n("Prefer CD-Text"), m_groupAdvancedAudioOptions );

  advancedTabGrid->addWidget( m_groupAdvancedDataOptions, 0, 1 );
  advancedTabGrid->addWidget( m_groupAdvancedAudioOptions, 0, 0 );  

  tabWidget->addTab( advancedTab, i18n("&Advanced") );

  mainGrid->addWidget( groupSource, 0, 0  );
  mainGrid->addWidget( m_writerSelectionWidget, 1, 0  );
  mainGrid->addWidget( tabWidget, 2, 0 );
  mainGrid->setRowStretch( 2, 1 );


  connect( m_comboSourceDevice, SIGNAL(selectionChanged(K3bDevice::Device*)), this, SLOT(slotToggleAll()) );
  connect( m_comboSourceDevice, SIGNAL(selectionChanged(K3bDevice::Device*)), 
	   this, SLOT(slotSourceMediumChanged(K3bDevice::Device*)) );
  connect( m_writerSelectionWidget, SIGNAL(writerChanged()), this, SLOT(slotToggleAll()) );
  connect( m_writerSelectionWidget, SIGNAL(writerChanged(K3bDevice::Device*)), 
	   m_writingModeWidget, SLOT(setDevice(K3bDevice::Device*)) );
  connect( m_writingModeWidget, SIGNAL(writingModeChanged(int)), this, SLOT(slotToggleAll()) );
  connect( m_checkCacheImage, SIGNAL(toggled(bool)), this, SLOT(slotToggleAll()) );
  connect( m_checkSimulate, SIGNAL(toggled(bool)), this, SLOT(slotToggleAll()) );
  connect( m_checkOnlyCreateImage, SIGNAL(toggled(bool)), this, SLOT(slotToggleAll()) );
  connect( m_comboCopyMode, SIGNAL(activated(int)), this, SLOT(slotToggleAll()) );
  connect( m_checkReadCdText, SIGNAL(toggled(bool)), this, SLOT(slotToggleAll()) );

  QToolTip::add( m_checkIgnoreDataReadErrors, i18n("Skip unreadable data sectors") );
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
  QWhatsThis::add( m_checkIgnoreDataReadErrors, i18n("<p>If this option is checked and K3b is not able to read a data sector from the "
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


void K3bCdCopyDialog::init()
{
  slotSourceMediumChanged( m_comboSourceDevice->selectedDevice() );
}


void K3bCdCopyDialog::setReadingDevice( K3bDevice::Device* dev )
{
  m_comboSourceDevice->setSelectedDevice( dev );
}


K3bDevice::Device* K3bCdCopyDialog::readingDevice() const
{
  return m_comboSourceDevice->selectedDevice();
}


void K3bCdCopyDialog::slotStartClicked()
{
  //
  // Let's check the available size
  //
  if( m_checkCacheImage->isChecked() || m_checkOnlyCreateImage->isChecked() ) {
    if( neededSize()/1024 > m_tempDirSelectionWidget->freeTempSpace() ) {
      if( KMessageBox::warningContinueCancel( this, i18n("There seems to be not enough free space in temporary directory. "
							 "Write anyway?") ) == KMessageBox::Cancel )
	return;
    }
  }


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
      if( KMessageBox::warningContinueCancel( this,
				     i18n("Do you want to overwrite %1?").arg(m_tempDirSelectionWidget->tempPath()),
				     i18n("File Exists"), i18n("Overwrite") )
	  != KMessageBox::Continue )
	return;
    }
    
    if( QFileInfo( m_tempDirSelectionWidget->tempPath() + ".toc" ).isFile() ) {
      if( KMessageBox::warningContinueCancel( this,
				     i18n("Do you want to overwrite %1?").arg(m_tempDirSelectionWidget->tempPath() + ".toc"),
				     i18n("File Exists"), i18n("Overwrite") )
	  != KMessageBox::Continue )
	return;
    }
    
    K3bCloneJob* job = new K3bCloneJob( dlg, this );

    job->setWriterDevice( m_writerSelectionWidget->writerDevice() );
    job->setReaderDevice( m_comboSourceDevice->selectedDevice() );
    job->setImagePath( m_tempDirSelectionWidget->tempPath() );
    job->setNoCorrection( m_checkNoCorrection->isChecked() );
    job->setRemoveImageFiles( m_checkDeleteImages->isChecked() && !m_checkOnlyCreateImage->isChecked() );
    job->setOnlyCreateImage( m_checkOnlyCreateImage->isChecked() );
    job->setSimulate( m_checkSimulate->isChecked() );
    job->setWriteSpeed( m_writerSelectionWidget->writerSpeed() );
    job->setCopies( m_checkSimulate->isChecked() ? 1 : m_spinCopies->value() );
    job->setReadRetries( m_spinDataRetries->value() );

    burnJob = job;
  }
  else {
    K3bCdCopyJob* job = new K3bCdCopyJob( dlg, this );
    
    job->setWriterDevice( m_writerSelectionWidget->writerDevice() );
    job->setReaderDevice( m_comboSourceDevice->selectedDevice() );
    job->setSpeed( m_writerSelectionWidget->writerSpeed() );
    job->setSimulate( m_checkSimulate->isChecked() );
    job->setOnTheFly( !m_checkCacheImage->isChecked() );
    job->setKeepImage( !m_checkDeleteImages->isChecked() || m_checkOnlyCreateImage->isChecked() );
    job->setOnlyCreateImage( m_checkOnlyCreateImage->isChecked() );
    job->setTempPath( m_tempDirSelectionWidget->plainTempPath() );
    job->setCopies( m_checkSimulate->isChecked() ? 1 : m_spinCopies->value() );
    job->setParanoiaMode( m_comboParanoiaMode->currentText().toInt() );
    job->setDataReadRetries( m_spinDataRetries->value() );
    job->setAudioReadRetries( m_spinAudioRetries->value() );
    job->setCopyCdText( m_checkReadCdText->isChecked() );
    job->setPreferCdText( m_checkPrefereCdText->isChecked() );
    job->setIgnoreDataReadErrors( m_checkIgnoreDataReadErrors->isChecked() );
    job->setIgnoreAudioReadErrors( m_checkIgnoreAudioReadErrors->isChecked() );
    job->setNoCorrection( m_checkNoCorrection->isChecked() );
    job->setWritingMode( m_writingModeWidget->writingMode() );

    burnJob = job;
  }

  if( !exitLoopOnHide() )
    hide();

  dlg->startJob( burnJob );

  delete dlg;
  delete burnJob;

  if( KConfigGroup( k3bcore->config(), "General Options" ).readBoolEntry( "keep action dialogs open", false ) &&
      !exitLoopOnHide() )
    show();
  else
    close();
}


void K3bCdCopyDialog::slotToggleAll()
{
  updateOverrideDevice();

  K3bDevice::Device* dev = m_writerSelectionWidget->writerDevice();

  m_checkSimulate->setEnabled( !m_checkOnlyCreateImage->isChecked() );
  m_checkDeleteImages->setEnabled( !m_checkOnlyCreateImage->isChecked() && m_checkCacheImage->isChecked() );
  m_spinCopies->setDisabled( m_checkSimulate->isChecked() || m_checkOnlyCreateImage->isChecked() );
  m_tempDirSelectionWidget->setDisabled( !m_checkCacheImage->isChecked() );
  m_checkOnlyCreateImage->setEnabled( m_checkCacheImage->isChecked() );
  m_writerSelectionWidget->setDisabled( m_checkOnlyCreateImage->isChecked() );
  m_checkCacheImage->setEnabled( !m_checkOnlyCreateImage->isChecked() );

  if( m_comboCopyMode->currentItem() == 1 ) {
    // cdrecord does not support cloning on-the-fly
    m_checkCacheImage->setChecked(true);
    m_checkCacheImage->setEnabled(false);
    
    m_writingModeWidget->setSupportedModes( K3b::RAW );
  }
  else {
    m_writingModeWidget->setSupportedModes( K3b::TAO|K3b::DAO|K3b::RAW );
  }

  
  
  static_cast<QWidget*>( child( "audio_options" ) )->setDisabled( m_comboCopyMode->currentItem() == 1 );
  
  m_checkIgnoreDataReadErrors->setDisabled( m_comboCopyMode->currentItem() == 1 );
  
  m_groupAdvancedAudioOptions->setEnabled( k3bappcore->mediaCache()->medium( m_comboSourceDevice->selectedDevice() ).content() & K3bMedium::CONTENT_AUDIO &&
					   m_comboCopyMode->currentItem() == 0 );
  
  m_writingModeWidget->setEnabled( !m_checkOnlyCreateImage->isChecked() );

  m_tempDirSelectionWidget->setNeededSize( neededSize() );
  
  setButtonEnabled( START_BUTTON, m_comboSourceDevice->selectedDevice() && 
		    (dev || m_checkOnlyCreateImage->isChecked()) );
}


void K3bCdCopyDialog::slotSourceMediumChanged( K3bDevice::Device* dev )
{
  updateOverrideDevice();

  K3bMedium medium = k3bappcore->mediaCache()->medium( dev );

  m_tempDirSelectionWidget->setNeededSize( neededSize() );

  if( k3bappcore->mediaCache()->toc( dev ).contentType() == K3bDevice::DATA ) {
    m_tempDirSelectionWidget->setSelectionMode( K3bTempDirSelectionWidget::FILE );
    m_tempDirSelectionWidget->setTempPath( m_tempDirSelectionWidget->tempDirectory() 
					   + medium.volumeId().lower() 
					   + ( medium.toc().count() == 1 ? QString(".iso") : QString::null ) );
  }
  else {
    m_tempDirSelectionWidget->setSelectionMode( K3bTempDirSelectionWidget::DIR );
  }

  m_groupAdvancedAudioOptions->setEnabled( k3bappcore->mediaCache()->medium( dev ).content() & K3bMedium::CONTENT_AUDIO );
  m_groupAdvancedDataOptions->setEnabled( k3bappcore->mediaCache()->medium( dev ).content() & K3bMedium::CONTENT_DATA );

  // we can only clone single session CDs
  if( k3bappcore->mediaCache()->toc( dev ).sessions() > 1 ) {
    m_comboCopyMode->setEnabled( false );
    m_comboCopyMode->setCurrentItem( 0 );
  }
  else {
    m_comboCopyMode->setEnabled( true );
  }

  toggleAll();
}


void K3bCdCopyDialog::updateOverrideDevice()
{
  if( !m_checkCacheImage->isChecked() )
    m_writerSelectionWidget->setOverrideDevice( 0 );
  else
    m_writerSelectionWidget->setOverrideDevice( m_comboSourceDevice->selectedDevice(),
						i18n("Use the same device for burning"),
						i18n("<qt>Use the same device for burning <i>(Or insert another medium)</i>") );
}


void K3bCdCopyDialog::loadUserDefaults( KConfigBase* c )
{
  m_writerSelectionWidget->loadConfig( c );
  m_comboSourceDevice->setSelectedDevice( k3bcore->deviceManager()->findDevice( c->readEntry( "source_device" ) ) );
  m_writingModeWidget->loadConfig( c );
  m_checkSimulate->setChecked( c->readBoolEntry( "simulate", false ) );
  m_checkCacheImage->setChecked( !c->readBoolEntry( "on_the_fly", false ) );
  m_checkDeleteImages->setChecked( c->readBoolEntry( "delete_images", true ) );
  m_checkOnlyCreateImage->setChecked( c->readBoolEntry( "only_create_image", false ) );
  m_comboParanoiaMode->setCurrentItem( c->readNumEntry( "paranoia_mode", 0 ) );

  m_spinCopies->setValue( c->readNumEntry( "copies", 1 ) );

  m_tempDirSelectionWidget->readConfig( c );

  if( c->readEntry( "copy mode", "normal" ) == "normal" )
    m_comboCopyMode->setCurrentItem( 0 );
  else
    m_comboCopyMode->setCurrentItem( 1 );

  m_checkReadCdText->setChecked( c->readBoolEntry( "copy cdtext", true ) );
  m_checkPrefereCdText->setChecked( c->readBoolEntry( "prefer cdtext", false ) );
  m_checkIgnoreDataReadErrors->setChecked( c->readBoolEntry( "ignore data read errors", false ) );
  m_checkIgnoreAudioReadErrors->setChecked( c->readBoolEntry( "ignore audio read errors", true ) );
  m_checkNoCorrection->setChecked( c->readBoolEntry( "no correction", false ) );

  m_spinDataRetries->setValue( c->readNumEntry( "data retries", 128 ) );
  m_spinAudioRetries->setValue( c->readNumEntry( "audio retries", 5 ) );

  slotToggleAll();
}


void K3bCdCopyDialog::saveUserDefaults( KConfigBase* c )
{
  m_writingModeWidget->saveConfig( c );
  c->writeEntry( "simulate", m_checkSimulate->isChecked() );
  c->writeEntry( "on_the_fly", !m_checkCacheImage->isChecked() );
  c->writeEntry( "delete_images", m_checkDeleteImages->isChecked() );
  c->writeEntry( "only_create_image", m_checkOnlyCreateImage->isChecked() );
  c->writeEntry( "paranoia_mode", m_comboParanoiaMode->currentText().toInt() );
  c->writeEntry( "copies", m_spinCopies->value() );

  m_writerSelectionWidget->saveConfig( c );
  m_tempDirSelectionWidget->saveConfig( c );

  c->writeEntry( "source_device", m_comboSourceDevice->selectedDevice() ? m_comboSourceDevice->selectedDevice()->devicename() : QString() );

  c->writeEntry( "copy cdtext", m_checkReadCdText->isChecked() );
  c->writeEntry( "prefer cdtext", m_checkPrefereCdText->isChecked() );
  c->writeEntry( "ignore data read errors", m_checkIgnoreDataReadErrors->isChecked() );
  c->writeEntry( "ignore audio read errors", m_checkIgnoreAudioReadErrors->isChecked() );
  c->writeEntry( "no correction", m_checkNoCorrection->isChecked() );
  c->writeEntry( "data retries", m_spinDataRetries->value() );
  c->writeEntry( "audio retries", m_spinAudioRetries->value() );

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
  m_checkCacheImage->setChecked( true );
  m_checkDeleteImages->setChecked( true );
  m_checkOnlyCreateImage->setChecked( false );
  m_comboParanoiaMode->setCurrentItem(0);
  m_spinCopies->setValue(1);
  m_checkReadCdText->setChecked(true);
  m_checkPrefereCdText->setChecked(false);
  m_checkIgnoreDataReadErrors->setChecked(false);
  m_checkIgnoreAudioReadErrors->setChecked(true);
  m_checkNoCorrection->setChecked(false);
  m_comboCopyMode->setCurrentItem( 0 ); // normal
  m_spinDataRetries->setValue(128);
  m_spinAudioRetries->setValue(5);
  m_tempDirSelectionWidget->setTempPath( K3b::defaultTempPath() );

  slotToggleAll();
}


KIO::filesize_t K3bCdCopyDialog::neededSize() const
{
  if( m_comboCopyMode->currentItem() == 0 )
    return k3bappcore->mediaCache()->medium( m_comboSourceDevice->selectedDevice() ).diskInfo().size().mode1Bytes();
  else
    return k3bappcore->mediaCache()->medium( m_comboSourceDevice->selectedDevice() ).diskInfo().size().rawBytes();
}

#include "k3bcdcopydialog.moc"
