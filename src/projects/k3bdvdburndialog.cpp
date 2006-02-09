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

#include "k3bdvdburndialog.h"
#include "k3bdvddoc.h"
#include "k3bdatamultisessioncombobox.h"

#include <k3bdevice.h>
#include <k3bwriterselectionwidget.h>
#include <k3btempdirselectionwidget.h>
#include <k3bcore.h>
#include <k3bwritingmodewidget.h>
#include <k3bglobals.h>
#include <k3bdataimagesettingswidget.h>
#include <k3bdataadvancedimagesettingswidget.h>
#include <k3bdatavolumedescwidget.h>
#include <k3bisooptions.h>
#include <k3bstdguiitems.h>

#include <kconfig.h>
#include <klocale.h>
#include <kio/global.h>
#include <kmessagebox.h>

#include <qlayout.h>
#include <qcheckbox.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qspinbox.h>


K3bDvdBurnDialog::K3bDvdBurnDialog( K3bDvdDoc* doc, QWidget *parent, const char *name, bool modal )
  : K3bProjectBurnDialog( doc, parent, name, modal, true ),
    m_doc( doc )
{
  prepareGui();

  setTitle( i18n("DVD Project"), i18n("Size: %1").arg( KIO::convertSize(doc->size()) ) );

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

  m_tempDirSelectionWidget->setSelectionMode( K3bTempDirSelectionWidget::FILE );

  m_writerSelectionWidget->setWantedMediumState( K3bDevice::STATE_EMPTY|K3bDevice::STATE_INCOMPLETE );

  readSettings();

  QString path = m_doc->tempDir();
  if( path.isEmpty() ) {
    path = K3b::defaultTempPath();
    if( m_doc->isoOptions().volumeID().isEmpty() )
      path.append( "image.iso" );
    else
      path.append( m_doc->isoOptions().volumeID() + ".iso" );
  }
  m_tempDirSelectionWidget->setTempPath( path );
}


K3bDvdBurnDialog::~K3bDvdBurnDialog()
{
}


void K3bDvdBurnDialog::setupSettingsTab()
{
  QWidget* frame = new QWidget( this );
  QGridLayout* frameLayout = new QGridLayout( frame );
  frameLayout->setSpacing( spacingHint() );
  frameLayout->setMargin( marginHint() );

  // Multisession
  // ////////////////////////////////////////////////////////////////////////
  QGroupBox* groupMultiSession = new QGroupBox( 2, Qt::Vertical, i18n("Multisession Mode"), frame );
  (void)new QLabel( i18n("<p>For DVD-R(W) in sequential incremental mode and "
			 "DVD+R it behaves like multisession CDs.<br> "
			 "<b>Caution:</b> Most DVD-ROM drives are only able "
			 "to read the first session and most likely even on "
			 "DVD writers only the first session will be mounted.</p>"
			 "<p>For DVD+RW and DVD-RW in restricted overwrite mode "
			 "K3b will utilize growisofs to <em>grow</em> the ISO9660 "
			 "filesystem in the first session.</p>"
			 "<p><b>Be aware that K3b needs to write multisession "
			 "DVDs on-the-fly.</b>"), groupMultiSession );
  m_comboMultisession = new K3bDataMultiSessionCombobox( groupMultiSession );

  frameLayout->addWidget( groupMultiSession, 0, 0 );
  frameLayout->setRowStretch( 1, 1 );

  addPage( frame, i18n("Settings") );

  connect( m_comboMultisession, SIGNAL(activated(int)),
	   this, SLOT(slotMultiSessionModeChanged()) );
}


void K3bDvdBurnDialog::saveSettings()
{
  K3bProjectBurnDialog::saveSettings();

  // save iso image settings
  K3bIsoOptions o = m_doc->isoOptions();
  m_imageSettingsWidget->save( o );
  m_advancedImageSettingsWidget->save( o );
  m_volumeDescWidget->save( o );
  m_doc->setIsoOptions( o );

  // save image file path
  m_doc->setTempDir( m_tempDirSelectionWidget->tempPath() );

  // save multisession settings
  m_doc->setMultiSessionMode( m_comboMultisession->multiSessionMode() );

  m_doc->setVerifyData( m_checkVerify->isChecked() );
}


void K3bDvdBurnDialog::readSettings()
{
  K3bProjectBurnDialog::readSettings();

  // read multisession
  m_comboMultisession->setMultiSessionMode( m_doc->multiSessionMode() );

  if( !doc()->tempDir().isEmpty() )
    m_tempDirSelectionWidget->setTempPath( doc()->tempDir() );

  m_checkVerify->setChecked( m_doc->verifyData() );

  m_imageSettingsWidget->load( m_doc->isoOptions() );
  m_advancedImageSettingsWidget->load( m_doc->isoOptions() );
  m_volumeDescWidget->load( m_doc->isoOptions() );

  // for now we do not support dual layer multisession (growisofs does not handle layer jump yet)
  if( doc()->size() > 4700372992LL )
    m_writerSelectionWidget->setWantedMediumType( K3bDevice::MEDIA_WRITABLE_DVD_DL );
  else
    m_writerSelectionWidget->setWantedMediumType( K3bDevice::MEDIA_WRITABLE_DVD_SL );

  toggleAllOptions();
}


void K3bDvdBurnDialog::toggleAllOptions()
{
  K3bProjectBurnDialog::toggleAllOptions();

  // Multisession in DAO is not possible
  if( m_writingModeWidget->writingMode() == K3b::DAO ) {
    if( m_comboMultisession->multiSessionMode() == K3bDataDoc::START ||
	m_comboMultisession->multiSessionMode() == K3bDataDoc::CONTINUE ||
	m_comboMultisession->multiSessionMode() == K3bDataDoc::FINISH )
      KMessageBox::information( this, i18n("It is not possible to write multisession DVDs in DAO mode."
					   "Multisession has been disabled."),
				i18n("DVD multisession"),
				"dvd_multisession_no_dao" );

    m_comboMultisession->setEnabled(false);
  }
  else {
//     // for some reason I don't know yet when writing multisession volume set size needs to be 1
//     if( m_comboMultisession->multiSessionMode() != K3bDataDoc::NONE ) {
//       m_volumeDescWidget->m_spinVolumeSetSize->setValue( 1 );
//       m_volumeDescWidget->m_spinVolumeSetSize->setEnabled( false );
//     }
//     else {
//       m_volumeDescWidget->m_spinVolumeSetSize->setEnabled( true );
//     }

    m_comboMultisession->setEnabled(true);

//     if( !m_checkOnTheFly->isChecked() ) {
//       // no continue and finish multisession in non-the-fly mode since
//       // we can only continue ms with growisofsimager
//       if( m_comboMultisession->multiSessionMode() == K3bDataDoc::START ||
// 	  m_comboMultisession->multiSessionMode() == K3bDataDoc::FINISH ||
// 	  m_comboMultisession->multiSessionMode() == K3bDataDoc::CONTINUE ) {
// 	KMessageBox::information( this, i18n("K3b does only support writing multisession DVDs on-the-fly. "
// 					     "Multisession has been disabled."),
// 				  i18n("DVD Multisession"),
// 				  "dvd_multisession_only_on_the_fly" );
//       }
//       m_comboMultisession->setForceNoMultisession( true );
//     }
//     else {
//       m_comboMultisession->setForceNoMultisession( false );
//     }
  }

  if( m_checkSimulate->isChecked() || m_checkOnlyCreateImage->isChecked() ) {
    m_checkVerify->setChecked(false);
    m_checkVerify->setEnabled(false);
  }
  else
    m_checkVerify->setEnabled(true);
}


void K3bDvdBurnDialog::slotMultiSessionModeChanged()
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


void K3bDvdBurnDialog::loadK3bDefaults()
{
  K3bProjectBurnDialog::loadK3bDefaults();

  m_imageSettingsWidget->load( K3bIsoOptions::defaults() );
  m_advancedImageSettingsWidget->load( K3bIsoOptions::defaults() );
  m_volumeDescWidget->load( K3bIsoOptions::defaults() );
  m_checkVerify->setChecked( false );

  m_comboMultisession->setMultiSessionMode( K3bDataDoc::AUTO );

  toggleAllOptions();
}


void K3bDvdBurnDialog::loadUserDefaults( KConfigBase* c )
{
  K3bProjectBurnDialog::loadUserDefaults(c);

  K3bIsoOptions o = K3bIsoOptions::load( c );
  m_imageSettingsWidget->load( o );
  m_advancedImageSettingsWidget->load( o );
  m_volumeDescWidget->load( o );

  m_comboMultisession->loadConfig( c );

  m_checkVerify->setChecked( c->readBoolEntry( "verify data", false ) );

  toggleAllOptions();
}


void K3bDvdBurnDialog::saveUserDefaults( KConfigBase* c )
{
  K3bProjectBurnDialog::saveUserDefaults(c);

  K3bIsoOptions o;
  m_imageSettingsWidget->save( o );
  m_advancedImageSettingsWidget->save( o );
  m_volumeDescWidget->save( o );
  o.save( c );

  m_comboMultisession->saveConfig( c );

  c->writeEntry( "verify data", m_checkVerify->isChecked() );
}

#include "k3bdvdburndialog.moc"
