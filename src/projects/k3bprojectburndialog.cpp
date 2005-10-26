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


#include "k3bprojectburndialog.h"
#include "k3bdoc.h"
#include "k3bburnprogressdialog.h"
#include "k3bjob.h"
#include "k3btempdirselectionwidget.h"
#include "k3bwriterselectionwidget.h"
#include "k3bstdguiitems.h"
#include "k3bwritingmodewidget.h"
#include <k3bdevice.h>
#include <k3bdevicemanager.h>
#include <k3bglobals.h>
#include <k3bcore.h>

#include <qstring.h>
#include <qpushbutton.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qlayout.h>
#include <qvbox.h>
#include <qcheckbox.h>
#include <qtabwidget.h>
#include <qgroupbox.h>
#include <qspinbox.h>
#include <qlabel.h>

#include <klocale.h>
#include <kconfig.h>
#include <kmessagebox.h>
#include <kguiitem.h>
#include <kstdguiitem.h>
#include <kdebug.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kapplication.h>
#include <kiconloader.h>


K3bProjectBurnDialog::K3bProjectBurnDialog( K3bDoc* doc, QWidget *parent, const char *name, bool modal, bool dvd )
  : K3bInteractionDialog( parent, name, i18n("Project"), QString::null, 
			  START_BUTTON|SAVE_BUTTON|CANCEL_BUTTON, START_BUTTON, 
			  "default " + doc->typeString() + " settings", modal ),
    m_writerSelectionWidget(0),
    m_tempDirSelectionWidget(0),
    m_dvd(dvd)
{
  m_doc = doc;

  setSaveButtonText( i18n("Save"), i18n("Save Settings and close"),
		     i18n("Saves the settings to the project and closes the burn dialog.") );
  setStartButtonText( i18n("Burn") );

  m_job = 0;
}


K3bProjectBurnDialog::~K3bProjectBurnDialog(){
}


void K3bProjectBurnDialog::init()
{
  readSettings();
  if( !m_writerSelectionWidget->writerDevice() )
    m_checkOnlyCreateImage->setChecked(true);
}


void K3bProjectBurnDialog::slotWriterChanged()
{
  toggleAllOptions();
}


void K3bProjectBurnDialog::slotWritingAppChanged( int )
{
  toggleAllOptions();
}


void K3bProjectBurnDialog::toggleAllOptions()
{
  if( K3bDevice::Device* dev = m_writerSelectionWidget->writerDevice() ) {
    if( m_dvd ) {
      if( (dev->type() & (K3bDevice::DVDPR|K3bDevice::DVDPRW)) &&
	  !(dev->type() & (K3bDevice::DVDR|K3bDevice::DVDRW)) ) {
	// no simulation support for DVD+R(W) only drives
	m_checkSimulate->setChecked(false);
	m_checkSimulate->setEnabled(false);

	// what about the writing mode? We just say "overwrite" for DVD+R(W) for now
	m_writingModeWidget->setSupportedModes( K3b::WRITING_MODE_RES_OVWR );
      }
      else {
	// DVD-R(W) supported
	m_writingModeWidget->setSupportedModes( K3b::WRITING_MODE_RES_OVWR|K3b::WRITING_MODE_INCR_SEQ|K3b::DAO );
	m_checkSimulate->setEnabled(true);
      }
    }

    m_buttonStart->setDisabled(false);
  }
  else
    m_buttonStart->setDisabled(true);

  m_writingModeWidget->setDisabled( m_checkOnlyCreateImage->isChecked() );
  m_checkSimulate->setDisabled( m_checkOnlyCreateImage->isChecked() );
  m_checkOnTheFly->setDisabled( m_checkOnlyCreateImage->isChecked() );
  m_checkRemoveBufferFiles->setDisabled( m_checkOnlyCreateImage->isChecked() || m_checkOnTheFly->isChecked() );
  if( m_checkOnlyCreateImage->isChecked() ) {
    m_checkRemoveBufferFiles->setChecked(false);
    m_buttonStart->setDisabled(false);
  }
  m_tempDirSelectionWidget->setDisabled( m_checkOnTheFly->isChecked() && !m_checkOnlyCreateImage->isChecked() );
  m_writerSelectionWidget->setDisabled( m_checkOnlyCreateImage->isChecked() );
  m_spinCopies->setDisabled( m_checkSimulate->isChecked() || m_checkOnlyCreateImage->isChecked() );

  if( !m_dvd ) {
    // default is cdrecord and cdrecord supports all modes
    if( m_writerSelectionWidget->writingApp() == K3b::CDRDAO )
      m_writingModeWidget->setSupportedModes( K3b::DAO );
    else
      m_writingModeWidget->setSupportedModes( K3b::DAO | K3b::TAO | K3b::RAW );
  }
}


int K3bProjectBurnDialog::exec( bool burn )
{
  if( burn && m_job == 0 ) {
    m_buttonStart->show();
    setDefaultButton( START_BUTTON );
    if( !m_writerSelectionWidget->writerDevice() )
      KMessageBox::information( this, 
				i18n("K3b did not find a suitable writer. "
				     "You will only be able to create an image."),
				i18n("No Writer Available"),
				"project_no_writer" );
  }
  else {
    m_buttonStart->hide();
    setDefaultButton( SAVE_BUTTON );
  }

  return K3bInteractionDialog::exec();
}


void K3bProjectBurnDialog::slotSaveClicked()
{
  saveSettings();
  done( Saved );
}


void K3bProjectBurnDialog::slotCancelClicked()
{
  done( Canceled );
}


void K3bProjectBurnDialog::slotStartClicked()
{
  saveSettings();

  // check if enough space in tempdir if not on-the-fly
  if( m_tempDirSelectionWidget )
    if( (!doc()->onTheFly() || doc()->onlyCreateImages()) &&
	doc()->size()/1024 > m_tempDirSelectionWidget->freeTempSpace() ) {
      if( KMessageBox::warningContinueCancel( this, i18n("There seems to be not enough free space in temporary directory. "
						"Write anyway?") ) == KMessageBox::Cancel )
	return;
    }

  K3bJobProgressDialog* dlg = 0;
  if( m_checkOnlyCreateImage && m_checkOnlyCreateImage->isChecked() )
    dlg = new K3bJobProgressDialog( parentWidget() );
  else
    dlg = new K3bBurnProgressDialog( parentWidget() );

  m_job = m_doc->newBurnJob( dlg );

  if( m_writerSelectionWidget )
    m_job->setWritingApp( m_writerSelectionWidget->writingApp() );
  prepareJob( m_job );

  hide();
  dlg->startJob(m_job);

  kdDebug() << "(K3bProjectBurnDialog) job done. cleaning up." << endl;

  delete m_job;
  m_job = 0;
  delete dlg;

  done( Burn );
}


void K3bProjectBurnDialog::prepareGui()
{
  m_tabWidget = new QTabWidget( this );
  setMainWidget( m_tabWidget );
  QWidget* w = new QWidget( m_tabWidget );
  m_tabWidget->addTab( w, i18n("Writing") );
  m_writerSelectionWidget = new K3bWriterSelectionWidget( w );
  m_writerSelectionWidget->setWantedMediumType( m_dvd ? K3bDevice::MEDIA_WRITABLE_DVD : K3bDevice::MEDIA_WRITABLE_CD );
  m_writerSelectionWidget->setWantedMediumState( K3bDevice::STATE_EMPTY );

  QGroupBox* groupWritingMode = new QGroupBox( 1, Qt::Vertical, i18n("Writing Mode"), w );
  groupWritingMode->setInsideMargin( marginHint() );
  m_writingModeWidget = new K3bWritingModeWidget( groupWritingMode );

  m_optionGroup = new QGroupBox( 0, Qt::Vertical, i18n("Options"), w );
  m_optionGroup->layout()->setMargin(0);
  m_optionGroup->layout()->setSpacing(0);
  m_optionGroupLayout = new QVBoxLayout( m_optionGroup->layout() );
  m_optionGroupLayout->setMargin( KDialog::marginHint() );
  m_optionGroupLayout->setSpacing( KDialog::spacingHint() );

  // add the options
  m_checkOnTheFly = K3bStdGuiItems::onTheFlyCheckbox( m_optionGroup );
  m_checkSimulate = K3bStdGuiItems::simulateCheckbox( m_optionGroup );
  m_checkRemoveBufferFiles = K3bStdGuiItems::removeImagesCheckbox( m_optionGroup );
  m_checkOnlyCreateImage = K3bStdGuiItems::onlyCreateImagesCheckbox( m_optionGroup );

  m_optionGroupLayout->addWidget(m_checkSimulate);
  m_optionGroupLayout->addWidget(m_checkOnTheFly);
  m_optionGroupLayout->addWidget(m_checkOnlyCreateImage);
  m_optionGroupLayout->addWidget(m_checkRemoveBufferFiles);

  QGroupBox* groupCopies = new QGroupBox( 2, Qt::Horizontal, i18n("Copies"), w );
  groupCopies->setInsideSpacing( spacingHint() );
  groupCopies->setInsideMargin( marginHint() );
  QLabel* pixLabel = new QLabel( groupCopies );
  pixLabel->setPixmap( SmallIcon( "cdcopy", KIcon::SizeMedium ) );
  pixLabel->setScaledContents( false );
  m_spinCopies = new QSpinBox( 1, 999, 1, groupCopies );

  // arrange it
  QGridLayout* grid = new QGridLayout( w );
  grid->setMargin( KDialog::marginHint() );
  grid->setSpacing( KDialog::spacingHint() );

  grid->addMultiCellWidget( m_writerSelectionWidget, 0, 0, 0, 1 );
  grid->addWidget( groupWritingMode, 1, 0 );
  grid->addMultiCellWidget( m_optionGroup, 1, 3, 1, 1 );
  grid->addWidget( groupCopies, 3, 0 );
  //  grid->addMultiCellWidget( m_tempDirSelectionWidget, 1, 3, 1, 1 );
  grid->setRowStretch( 2, 1 );
  grid->setColStretch( 1, 1 );

  // some default connections that should always be useful
  connect( m_writerSelectionWidget, SIGNAL(writerChanged()), this, SLOT(slotWriterChanged()) );
  connect( m_writerSelectionWidget, SIGNAL(writingAppChanged(int)), this, SLOT(slotWritingAppChanged(int)) );
  connect( m_checkOnTheFly, SIGNAL(toggled(bool)), this, SLOT(toggleAllOptions()) );
  connect( m_checkSimulate, SIGNAL(toggled(bool)), this, SLOT(toggleAllOptions()) );
  connect( m_checkOnlyCreateImage, SIGNAL(toggled(bool)), this, SLOT(toggleAllOptions()) );
  connect( m_writingModeWidget, SIGNAL(writingModeChanged(int)), this, SLOT(toggleAllOptions()) );

  QWidget* tempW = new QWidget( m_tabWidget );
  grid = new QGridLayout( tempW );
  grid->setMargin( KDialog::marginHint() );
  grid->setSpacing( KDialog::spacingHint() );
  m_tabWidget->addTab( tempW, i18n("Image") );
  m_tempDirSelectionWidget = new K3bTempDirSelectionWidget( tempW );
  grid->addWidget( m_tempDirSelectionWidget, 0, 0 );
  m_tempDirSelectionWidget->setNeededSize( doc()->size() );

  // tab order
  setTabOrder( m_writerSelectionWidget, m_writingModeWidget );
  setTabOrder( m_writingModeWidget, groupCopies );
  setTabOrder( groupCopies, m_optionGroup );
}


void K3bProjectBurnDialog::addPage( QWidget* page, const QString& title )
{
  m_tabWidget->addTab( page, title );
}


void K3bProjectBurnDialog::saveSettings()
{
  m_doc->setDummy( m_checkSimulate->isChecked() );
  m_doc->setOnTheFly( m_checkOnTheFly->isChecked() );
  m_doc->setOnlyCreateImages( m_checkOnlyCreateImage->isChecked() );
  m_doc->setRemoveImages( m_checkRemoveBufferFiles->isChecked() );
  m_doc->setSpeed( m_writerSelectionWidget->writerSpeed() );
  m_doc->setBurner( m_writerSelectionWidget->writerDevice() );
  m_doc->setWritingMode( m_writingModeWidget->writingMode() );
  m_doc->setWritingApp( m_writerSelectionWidget->writingApp() );
  m_doc->setCopies( m_spinCopies->value() );
}


void K3bProjectBurnDialog::readSettings()
{
  m_checkSimulate->setChecked( doc()->dummy() );
  m_checkOnTheFly->setChecked( doc()->onTheFly() );
  m_checkOnlyCreateImage->setChecked( m_doc->onlyCreateImages() );
  m_checkRemoveBufferFiles->setChecked( m_doc->removeImages() );
  m_writingModeWidget->setWritingMode( doc()->writingMode() );
  m_writerSelectionWidget->setWriterDevice( doc()->burner() );
  m_writerSelectionWidget->setSpeed( doc()->speed() );
  m_writerSelectionWidget->setWritingApp( doc()->writingApp() );
  m_spinCopies->setValue( m_doc->copies() );
}


void K3bProjectBurnDialog::saveUserDefaults( KConfigBase* c )
{
  m_writingModeWidget->saveConfig( c );
  c->writeEntry( "simulate", m_checkSimulate->isChecked() );
  c->writeEntry( "on_the_fly", m_checkOnTheFly->isChecked() );
  c->writeEntry( "remove_image", m_checkRemoveBufferFiles->isChecked() );
  c->writeEntry( "only_create_image", m_checkOnlyCreateImage->isChecked() );

  m_writerSelectionWidget->saveConfig( c );

  if( m_tempDirSelectionWidget->isEnabled() ) {
    m_tempDirSelectionWidget->saveConfig();
  }
}


void K3bProjectBurnDialog::loadUserDefaults( KConfigBase* c )
{
  m_tempDirSelectionWidget->setTempPath( K3b::defaultTempPath() );

  m_writingModeWidget->loadConfig( c );
  m_checkSimulate->setChecked( c->readBoolEntry( "simulate", false ) );
  m_checkOnTheFly->setChecked( c->readBoolEntry( "on_the_fly", true ) );
  m_checkRemoveBufferFiles->setChecked( c->readBoolEntry( "remove_image", true ) );
  m_checkOnlyCreateImage->setChecked( c->readBoolEntry( "only_create_image", false ) );

  m_writerSelectionWidget->loadConfig( c );
}


void K3bProjectBurnDialog::loadK3bDefaults()
{
  m_writerSelectionWidget->loadDefaults();
  m_writingModeWidget->setWritingMode( K3b::WRITING_MODE_AUTO );
  m_checkSimulate->setChecked( false );
  m_checkOnTheFly->setChecked( true );
  m_checkRemoveBufferFiles->setChecked( true );
  m_checkOnlyCreateImage->setChecked( false );

  m_tempDirSelectionWidget->setTempPath( KGlobal::dirs()->resourceDirs( "tmp" ).first() );
}

#include "k3bprojectburndialog.moc"
