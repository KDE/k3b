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


#include "k3bmovixburndialog.h"
#include "k3bmovixdoc.h"
#include "k3bmovixprogram.h"
#include "k3bmovixoptionswidget.h"

#include <k3bdataimagesettingswidget.h>
#include <k3bdataadvancedimagesettingswidget.h>
#include <k3bdatavolumedescwidget.h>
#include <k3bexternalbinmanager.h>
#include <k3bwriterselectionwidget.h>
#include <k3btempdirselectionwidget.h>
#include <k3bstdguiitems.h>
#include <k3bglobals.h>
#include <k3bdatamodewidget.h>
#include <k3bisooptions.h>
#include <k3bwritingmodewidget.h>
#include <k3bcore.h>

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
    m_doc(doc)
{
  prepareGui();

  m_tempDirSelectionWidget->setSelectionMode( K3bTempDirSelectionWidget::FILE );

  setTitle( i18n("eMovix CD Project"),
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

  // for now we just put the verify checkbox on the main page...
  m_checkVerify = K3bStdGuiItems::verifyCheckBox( m_optionGroup );
  m_optionGroupLayout->addWidget( m_checkVerify );

  QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );
  m_optionGroupLayout->addItem( spacer );


  QString path = m_doc->tempDir();
  if( path.isEmpty() ) {
    path = K3b::defaultTempPath();
    if( doc->isoOptions().volumeID().isEmpty() )
      path.append( "image.iso" );
    else
      path.append( doc->isoOptions().volumeID() + ".iso" );
  }
  m_tempDirSelectionWidget->setTempPath( path );
}


K3bMovixBurnDialog::~K3bMovixBurnDialog()
{
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


void K3bMovixBurnDialog::loadK3bDefaults()
{
  K3bProjectBurnDialog::loadK3bDefaults();

  m_checkStartMultiSesssion->setChecked( false );
  m_dataModeWidget->setDataMode( K3b::DATA_MODE_AUTO );

  m_imageSettingsWidget->load( K3bIsoOptions::defaults() );
  m_advancedImageSettingsWidget->load( K3bIsoOptions::defaults() );
  m_volumeDescWidget->load( K3bIsoOptions::defaults() );

  m_movixOptionsWidget->loadDefaults();

  m_checkVerify->setChecked( false );

  toggleAllOptions();
}


void K3bMovixBurnDialog::loadUserDefaults( KConfig* c )
{
  K3bProjectBurnDialog::loadUserDefaults(c);

  m_checkStartMultiSesssion->setChecked( c->readBoolEntry( "start_multisession", false ) );

  m_dataModeWidget->loadConfig(c);

  K3bIsoOptions o = K3bIsoOptions::load( c );
  m_imageSettingsWidget->load( o );
  m_advancedImageSettingsWidget->load( o );
  m_volumeDescWidget->load( o );

  m_movixOptionsWidget->loadConfig(c);

  m_checkVerify->setChecked( c->readBoolEntry( "verify data", false ) );

  toggleAllOptions();
}


void K3bMovixBurnDialog::saveUserDefaults( KConfig* c )
{
  K3bProjectBurnDialog::saveUserDefaults(c);

  c->writeEntry( "start_multisession", m_checkStartMultiSesssion->isChecked() );

  m_dataModeWidget->saveConfig(c);

  K3bIsoOptions o;
  m_imageSettingsWidget->save( o );
  m_advancedImageSettingsWidget->save( o );
  m_volumeDescWidget->save( o );
  o.save( c );

  c->writeEntry( "verify data", m_checkVerify->isChecked() );

  m_movixOptionsWidget->saveConfig(c);
}


void K3bMovixBurnDialog::saveSettings()
{
  K3bProjectBurnDialog::saveSettings();

  m_movixOptionsWidget->saveSettings( m_doc );

  m_doc->setMultiSessionMode( m_checkStartMultiSesssion->isChecked() ? K3bDataDoc::START : K3bDataDoc::NONE );

  // save iso image settings
  m_imageSettingsWidget->save( m_doc->isoOptions() );
  m_advancedImageSettingsWidget->save( m_doc->isoOptions() );
  m_volumeDescWidget->save( m_doc->isoOptions() );

  m_doc->setDataMode( m_dataModeWidget->dataMode() );

  // save image file path
  m_doc->setTempDir( m_tempDirSelectionWidget->tempPath() );

  m_doc->setVerifyData( m_checkVerify->isChecked() );
}


void K3bMovixBurnDialog::readSettings()
{
  K3bProjectBurnDialog::readSettings();

  m_checkStartMultiSesssion->setChecked( m_doc->multiSessionMode() == K3bDataDoc::START );

  m_checkVerify->setChecked( m_doc->verifyData() );

  m_imageSettingsWidget->load( m_doc->isoOptions() );
  m_advancedImageSettingsWidget->load( m_doc->isoOptions() );
  m_volumeDescWidget->load( m_doc->isoOptions() );

  m_dataModeWidget->setDataMode( m_doc->dataMode() );

  // first of all we need a movix installation object
  const K3bMovixBin* bin = dynamic_cast<const K3bMovixBin*>( k3bcore->externalBinManager()->binObject("eMovix") );
  if( bin ) {
    m_movixOptionsWidget->init( bin );
    m_movixOptionsWidget->readSettings( m_doc );
  }
  else {
    KMessageBox::error( this, i18n("Could not find a valid eMovix installation.") );
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
				     i18n("Do you want to overwrite %1?").arg(m_tempDirSelectionWidget->tempPath()),
				     i18n("File Exists") )
	  != KMessageBox::Yes )
	return;
    }
  }

  if( m_writingModeWidget->writingMode() == K3b::DAO &&
      m_checkStartMultiSesssion->isChecked() &&
      m_writerSelectionWidget->writingApp() == K3b::CDRECORD )
    if( KMessageBox::warningContinueCancel( this,
					    i18n("Most writers do not support writing "
						 "multisession CDs in DAO mode.") )
	== KMessageBox::Cancel )
      return;


  K3bProjectBurnDialog::slotStartClicked();
}


void K3bMovixBurnDialog::toggleAllOptions()
{
  K3bProjectBurnDialog::toggleAllOptions();

  if( m_checkSimulate->isChecked() || m_checkOnlyCreateImage->isChecked() ) {
    m_checkVerify->setChecked(false);
    m_checkVerify->setEnabled(false);
  }
  else
    m_checkVerify->setEnabled(true);
}

#include "k3bmovixburndialog.moc"
