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

#include <klocale.h>
#include <kdebug.h>
#include <kmessagebox.h>

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


void K3bMovixBurnDialog::loadDefaults()
{
}


void K3bMovixBurnDialog::loadUserDefaults()
{
}


void K3bMovixBurnDialog::saveUserDefaults()
{
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
    slotCancel();
  }
}


void K3bMovixBurnDialog::slotOk()
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
				    
    
  K3bProjectBurnDialog::slotOk();
}


#include "k3bmovixburndialog.moc"
