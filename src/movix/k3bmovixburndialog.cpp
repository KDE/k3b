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


#include "k3bmovixburndialog.h"
#include "k3bmovixdoc.h"
#include "k3bmovixinstallation.h"
#include "k3bmovixoptionswidget.h"

#include <data/k3bdataimagesettingswidget.h>
#include <data/k3bdataadvancedimagesettingswidget.h>
#include <data/k3bdatavolumedescwidget.h>
#include <tools/k3bexternalbinmanager.h>
#include <k3bwriterselectionwidget.h>
#include <k3btempdirselectionwidget.h>
#include <k3bstdguiitems.h>
#include <tools/k3bglobals.h>
#include <tools/k3bdatamodewidget.h>
#include <k3bisooptions.h>

#include <klocale.h>
#include <kdebug.h>
#include <kmessagebox.h>
#include <kio/global.h>
#include <kapplication.h>
#include <kconfig.h>

#include <qcheckbox.h>
#include <qlayout.h>
#include <qgroupbox.h>
#include <qfileinfo.h>


K3bMovixBurnDialog::K3bMovixBurnDialog( K3bMovixDoc* doc, QWidget* parent, const char* name, bool modal )
  : K3bProjectBurnDialog( doc, parent, name, modal ),
    m_doc(doc),
    m_installation(0)
{
  prepareGui();

  setTitle( i18n("eMovix Project"), 
	    i18n("1 file (%1)", "%n files (%1)", m_doc->movixFileItems().count()).arg(KIO::convertSize(m_doc->size())) );

  m_movixOptionsWidget = new K3bMovixOptionsWidget( this );
  addPage( m_movixOptionsWidget, i18n("eMovix") );

  setupSettingsPage();

  // create volume descriptor tab
  m_volumeDescWidget = new K3bDataVolumeDescWidget( this );
  m_volumeDescWidget->layout()->setMargin( marginHint() );
  addPage( m_volumeDescWidget, i18n("Volume Desc") );

  // create image settings tab
  m_imageSettingsWidget = new K3bDataImageSettingsWidget( this );
  m_imageSettingsWidget->layout()->setMargin( marginHint() );
  addPage( m_imageSettingsWidget, i18n("Filesystem") );

  // create advanced image settings tab
  m_advancedImageSettingsWidget = new K3bDataAdvancedImageSettingsWidget( this );
  m_advancedImageSettingsWidget->layout()->setMargin( marginHint() );
  addPage( m_advancedImageSettingsWidget, i18n("Advanced") );

  QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );
  m_optionGroupLayout->addItem( spacer );
}


K3bMovixBurnDialog::~K3bMovixBurnDialog()
{
  if( m_installation )
    delete m_installation;
}


void K3bMovixBurnDialog::setupSettingsPage()
{
  QWidget* frame = new QWidget( this );
  QGridLayout* frameLayout = new QGridLayout( frame );
  frameLayout->setSpacing( spacingHint() );
  frameLayout->setMargin( marginHint() );

  QGroupBox* groupDataMode = new QGroupBox( 1, Qt::Vertical, i18n("Datatrack Mode"), frame );
  m_dataModeWidget = new K3bDataModeWidget( groupDataMode );

  QGroupBox* groupMultisession = new QGroupBox( 1, Qt::Vertical, i18n("Multisession"), frame );
  m_checkStartMultiSesssion = K3bStdGuiItems::startMultisessionCheckBox( groupMultisession );

  frameLayout->addWidget( groupDataMode, 0, 0 );
  frameLayout->addWidget( groupMultisession, 1, 0 );
  frameLayout->setRowStretch( 2, 1 );

  addPage( frame, i18n("Settings") );
}


void K3bMovixBurnDialog::slotLoadK3bDefaults()
{
  m_checkSimulate->setChecked( false );
  m_checkDao->setChecked( true );
  m_checkOnTheFly->setChecked( true );
  m_checkBurnproof->setChecked( true );
  m_checkStartMultiSesssion->setChecked( false );
  m_checkRemoveBufferFiles->setChecked( true );
  m_checkOnlyCreateImage->setChecked( false );
  m_dataModeWidget->setDataMode( K3b::AUTO );

  m_imageSettingsWidget->load( K3bIsoOptions::defaults() );
  m_advancedImageSettingsWidget->load( K3bIsoOptions::defaults() );
  m_volumeDescWidget->load( K3bIsoOptions::defaults() );

  m_movixOptionsWidget->loadDefaults();

  toggleAllOptions();
}


void K3bMovixBurnDialog::slotLoadUserDefaults()
{
  KConfig* c = kapp->config();
  c->setGroup( "default movix settings" );

  m_checkSimulate->setChecked( c->readBoolEntry( "simulate", false ) );
  m_checkDao->setChecked( c->readBoolEntry( "dao", true ) );
  m_checkOnTheFly->setChecked( c->readBoolEntry( "on_the_fly", true ) );
  m_checkBurnproof->setChecked( c->readBoolEntry( "burnproof", true ) );
  m_checkRemoveBufferFiles->setChecked( c->readBoolEntry( "remove_image", true ) );
  m_checkOnlyCreateImage->setChecked( c->readBoolEntry( "only_create_image", false ) );
  m_checkStartMultiSesssion->setChecked( c->readBoolEntry( "start_multisession", false ) );

  QString datamode = c->readEntry( "data_track_mode" );
  if( datamode == "mode1" )
    m_dataModeWidget->setDataMode( K3b::MODE1 );
  else if( datamode == "mode2" )
    m_dataModeWidget->setDataMode( K3b::MODE2 );
  else
    m_dataModeWidget->setDataMode( K3b::AUTO );

  K3bIsoOptions o = K3bIsoOptions::load( c );
  m_imageSettingsWidget->load( o );
  m_advancedImageSettingsWidget->load( o );
  m_volumeDescWidget->load( o );

  m_movixOptionsWidget->loadConfig(c);

  toggleAllOptions();
}


void K3bMovixBurnDialog::slotSaveUserDefaults()
{
  KConfig* c = kapp->config();

  c->setGroup( "default movix settings" );

  c->writeEntry( "simulate", m_checkSimulate->isChecked() );
  c->writeEntry( "dao", m_checkDao->isChecked() );
  c->writeEntry( "on_the_fly", m_checkOnTheFly->isChecked() );
  c->writeEntry( "burnproof", m_checkBurnproof->isChecked() );
  c->writeEntry( "remove_image", m_checkRemoveBufferFiles->isChecked() );
  c->writeEntry( "only_create_image", m_checkOnlyCreateImage->isChecked() );
  c->writeEntry( "start_multisession", m_checkStartMultiSesssion->isChecked() );

  QString datamode;
  if( m_dataModeWidget->dataMode() == K3b::MODE1 )
    datamode = "mode1";
  else if( m_dataModeWidget->dataMode() == K3b::MODE2 )
    datamode = "mode2";
  else
    datamode = "auto";
  c->writeEntry( "data_track_mode", datamode );


  K3bIsoOptions o;
  m_imageSettingsWidget->save( o );
  m_advancedImageSettingsWidget->save( o );
  m_volumeDescWidget->save( o );
  o.save( c );

  m_movixOptionsWidget->saveConfig(c);
}


void K3bMovixBurnDialog::saveSettings()
{
  m_movixOptionsWidget->saveSettings( m_doc );

  m_doc->setDao( m_checkDao->isChecked() );
  m_doc->setDummy( m_checkSimulate->isChecked() );
  m_doc->setOnTheFly( m_checkOnTheFly->isChecked() );
  m_doc->setBurnproof( m_checkBurnproof->isChecked() );
  m_doc->setOnlyCreateImage( m_checkOnlyCreateImage->isChecked() );
  m_doc->setDeleteImage( m_checkRemoveBufferFiles->isChecked() );
  m_doc->setMultiSessionMode( m_checkStartMultiSesssion->isChecked() ? K3bDataDoc::START : K3bDataDoc::NONE );
			
  // -- saving current speed --------------------------------------
  m_doc->setSpeed( m_writerSelectionWidget->writerSpeed() );
	
  // -- saving current device --------------------------------------
  m_doc->setBurner( m_writerSelectionWidget->writerDevice() );

  // save iso image settings
  m_imageSettingsWidget->save( m_doc->isoOptions() );
  m_advancedImageSettingsWidget->save( m_doc->isoOptions() );
  m_volumeDescWidget->save( m_doc->isoOptions() );
	
  m_doc->setDataMode( m_dataModeWidget->dataMode() );

  // save image file path
  m_doc->setIsoImage( m_tempDirSelectionWidget->tempPath() );  
}
 

void K3bMovixBurnDialog::readSettings()
{
  m_checkDao->setChecked( doc()->dao() );
  m_checkSimulate->setChecked( doc()->dummy() );
  m_checkOnTheFly->setChecked( doc()->onTheFly() );
  m_checkBurnproof->setChecked( doc()->burnproof() );
  m_checkOnlyCreateImage->setChecked( m_doc->onlyCreateImage() );
  m_checkRemoveBufferFiles->setChecked( m_doc->deleteImage() );
  m_checkStartMultiSesssion->setChecked( m_doc->multiSessionMode() == K3bDataDoc::START );

  m_imageSettingsWidget->load( m_doc->isoOptions() );
  m_advancedImageSettingsWidget->load( m_doc->isoOptions() );
  m_volumeDescWidget->load( m_doc->isoOptions() );

  m_dataModeWidget->setDataMode( m_doc->dataMode() );

  // first of all we need a movix installation object
  QString path = K3bExternalBinManager::self()->binPath("eMovix");
  m_installation = K3bMovixInstallation::probeInstallation( path );
  if( m_installation ) {
    m_movixOptionsWidget->init( m_installation );
    m_movixOptionsWidget->readSettings( m_doc );
  }
  else {
    KMessageBox::error( this, i18n("Could not find eMovix installation in %1").arg(path) );
    slotCancelClicked();
  }
}


void K3bMovixBurnDialog::slotStartClicked()
{
  if( m_checkOnlyCreateImage->isChecked() ||
      !m_checkOnTheFly->isChecked() ) {
    QFileInfo fi( m_tempDirSelectionWidget->tempPath() );
    if( fi.isDir() )
      m_tempDirSelectionWidget->setTempPath( fi.filePath() + "/image.iso" );
    
    if( QFile::exists( m_tempDirSelectionWidget->tempPath() ) ) {
      if( KMessageBox::warningYesNo( this, 
				     i18n("Do you want to overwrite %1").arg(m_tempDirSelectionWidget->tempPath()), 
				     i18n("File exists...") ) 
	  != KMessageBox::Yes )
	return;
    }
  }

  if( m_checkDao->isChecked() &&
      m_checkStartMultiSesssion->isChecked() &&
      m_writerSelectionWidget->writingApp() == K3b::CDRECORD )
    if( KMessageBox::warningContinueCancel( this,
					    i18n("Most writers do not support writing "
						 "multisession cds in DAO mode.") )
	== KMessageBox::Cancel )
      return;
				    
    
  K3bProjectBurnDialog::slotStartClicked();
}


#include "k3bmovixburndialog.moc"
