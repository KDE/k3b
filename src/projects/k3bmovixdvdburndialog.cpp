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


#include "k3bmovixdvdburndialog.h"
#include "k3bmovixdvddoc.h"

#include <k3bmovixprogram.h>
#include <k3bmovixoptionswidget.h>

#include <k3bdataimagesettingswidget.h>
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
#include <kmessagebox.h>
#include <kio/global.h>
#include <kapplication.h>
#include <kconfig.h>
#include <kdebug.h>

#include <qcheckbox.h>
#include <qlayout.h>
#include <qgroupbox.h>
#include <qfileinfo.h>


K3bMovixDvdBurnDialog::K3bMovixDvdBurnDialog( K3bMovixDvdDoc* doc, QWidget* parent, const char* name, bool modal )
  : K3bProjectBurnDialog( doc, parent, name, modal, true ),
    m_doc(doc)
{
  prepareGui();

  m_tempDirSelectionWidget->setSelectionMode( K3bTempDirSelectionWidget::FILE );

  setTitle( i18n("eMovix DVD Project"),
	    i18n("1 file (%1)", "%n files (%1)", m_doc->movixFileItems().count()).arg(KIO::convertSize(m_doc->size())) );

  m_movixOptionsWidget = new K3bMovixOptionsWidget( this );
  addPage( m_movixOptionsWidget, i18n("eMovix") );

  // create image settings tab
  m_imageSettingsWidget = new K3bDataImageSettingsWidget( this );
  addPage( m_imageSettingsWidget, i18n("Filesystem") );

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


K3bMovixDvdBurnDialog::~K3bMovixDvdBurnDialog()
{
}


void K3bMovixDvdBurnDialog::loadK3bDefaults()
{
  K3bProjectBurnDialog::loadK3bDefaults();

  m_imageSettingsWidget->load( K3bIsoOptions::defaults() );

  m_movixOptionsWidget->loadDefaults();

  m_checkVerify->setChecked( false );

  toggleAll();
}


void K3bMovixDvdBurnDialog::loadUserDefaults( KConfigBase* c )
{
  K3bProjectBurnDialog::loadUserDefaults(c);

  K3bIsoOptions o = K3bIsoOptions::load( c );
  m_imageSettingsWidget->load( o );

  m_movixOptionsWidget->loadConfig(c);

  m_checkVerify->setChecked( c->readBoolEntry( "verify data", false ) );

  toggleAll();
}


void K3bMovixDvdBurnDialog::saveUserDefaults( KConfigBase* c )
{
  K3bProjectBurnDialog::saveUserDefaults(c);

  K3bIsoOptions o;
  m_imageSettingsWidget->save( o );
  o.save( c );

  c->writeEntry( "verify data", m_checkVerify->isChecked() );

  m_movixOptionsWidget->saveConfig(c);
}


void K3bMovixDvdBurnDialog::saveSettings()
{
  K3bProjectBurnDialog::saveSettings();

  m_movixOptionsWidget->saveSettings( m_doc );

  // save iso image settings
  K3bIsoOptions o = m_doc->isoOptions();
  m_imageSettingsWidget->save( o );
  m_doc->setIsoOptions( o );

  m_doc->setVerifyData( m_checkVerify->isChecked() );

  // save image file path
  m_doc->setTempDir( m_tempDirSelectionWidget->tempPath() );
}


void K3bMovixDvdBurnDialog::readSettings()
{
  K3bProjectBurnDialog::readSettings();

  m_imageSettingsWidget->load( m_doc->isoOptions() );

  m_checkVerify->setChecked( m_doc->verifyData() );

  if( !doc()->tempDir().isEmpty() )
    m_tempDirSelectionWidget->setTempPath( doc()->tempDir() );
  else
    m_tempDirSelectionWidget->setTempPath( K3b::defaultTempPath() + doc()->name() + ".iso" );

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

  if( doc()->size() > 4700372992LL )
    m_writerSelectionWidget->setWantedMediumType( K3bDevice::MEDIA_WRITABLE_DVD_DL );
  else
    m_writerSelectionWidget->setWantedMediumType( K3bDevice::MEDIA_WRITABLE_DVD );
}


void K3bMovixDvdBurnDialog::slotStartClicked()
{
  if( m_checkOnlyCreateImage->isChecked() ||
      m_checkCacheImage->isChecked() ) {
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

  K3bProjectBurnDialog::slotStartClicked();
}


void K3bMovixDvdBurnDialog::toggleAll()
{
  K3bProjectBurnDialog::toggleAll();

  if( m_checkSimulate->isChecked() || m_checkOnlyCreateImage->isChecked() ) {
    m_checkVerify->setChecked(false);
    m_checkVerify->setEnabled(false);
  }
  else
    m_checkVerify->setEnabled(true);
}

#include "k3bmovixdvdburndialog.moc"
