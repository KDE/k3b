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

#include "k3bmixedburndialog.h"
#include "k3bmixeddoc.h"

#include <k3bdataimagesettingswidget.h>
#include <k3bdataadvancedimagesettingswidget.h>
#include <k3bdatavolumedescwidget.h>
#include <k3bdatadoc.h>
#include <k3baudiodoc.h>
#include <k3bdevice.h>
#include <k3bwriterselectionwidget.h>
#include <k3btempdirselectionwidget.h>
#include <k3bisooptions.h>
#include <k3bglobals.h>
#include <k3baudiocdtextwidget.h>
#include <k3bdatamodewidget.h>
#include <k3bmsf.h>
#include <k3bstdguiitems.h>
#include <k3bwritingmodewidget.h>
#include <k3bexternalbinmanager.h>
#include <k3bversion.h>
#include <k3bcore.h>

#include <qtabwidget.h>
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
#include <qvbox.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>

#include <klocale.h>
#include <kconfig.h>
#include <kapplication.h>
#include <kdebug.h>


K3bMixedBurnDialog::K3bMixedBurnDialog( K3bMixedDoc* doc, QWidget *parent, const char *name, bool modal )
  : K3bProjectBurnDialog( doc, parent, name, modal ), m_doc(doc)
{
  prepareGui();

  setTitle( i18n("Mixed Project"), i18n("1 track (%1 minutes)",
					"%n tracks (%1 minutes)",
					m_doc->numOfTracks()).arg(m_doc->length().toString()) );

  m_checkOnlyCreateImage->hide();

  setupSettingsPage();

  // create cd-text page
  m_cdtextWidget = new K3bAudioCdTextWidget( this );
  addPage( m_cdtextWidget, i18n("CD-Text") );

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

  createContextHelp();

  QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );
  m_optionGroupLayout->addItem( spacer );

  connect( m_checkNormalize, SIGNAL(toggled(bool)), this, SLOT(toggleAllOptions()) );
  connect( m_writerSelectionWidget, SIGNAL(writingAppChanged(int)), this, SLOT(toggleAllOptions()) );
  connect( m_writingModeWidget, SIGNAL(writingModeChanged(int)), this, SLOT(toggleAllOptions()) );

  readSettings();
}


void K3bMixedBurnDialog::setupSettingsPage()
{
  QWidget* w = new QWidget( this );

  QGroupBox* groupDataMode = new QGroupBox( 1, Qt::Vertical, i18n("Datatrack Mode"), w );
  m_dataModeWidget = new K3bDataModeWidget( groupDataMode );

  QGroupBox* groupNormalize = new QGroupBox( 1, Qt::Vertical, i18n("Misc."), w );
  m_checkNormalize = K3bStdGuiItems::normalizeCheckBox( groupNormalize );

  m_groupMixedType = new QButtonGroup( 4, Qt::Vertical, i18n("Mixed Mode Type"), w );
  // standard mixed mode
  m_radioMixedTypeFirstTrack = new QRadioButton( i18n("Data in first track"), m_groupMixedType );
  // is this a standard?
  m_radioMixedTypeLastTrack = new QRadioButton( i18n("Data in last track"), m_groupMixedType );

  // Enhanced music CD/CD Extra/CD Plus format (Blue Book)
  // to fulfill the standard we also need the special file structure
  // but in the case of our simple mixed mode cd we allow to create blue book cds without
  // these special files and directories
  m_radioMixedTypeSessions = new QRadioButton( i18n("Data in second session (CD-Extra)"), m_groupMixedType );
  m_groupMixedType->setExclusive(true);

  QGridLayout* grid = new QGridLayout( w );
  grid->setMargin( marginHint() );
  grid->setSpacing( spacingHint() );
  grid->addWidget( m_groupMixedType, 0, 0 );
  grid->addWidget( groupDataMode, 1, 0 );
  grid->addWidget( groupNormalize, 2, 0 );
  grid->setRowStretch( 3, 1 );


  addPage( w, i18n("Settings") );
}


void K3bMixedBurnDialog::createContextHelp()
{
  QToolTip::add( m_radioMixedTypeFirstTrack, i18n("First track will contain the data") );
  QWhatsThis::add( m_radioMixedTypeFirstTrack, i18n("<p><b>Standard mixed mode cd 1</b>"
						    "<p>K3b will write the data track before all "
						    "audio tracks."
						    "<p>This mode should only be used for cds that are unlikely to "
						    "be played on a hifi audio cd player."
						    "<p><b>Caution:</b> It could lead to problems with some older "
						    "hifi audio cd player that try to play the data track.") );

  QToolTip::add( m_radioMixedTypeLastTrack, i18n("Last track will contain the data") );
  QWhatsThis::add( m_radioMixedTypeLastTrack, i18n("<p><b>Standard mixed mode cd 2</b>"
						   "<p>K3b will write the data track after all "
						   "audio tracks."
						    "<p>This mode should only be used for cds that are unlikely to "
						    "be played on a hifi audio cd player."
						   "<p><b>Caution:</b> It could lead to problems with some older "
						   "hifi audio cd player that try to play the data track.") );

  QToolTip::add( m_radioMixedTypeSessions, i18n("The data will be written in a second session") );
  QWhatsThis::add( m_radioMixedTypeSessions, i18n("<p><b>Blue book cd</b>"
						  "<p>K3b will create a multisession cd with "
						  "2 sessions. The first session will contain all "
						  "audio tracks and the second session will contain "
						  "a mode 2 form 1 data track."
						  "<p>This mode is based on the <em>Blue book</em> "
						  "standard (also known as <em>Extended Audio CD</em>, "
						  "<em>CD-Extra</em>, or <em>CD Plus</em>) "
						  "and has the advantage that a hifi audio "
						  "cd player will just recognize the first session "
						  "and ignore the second session with the data track."
						  "<p>If the cd is intended to be used in a hifi audio cd player "
						  "this is the recommended mode."
						  "<p>Some older CD-ROMs may have problems reading "
						  "a blue book cd since it's a multisession cd.") );
}


void K3bMixedBurnDialog::saveSettings()
{
  K3bProjectBurnDialog::saveSettings();

  if( m_groupMixedType->selected() == m_radioMixedTypeLastTrack )
    m_doc->setMixedType( K3bMixedDoc::DATA_LAST_TRACK );
  else if( m_groupMixedType->selected() == m_radioMixedTypeSessions )
    m_doc->setMixedType( K3bMixedDoc::DATA_SECOND_SESSION );
  else
    m_doc->setMixedType( K3bMixedDoc::DATA_FIRST_TRACK );

  m_cdtextWidget->save( m_doc->audioDoc() );

  m_doc->audioDoc()->setNormalize( m_checkNormalize->isChecked() );

  // save iso image settings
  m_imageSettingsWidget->save( m_doc->dataDoc()->isoOptions() );
  m_advancedImageSettingsWidget->save( m_doc->dataDoc()->isoOptions() );
  m_volumeDescWidget->save( m_doc->dataDoc()->isoOptions() );

  m_doc->dataDoc()->setDataMode( m_dataModeWidget->dataMode() );

  // save image file path
  m_doc->setTempDir( m_tempDirSelectionWidget->tempPath() );
}


void K3bMixedBurnDialog::readSettings()
{
  K3bProjectBurnDialog::readSettings();

  m_checkNormalize->setChecked( m_doc->audioDoc()->normalize() );

  if( !m_doc->tempDir().isEmpty() )
    m_tempDirSelectionWidget->setTempPath( m_doc->tempDir() );

  switch( m_doc->mixedType() ) {
  case K3bMixedDoc::DATA_FIRST_TRACK:
    m_radioMixedTypeFirstTrack->setChecked(true);
    break;
  case K3bMixedDoc::DATA_LAST_TRACK:
    m_radioMixedTypeLastTrack->setChecked(true);
    break;
  case K3bMixedDoc::DATA_SECOND_SESSION:
    m_radioMixedTypeSessions->setChecked(true);
    break;
  }

  m_cdtextWidget->load( m_doc->audioDoc() );

  m_imageSettingsWidget->load( m_doc->dataDoc()->isoOptions() );
  m_advancedImageSettingsWidget->load( m_doc->dataDoc()->isoOptions() );
  m_volumeDescWidget->load( m_doc->dataDoc()->isoOptions() );

  m_dataModeWidget->setDataMode( m_doc->dataDoc()->dataMode() );

  toggleAllOptions();
}


void K3bMixedBurnDialog::slotLoadK3bDefaults()
{
  K3bProjectBurnDialog::slotLoadK3bDefaults();

  m_cdtextWidget->setChecked( false );
  m_checkNormalize->setChecked(false);

  m_radioMixedTypeFirstTrack->setChecked(true);

  m_dataModeWidget->setDataMode( K3b::DATA_MODE_AUTO );

  m_imageSettingsWidget->load( K3bIsoOptions::defaults() );
  m_advancedImageSettingsWidget->load( K3bIsoOptions::defaults() );
  m_volumeDescWidget->load( K3bIsoOptions::defaults() );

  toggleAllOptions();
}


void K3bMixedBurnDialog::slotLoadUserDefaults()
{
  K3bProjectBurnDialog::slotLoadUserDefaults();

  KConfig* c = kapp->config();

  m_cdtextWidget->setChecked( c->readBoolEntry( "cd_text", false ) );
  m_checkNormalize->setChecked( c->readBoolEntry( "normalize", false ) );

  // load mixed type
  if( c->readEntry( "mixed_type" ) == "last_track" )
    m_radioMixedTypeLastTrack->setChecked(true);
  else if( c->readEntry( "mixed_type" ) == "second_session" )
    m_radioMixedTypeSessions->setChecked(true);
  else
    m_radioMixedTypeFirstTrack->setChecked(true);

  m_dataModeWidget->loadConfig(c);

  K3bIsoOptions o = K3bIsoOptions::load( c );
  m_imageSettingsWidget->load( o );
  m_advancedImageSettingsWidget->load( o );
  m_volumeDescWidget->load( o );

  toggleAllOptions();
}


void K3bMixedBurnDialog::slotSaveUserDefaults()
{
  K3bProjectBurnDialog::slotSaveUserDefaults();

  KConfig* c = kapp->config();

  c->writeEntry( "cd_text", m_cdtextWidget->isChecked() );
  c->writeEntry( "normalize", m_checkNormalize->isChecked() );

  // save mixed type
  if( m_groupMixedType->selected() == m_radioMixedTypeLastTrack )
   c->writeEntry( "mixed_type", "last_track" );
  else if( m_groupMixedType->selected() == m_radioMixedTypeSessions )
   c->writeEntry( "mixed_type", "second_session" );
  else
    c->writeEntry( "mixed_type", "first_track" );

  m_dataModeWidget->saveConfig(c);

  K3bIsoOptions o;
  m_imageSettingsWidget->save( o );
  m_advancedImageSettingsWidget->save( o );
  m_volumeDescWidget->save( o );
  o.save( c );

  if( m_tempDirSelectionWidget->isEnabled() ) {
    m_tempDirSelectionWidget->saveConfig();
  }
}


void K3bMixedBurnDialog::toggleAllOptions()
{
    K3bProjectBurnDialog::toggleAllOptions();

  bool cdrecordOnTheFly = false;
  bool cdrecordCdText = false;
  if ( k3bcore->externalBinManager()->binObject("cdrecord") ) {
    cdrecordOnTheFly = k3bcore->externalBinManager()->binObject("cdrecord")->version
      >= K3bVersion( 2, 1, -1, "a13" );
    cdrecordCdText = k3bcore->externalBinManager()->binObject("cdrecord")->hasFeature( "cdtext" );
  }

  if( m_writingModeWidget->writingMode() == K3b::TAO ||
      m_writingModeWidget->writingMode() == K3b::RAW ||
      m_writerSelectionWidget->writingApp() == K3b::CDRECORD ) {
    m_checkOnTheFly->setEnabled( cdrecordOnTheFly && !m_checkNormalize->isChecked() );
    if( !cdrecordOnTheFly || m_checkNormalize->isChecked() )
      m_checkOnTheFly->setChecked( false );

    m_cdtextWidget->setEnabled( cdrecordCdText );
    if( !cdrecordCdText )
      m_cdtextWidget->setChecked( false );
  }
  else {
    m_checkOnTheFly->setEnabled( !m_checkNormalize->isChecked() );
    if( m_checkNormalize->isChecked() )
      m_checkOnTheFly->setChecked( false );
    m_cdtextWidget->setEnabled( true );
  }

  // we are not able to normalize in on-the-fly mode
  m_checkNormalize->setDisabled( m_checkOnTheFly->isChecked() );
}


#include "k3bmixedburndialog.moc"

