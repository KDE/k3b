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


#include "k3bdataburndialog.h"
#include "k3bdataimagesettingswidget.h"
#include "k3bdataadvancedimagesettingswidget.h"
#include "k3bdatavolumedescwidget.h"
#include "k3bdatamultisessioncombobox.h"
#include "k3bdataview.h"

#include <k3bisooptions.h>
#include <k3bdatadoc.h>
#include <k3bdevice.h>
#include <k3bwriterselectionwidget.h>
#include <k3btempdirselectionwidget.h>
#include <k3bjob.h>
#include <k3bcore.h>
#include <k3bstdguiitems.h>
#include <k3bdatamodewidget.h>
#include <k3bglobals.h>
#include <k3bwritingmodewidget.h>

#include <qcheckbox.h>
#include <qframe.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qtoolbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qpoint.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qfileinfo.h>
#include <qtabwidget.h>
#include <qspinbox.h>

#include <kmessagebox.h>
#include <klineedit.h>
#include <klocale.h>
#include <kconfig.h>
#include <kstandarddirs.h>
#include <kfiledialog.h>
#include <kcombobox.h>
#include <kio/global.h>


#include "k3bfilecompilationsizehandler.h"


K3bDataBurnDialog::K3bDataBurnDialog(K3bDataDoc* _doc, QWidget *parent, const char *name, bool modal )
  : K3bProjectBurnDialog( _doc, parent, name, modal )
{
  m_bInitializing = true;
  prepareGui();

  m_writerSelectionWidget->setWantedMediumState( K3bDevice::STATE_EMPTY|K3bDevice::STATE_INCOMPLETE );

  setTitle( i18n("Data Project"), i18n("Size: %1").arg( KIO::convertSize(_doc->size()) ) );

  // for now we just put the verify checkbox on the main page...
  m_checkVerify = K3bStdGuiItems::verifyCheckBox( m_optionGroup );
  m_optionGroupLayout->addWidget( m_checkVerify );

  QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );
  m_optionGroupLayout->addItem( spacer );

  setupSettingsTab();

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

  connect( m_comboMultisession, SIGNAL(activated(int)),
	   this, SLOT(slotMultiSessionModeChanged()) );

  m_bInitializing = false;

  readSettings();

  m_tempDirSelectionWidget->setSelectionMode( K3bTempDirSelectionWidget::FILE );
  QString path = _doc->tempDir();
  if( path.isEmpty() ) {
    path = K3b::defaultTempPath();
    if( _doc->isoOptions().volumeID().isEmpty() )
      path.append( "image.iso" );
    else
      path.append( _doc->isoOptions().volumeID() + ".iso" );
  }
  m_tempDirSelectionWidget->setTempPath( path );
}

K3bDataBurnDialog::~K3bDataBurnDialog(){
}


void K3bDataBurnDialog::saveSettings()
{
  K3bProjectBurnDialog::saveSettings();

  // save iso image settings
  K3bIsoOptions o = ((K3bDataDoc*)doc())->isoOptions();
  m_imageSettingsWidget->save( o );
  m_advancedImageSettingsWidget->save( o );
  m_volumeDescWidget->save( o );
  ((K3bDataDoc*)doc())->setIsoOptions( o );

  // save image file path
  ((K3bDataDoc*)doc())->setTempDir( m_tempDirSelectionWidget->tempPath() );

  // save multisession settings
  ((K3bDataDoc*)doc())->setMultiSessionMode( m_comboMultisession->multiSessionMode() );

  ((K3bDataDoc*)doc())->setDataMode( m_dataModeWidget->dataMode() );

  ((K3bDataDoc*)doc())->setVerifyData( m_checkVerify->isChecked() );
}


void K3bDataBurnDialog::readSettings()
{
  K3bProjectBurnDialog::readSettings();

  // read multisession
  m_comboMultisession->setMultiSessionMode( ((K3bDataDoc*)doc())->multiSessionMode() );

  if( !doc()->tempDir().isEmpty() )
    m_tempDirSelectionWidget->setTempPath( doc()->tempDir() );

  m_checkVerify->setChecked( ((K3bDataDoc*)doc())->verifyData() );

  m_imageSettingsWidget->load( ((K3bDataDoc*)doc())->isoOptions() );
  m_advancedImageSettingsWidget->load( ((K3bDataDoc*)doc())->isoOptions() );
  m_volumeDescWidget->load( ((K3bDataDoc*)doc())->isoOptions() );

  m_dataModeWidget->setDataMode( ((K3bDataDoc*)doc())->dataMode() );

  toggleAllOptions();
}


void K3bDataBurnDialog::setupSettingsTab()
{
  QWidget* frame = new QWidget( this );
  QGridLayout* frameLayout = new QGridLayout( frame );
  frameLayout->setSpacing( spacingHint() );
  frameLayout->setMargin( marginHint() );

  m_groupDataMode = new QGroupBox( 1, Qt::Vertical, i18n("Datatrack Mode"), frame );
  m_dataModeWidget = new K3bDataModeWidget( m_groupDataMode );

  QGroupBox* groupMultiSession = new QGroupBox( 1, Qt::Vertical, i18n("Multisession Mode"), frame );
  m_comboMultisession = new K3bDataMultiSessionCombobox( groupMultiSession );

  frameLayout->addWidget( m_groupDataMode, 0, 0 );
  frameLayout->addWidget( groupMultiSession, 1, 0 );
  frameLayout->setRowStretch( 2, 1 );

  addPage( frame, i18n("Settings") );
}


void K3bDataBurnDialog::slotStartClicked()
{
  if( m_checkOnlyCreateImage->isChecked() ||
      !m_checkOnTheFly->isChecked() ) {
    QFileInfo fi( m_tempDirSelectionWidget->tempPath() );
    if( fi.isDir() )
      m_tempDirSelectionWidget->setTempPath( fi.filePath() + "/image.iso" );

    if( QFile::exists( m_tempDirSelectionWidget->tempPath() ) ) {
      if( KMessageBox::warningContinueCancel( this,
				     i18n("Do you want to overwrite %1?").arg(m_tempDirSelectionWidget->tempPath()),
				     i18n("File Exists"), i18n("Overwrite") )
	  != KMessageBox::Continue )
	return;
    }
  }

  if( m_writingModeWidget->writingMode() == K3b::DAO &&
      m_comboMultisession->multiSessionMode() != K3bDataDoc::NONE &&
      m_writerSelectionWidget->writingApp() == K3b::CDRECORD )
    if( KMessageBox::warningContinueCancel( this,
					    i18n("Most writers do not support writing "
						 "multisession CDs in DAO mode.") )
	== KMessageBox::Cancel )
      return;


  K3bProjectBurnDialog::slotStartClicked();
}


void K3bDataBurnDialog::loadK3bDefaults()
{
  K3bProjectBurnDialog::loadK3bDefaults();

  m_dataModeWidget->setDataMode( K3b::DATA_MODE_AUTO );

  m_imageSettingsWidget->load( K3bIsoOptions::defaults() );
  m_advancedImageSettingsWidget->load( K3bIsoOptions::defaults() );
  m_volumeDescWidget->load( K3bIsoOptions::defaults() );
  m_comboMultisession->setMultiSessionMode( K3bDataDoc::AUTO );
  m_checkVerify->setChecked( false );

  toggleAllOptions();
}


void K3bDataBurnDialog::loadUserDefaults( KConfigBase* c )
{
  K3bProjectBurnDialog::loadUserDefaults(c);

  m_dataModeWidget->loadConfig(c);
  m_comboMultisession->loadConfig( c );

  K3bIsoOptions o = K3bIsoOptions::load( c );
  m_imageSettingsWidget->load( o );
  m_advancedImageSettingsWidget->load( o );
  m_volumeDescWidget->load( o );

  m_checkVerify->setChecked( c->readBoolEntry( "verify data", false ) );

  toggleAllOptions();
}


void K3bDataBurnDialog::saveUserDefaults( KConfigBase* c )
{
  K3bProjectBurnDialog::saveUserDefaults(c);

  m_dataModeWidget->saveConfig(c);
  m_comboMultisession->saveConfig( c );

  K3bIsoOptions o;
  m_imageSettingsWidget->save( o );
  m_advancedImageSettingsWidget->save( o );
  m_volumeDescWidget->save( o );
  o.save( c );

  c->writeEntry( "verify data", m_checkVerify->isChecked() );
}


void K3bDataBurnDialog::toggleAllOptions()
{
  K3bProjectBurnDialog::toggleAllOptions();

  // this slot may be toggled by the writerselectionwidget before all the gui is ready
  if( m_bInitializing )
    return;

  if( m_checkSimulate->isChecked() || m_checkOnlyCreateImage->isChecked() ) {
    m_checkVerify->setChecked(false);
    m_checkVerify->setEnabled(false);
  }
  else
    m_checkVerify->setEnabled(true);

  m_comboMultisession->setDisabled( m_checkOnlyCreateImage->isChecked() );
  m_dataModeWidget->setDisabled( m_checkOnlyCreateImage->isChecked() );
}


void K3bDataBurnDialog::slotMultiSessionModeChanged()
{
  if( m_comboMultisession->multiSessionMode() == K3bDataDoc::CONTINUE ||
      m_comboMultisession->multiSessionMode() == K3bDataDoc::FINISH )
    m_spinCopies->setEnabled(false);

  // wait for the proper medium
  // we have to do this in another slot than toggleAllOptions to avoid an endless loop
  if( m_comboMultisession->multiSessionMode() == K3bDataDoc::NONE )
    m_writerSelectionWidget->setWantedMediumState( K3bDevice::STATE_EMPTY );
  else if( m_comboMultisession->multiSessionMode() == K3bDataDoc::CONTINUE ||
	   m_comboMultisession->multiSessionMode() == K3bDataDoc::FINISH )
    m_writerSelectionWidget->setWantedMediumState( K3bDevice::STATE_INCOMPLETE );
  else
    m_writerSelectionWidget->setWantedMediumState( K3bDevice::STATE_EMPTY|K3bDevice::STATE_INCOMPLETE );
}


#include "k3bdataburndialog.moc"
