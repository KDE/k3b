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

#include "k3bclonedialog.h"
#include "k3bclonejob.h"

#include <k3btempdirselectionwidget.h>
#include <k3bwriterselectionwidget.h>
#include <k3bglobals.h>
#include <k3bstdguiitems.h>
#include <k3bdevicecombobox.h>
#include <device/k3bdevice.h>
#include <device/k3bdevicemanager.h>
#include <k3bcore.h>
#include <k3bexternalbinmanager.h>
#include <k3bburnprogressdialog.h>
#include <k3bthememanager.h>

#include <qlayout.h>
#include <qgroupbox.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qtabwidget.h>
#include <qspinbox.h>
#include <qptrlist.h>
#include <qfile.h>
#include <qpushbutton.h>
#include <qtooltip.h>
#include <qwhatsthis.h>

#include <kdebug.h>
#include <klocale.h>
#include <kconfig.h>
#include <kapplication.h>
#include <kstandarddirs.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kglobal.h>



K3bCloneDialog::K3bCloneDialog( QWidget* parent, const char* name, bool modal )
  : K3bInteractionDialog( parent, name,
			  i18n("CD Cloning"),
			  i18n("Perfect CD copy"),
			  START_BUTTON|CANCEL_BUTTON,
			  START_BUTTON,
			  modal ),
    m_job(0)
{
  QWidget* w = mainWidget();

  //
  // Source group
  // //////////////////////////////////////////////////////////////////////////
  QGroupBox* groupSource = new QGroupBox( 1, Qt::Vertical, i18n("CD Reader Device"), w );
  groupSource->setInsideSpacing( spacingHint() );
  groupSource->setInsideMargin( marginHint() );
  m_comboSourceDevice = new K3bDeviceComboBox( groupSource );
  // //////////////////////////////////////////////////////////////////////////

  //
  // Writer group
  // //////////////////////////////////////////////////////////////////////////
  m_writerSelectionWidget = new K3bWriterSelectionWidget( false, w );
  m_writerSelectionWidget->setSupportedWritingApps( K3b::CDRECORD );
  // //////////////////////////////////////////////////////////////////////////

  //
  // Option tab
  // //////////////////////////////////////////////////////////////////////////
  QTabWidget* tabWidget = new QTabWidget( w );

  // option tab --------------------
  QWidget* optionTab = new QWidget( tabWidget );
  QGridLayout* optionTabGrid = new QGridLayout( optionTab );
  optionTabGrid->setSpacing( spacingHint() );
  optionTabGrid->setMargin( marginHint() );

  QGroupBox* groupOptions = new QGroupBox( 4, Qt::Vertical, i18n("Options"), optionTab );
  groupOptions->setInsideSpacing( spacingHint() );
  groupOptions->setInsideMargin( marginHint() );
  m_checkSimulate = K3bStdGuiItems::simulateCheckbox( groupOptions );
  m_checkBurnfree = K3bStdGuiItems::burnproofCheckbox( groupOptions );
  m_checkOnlyCreateImage = K3bStdGuiItems::onlyCreateImagesCheckbox( groupOptions );
  m_checkDeleteImages = K3bStdGuiItems::removeImagesCheckbox( groupOptions );

  QGroupBox* groupCopies = new QGroupBox( 2, Qt::Horizontal, i18n("Copies"), optionTab );
  groupCopies->setInsideSpacing( spacingHint() );
  groupCopies->setInsideMargin( marginHint() );
  QLabel* pixLabel = new QLabel( groupCopies );
  if( K3bTheme* theme = k3bthememanager->currentTheme() )
    pixLabel->setPixmap( theme->pixmap( "k3b_cd_copy" ) );
  pixLabel->setScaledContents( false );
  m_spinCopies = new QSpinBox( groupCopies );
  m_spinCopies->setMinValue( 1 );
  m_spinCopies->setMaxValue( 99 );

  m_tempDirSelectionWidget = new K3bTempDirSelectionWidget( optionTab );
  m_tempDirSelectionWidget->setSelectionMode( K3bTempDirSelectionWidget::FILE );

  optionTabGrid->addWidget( groupOptions, 0, 0 );
  optionTabGrid->addWidget( groupCopies, 1, 0 );
  optionTabGrid->addMultiCellWidget( m_tempDirSelectionWidget, 0, 1, 1, 1 );
  optionTabGrid->setRowStretch( 1, 1 );
  optionTabGrid->setColStretch( 1, 1 );

  tabWidget->addTab( optionTab, i18n("&Options") );


  // advanced tab ------------------
  QWidget* advancedTab = new QWidget( tabWidget );
  QGridLayout* advancedTabGrid = new QGridLayout( advancedTab );
  advancedTabGrid->setSpacing( spacingHint() );
  advancedTabGrid->setMargin( marginHint() );

  m_checkNoCorr = new QCheckBox( i18n("No correction"), advancedTab );
  advancedTabGrid->addWidget( m_checkNoCorr, 0, 0 );
  advancedTabGrid->setRowStretch( 1, 1 );

  tabWidget->addTab( advancedTab, i18n("&Advanced") );
  // //////////////////////////////////////////////////////////////////////////


  //
  // setup layout
  // //////////////////////////////////////////////////////////////////////////
  QGridLayout* grid = new QGridLayout( w );
  grid->setMargin( 0 );
  grid->setSpacing( spacingHint() );

  grid->addWidget( groupSource, 0, 0 );
  grid->addWidget( m_writerSelectionWidget, 1, 0 );
  grid->addWidget( tabWidget, 2, 0 );
  grid->setRowStretch( 2, 1 );
  // //////////////////////////////////////////////////////////////////////////


  //
  // setup connections
  // //////////////////////////////////////////////////////////////////////////
  connect( m_writerSelectionWidget, SIGNAL(writerChanged()), this, SLOT(slotToggleAll()) );
  connect( m_checkSimulate, SIGNAL(toggled(bool)), this, SLOT(slotToggleAll()) );
  connect( m_checkOnlyCreateImage, SIGNAL(toggled(bool)), this, SLOT(slotToggleAll()) );


  QToolTip::add( m_checkNoCorr, i18n("Disable the source drive's error correction") );
  QWhatsThis::add( m_checkNoCorr, i18n("<p>If this option is checked K3b will disable the "
				       "source drive's ECC/EDC error correction. This way sectors "
				       "that are unreadable by intention can be read."
				       "<p>This may be useful for cloning CDs with copy "
				       "protection based on corrupted sectors.") );
}


K3bCloneDialog::~K3bCloneDialog()
{
}


void K3bCloneDialog::show()
{
  init();
  K3bInteractionDialog::show();
}


void K3bCloneDialog::init()
{
  //
  // if the system supports ATAPI we use every device as reader
  // otherwise only the SCSI devices
  //

  const K3bExternalBin* readcdBin = k3bcore->externalBinManager()->binObject( "readcd" );
  const K3bExternalBin* cdrecordBin = k3bcore->externalBinManager()->binObject( "cdrecord" );
  bool atapi = true;

  if( readcdBin ) {
    atapi = ( (K3bCdDevice::plainAtapiSupport() && readcdBin->hasFeature("plain-atapi")) ||
	      (K3bCdDevice::hackedAtapiSupport() && readcdBin->hasFeature("hacked-atapi")) );
  }

  if( cdrecordBin && !cdrecordBin->hasFeature( "clone" ) ) {
    KMessageBox::sorry( this, i18n("Cdrecord version %1 does not have cloning support.").arg(cdrecordBin->version) );
  }


  m_comboSourceDevice->clear();
  QPtrList<K3bDevice>& devices = k3bcore->deviceManager()->cdReader();
  for( QPtrListIterator<K3bDevice> it( devices ); it.current(); ++it ) {
    K3bDevice* dev = it.current();
    
    if( dev->interfaceType() == K3bDevice::SCSI || atapi )
      m_comboSourceDevice->addDevice( dev );
  }

  slotLoadUserDefaults();
}


void K3bCloneDialog::slotStartClicked()
{
  //
  // check for m_tempDirSelectionWidget->tempPath() and
  // m_tempDirSelectionWidget-tempPath() + ".toc"
  //
  if( QFile::exists( m_tempDirSelectionWidget->tempPath() ) ) {
    if( KMessageBox::warningYesNo( this,
				   i18n("Do you want to overwrite %1?").arg(m_tempDirSelectionWidget->tempPath()),
				   i18n("File Exists") )
	!= KMessageBox::Yes )
      return;
  }

  if( QFile::exists( m_tempDirSelectionWidget->tempPath() + ".toc" ) ) {
    if( KMessageBox::warningYesNo( this,
				   i18n("Do you want to overwrite %1?").arg(m_tempDirSelectionWidget->tempPath() + ".toc"),
				   i18n("File Exists") )
	!= KMessageBox::Yes )
      return;
  }



  if( !m_job )
    m_job = new K3bCloneJob( this );

  m_job->setWriterDevice( m_writerSelectionWidget->writerDevice() );
  m_job->setReaderDevice( m_comboSourceDevice->selectedDevice() );
  m_job->setImagePath( m_tempDirSelectionWidget->tempPath() );
  m_job->setNoCorrection( m_checkNoCorr->isChecked() );
  m_job->setRemoveImageFiles( m_checkDeleteImages->isChecked() );
  m_job->setOnlyCreateImage( m_checkOnlyCreateImage->isChecked() );
  m_job->setSimulate( m_checkSimulate->isChecked() );
  m_job->setWriteSpeed( m_writerSelectionWidget->writerSpeed() );
  m_job->setBurnfree( m_checkBurnfree->isChecked() );
  m_job->setCopies( m_checkSimulate->isChecked() ? 1 : m_spinCopies->value() );


  K3bJobProgressDialog* dlg = 0;
  if( m_checkOnlyCreateImage->isChecked() ) {
    dlg = new K3bJobProgressDialog( kapp->mainWidget() );
  }
  else {
    dlg = new K3bBurnProgressDialog( kapp->mainWidget() );
  }
   
  hide();
  dlg->startJob( m_job );
  delete dlg;
}


void K3bCloneDialog::slotLoadUserDefaults()
{
  m_tempDirSelectionWidget->setTempPath( K3b::defaultTempPath() );

  KConfig* c = k3bcore->config();
  c->setGroup( "default cd cloning settings" );

  m_checkSimulate->setChecked( c->readBoolEntry( "simulate", false ) );
  m_checkOnlyCreateImage->setChecked( c->readBoolEntry( "only_create_image", false ) );
  m_checkDeleteImages->setChecked( c->readBoolEntry( "remove_image", true ) );
  m_checkNoCorr->setChecked( c->readBoolEntry( "no_correction", false ) );
  m_checkBurnfree->setChecked( c->readBoolEntry( "burnfree", true ) );

  m_spinCopies->setValue( c->readNumEntry( "copies", 1 ) );

  m_writerSelectionWidget->loadConfig( c );

  slotToggleAll();
}


void K3bCloneDialog::slotSaveUserDefaults()
{
  KConfig* c = k3bcore->config();

  c->setGroup( "General Options" );
  c->writePathEntry( "Temp Dir", m_tempDirSelectionWidget->tempPath() );

  c->setGroup( "default cd cloning settings" );

  c->writeEntry( "simulate", m_checkSimulate->isChecked() );
  c->writeEntry( "only_create_image", m_checkOnlyCreateImage->isChecked() );
  c->writeEntry( "remove_image", m_checkDeleteImages->isChecked() );
  c->writeEntry( "no_correction", m_checkNoCorr->isChecked() );
  c->writeEntry( "burnfree", m_checkBurnfree->isChecked() );

  c->writeEntry( "copies", m_spinCopies->value() );

  m_writerSelectionWidget->saveConfig( c );
}


void K3bCloneDialog::slotLoadK3bDefaults()
{
  m_writerSelectionWidget->loadDefaults();
  m_tempDirSelectionWidget->setTempPath( KGlobal::dirs()->resourceDirs( "tmp" ).first() );

  m_checkSimulate->setChecked( false );
  m_checkOnlyCreateImage->setChecked( false );
  m_checkDeleteImages->setChecked( true );
  m_checkNoCorr->setChecked( false );
  m_checkBurnfree->setChecked( true );

  m_spinCopies->setValue( 1 );

  slotToggleAll();
}


void K3bCloneDialog::slotToggleAll()
{
  if( K3bCdDevice::CdDevice* dev = m_writerSelectionWidget->writerDevice() ) {

    if( dev->burnfree() )
      m_checkBurnfree->setEnabled( !m_checkOnlyCreateImage->isChecked() );
    else {
      m_checkBurnfree->setEnabled( false );
      m_checkBurnfree->setChecked( false );
    }

    m_checkSimulate->setEnabled( !m_checkOnlyCreateImage->isChecked() );
    m_checkDeleteImages->setEnabled( !m_checkOnlyCreateImage->isChecked() );
    m_spinCopies->setEnabled( !m_checkSimulate->isChecked() && !m_checkOnlyCreateImage->isChecked() );
    if ( m_checkOnlyCreateImage->isChecked() )
      m_checkDeleteImages->setChecked( false );
    m_buttonStart->setEnabled(true);
  }
  else {
    m_buttonStart->setEnabled(false);
  }
}


#include "k3bclonedialog.moc"
