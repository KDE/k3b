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

#include <klocale.h>
#include <kdebug.h>
#include <kmessagebox.h>

#include <qcheckbox.h>
#include <qlayout.h>


K3bMovixBurnDialog::K3bMovixBurnDialog( K3bMovixDoc* doc, QWidget* parent, const char* name, bool modal )
  : K3bProjectBurnDialog( doc, parent, name, modal ),
    m_doc(doc),
    m_installation(0)
{
  prepareGui();

  m_movixOptionsWidget = new K3bMovixOptionsWidget( this );
  addPage( m_movixOptionsWidget, i18n("eMovix Options") );

  // create volume descriptor tab
  m_volumeDescWidget = new K3bDataVolumeDescWidget( this );
  m_volumeDescWidget->layout()->setMargin( marginHint() );
  addPage( m_volumeDescWidget, i18n("Volume Desc") );

  // create image settings tab
  m_imageSettingsWidget = new K3bDataImageSettingsWidget( this );
  m_imageSettingsWidget->layout()->setMargin( marginHint() );
  addPage( m_imageSettingsWidget, i18n("Data Settings") );

  // create advanced image settings tab
  m_advancedImageSettingsWidget = new K3bDataAdvancedImageSettingsWidget( this );
  m_advancedImageSettingsWidget->layout()->setMargin( marginHint() );
  addPage( m_advancedImageSettingsWidget, i18n("Advanced") );

}


K3bMovixBurnDialog::~K3bMovixBurnDialog()
{
  if( m_installation )
    delete m_installation;
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

			
  // -- saving current speed --------------------------------------
  m_doc->setSpeed( m_writerSelectionWidget->writerSpeed() );
	
  // -- saving current device --------------------------------------
  m_doc->setBurner( m_writerSelectionWidget->writerDevice() );

  // save iso image settings
  m_imageSettingsWidget->save( m_doc->isoOptions() );
  m_advancedImageSettingsWidget->save( m_doc->isoOptions() );
  m_volumeDescWidget->save( m_doc->isoOptions() );
	

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

  m_imageSettingsWidget->load( m_doc->isoOptions() );
  m_advancedImageSettingsWidget->load( m_doc->isoOptions() );
  m_volumeDescWidget->load( m_doc->isoOptions() );

  // first of all we need a movix installation object
  QString path = K3bExternalBinManager::self()->binPath("eMovix");
  m_installation = K3bMovixInstallation::probeInstallation( path );
  if( m_installation ) {
    m_movixOptionsWidget->init( m_installation );
    m_movixOptionsWidget->readSettings( m_doc );
  }
  else {
    KMessageBox::error( this, i18n("Could not find eMovix installation in %1").arg(path) );
  }
}


#include "k3bmovixburndialog.moc"
