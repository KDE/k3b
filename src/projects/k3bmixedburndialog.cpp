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

#include "k3bmixedburndialog.h"
#include "k3bmixeddoc.h"
#include "k3bmixedview.h"

#include <k3bdataimagesettingswidget.h>
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
#include <k3baudiotrackplayer.h>

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
#include <kmessagebox.h>


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

  createContextHelp();

  QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );
  m_optionGroupLayout->addItem( spacer );

  connect( m_checkNormalize, SIGNAL(toggled(bool)), this, SLOT(slotNormalizeToggled(bool)) );
  connect( m_checkOnTheFly, SIGNAL(toggled(bool)), this, SLOT(slotOnTheFlyToggled(bool)) );
  connect( m_writerSelectionWidget, SIGNAL(writingAppChanged(int)), this, SLOT(slotToggleAll()) );
  connect( m_writingModeWidget, SIGNAL(writingModeChanged(int)), this, SLOT(slotToggleAll()) );

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

  // Enhanced music CD/CD Extra/CD Plus format (Blue Book)
  // to fulfill the standard we also need the special file structure
  // but in the case of our simple mixed mode cd we allow to create blue book cds without
  // these special files and directories
  m_radioMixedTypeSessions = new QRadioButton( i18n("Data in second session (CD-Extra)"), m_groupMixedType );

  // standard mixed mode
  m_radioMixedTypeFirstTrack = new QRadioButton( i18n("Data in first track"), m_groupMixedType );

  // is this a standard?
  m_radioMixedTypeLastTrack = new QRadioButton( i18n("Data in last track"), m_groupMixedType );

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
  QWhatsThis::add( m_radioMixedTypeFirstTrack, i18n("<p><b>Standard mixed mode CD 1</b>"
						    "<p>K3b will write the data track before all "
						    "audio tracks."
						    "<p>This mode should only be used for CDs that are unlikely to "
						    "be played on a hifi audio CD player."
						    "<p><b>Caution:</b> It could lead to problems with some older "
						    "hifi audio CD players that try to play the data track.") );

  QToolTip::add( m_radioMixedTypeLastTrack, i18n("Last track will contain the data") );
  QWhatsThis::add( m_radioMixedTypeLastTrack, i18n("<p><b>Standard mixed mode CD 2</b>"
						   "<p>K3b will write the data track after all "
						   "audio tracks."
						    "<p>This mode should only be used for CDs that are unlikely to "
						    "be played on a hifi audio CD player."
						   "<p><b>Caution:</b> It could lead to problems with some older "
						   "hifi audio CD players that try to play the data track.") );

  QToolTip::add( m_radioMixedTypeSessions, i18n("The data will be written in a second session") );
  QWhatsThis::add( m_radioMixedTypeSessions, i18n("<p><b>Blue book CD</b>"
						  "<p>K3b will create a multisession CD with "
						  "2 sessions. The first session will contain all "
						  "audio tracks and the second session will contain "
						  "a mode 2 form 1 data track."
						  "<p>This mode is based on the <em>Blue book</em> "
						  "standard (also known as <em>Extended Audio CD</em>, "
						  "<em>CD-Extra</em>, or <em>CD Plus</em>) "
						  "and has the advantage that a hifi audio "
						  "CD player will only recognize the first session "
						  "and ignore the second session with the data track."
						  "<p>If the CD is intended to be used in a hifi audio CD player "
						  "this is the recommended mode."
						  "<p>Some older CD-ROMs may have problems reading "
						  "a blue book CD since it is a multisession CD.") );
}


void K3bMixedBurnDialog::slotStartClicked()
{
  // FIXME: this should not be done via the doc. So remove all gui stuff from the doc
  static_cast<K3bMixedView*>(m_doc->view())->player()->stop();
  K3bProjectBurnDialog::slotStartClicked();
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
  K3bIsoOptions o = m_doc->dataDoc()->isoOptions();
  m_imageSettingsWidget->save( o );
  m_volumeDescWidget->save( o );
  m_doc->dataDoc()->setIsoOptions( o );

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
  m_volumeDescWidget->load( m_doc->dataDoc()->isoOptions() );

  m_dataModeWidget->setDataMode( m_doc->dataDoc()->dataMode() );

  toggleAll();
}


void K3bMixedBurnDialog::loadK3bDefaults()
{
  K3bProjectBurnDialog::loadK3bDefaults();

  m_cdtextWidget->setChecked( false );
  m_checkNormalize->setChecked(false);

  m_radioMixedTypeSessions->setChecked(true);

  m_dataModeWidget->setDataMode( K3b::DATA_MODE_AUTO );

  m_imageSettingsWidget->load( K3bIsoOptions::defaults() );
  m_volumeDescWidget->load( K3bIsoOptions::defaults() );

  toggleAll();
}


void K3bMixedBurnDialog::loadUserDefaults( KConfigBase* c )
{
  K3bProjectBurnDialog::loadUserDefaults( c );

  m_cdtextWidget->setChecked( c->readBoolEntry( "cd_text", false ) );
  m_checkNormalize->setChecked( c->readBoolEntry( "normalize", false ) );

  // load mixed type
  if( c->readEntry( "mixed_type" ) == "last_track" )
    m_radioMixedTypeLastTrack->setChecked(true);
  else if( c->readEntry( "mixed_type" ) == "first_track" )
    m_radioMixedTypeFirstTrack->setChecked(true);
  else
    m_radioMixedTypeSessions->setChecked(true);

  m_dataModeWidget->loadConfig(c);

  K3bIsoOptions o = K3bIsoOptions::load( c );
  m_imageSettingsWidget->load( o );
  m_volumeDescWidget->load( o );

  toggleAll();
}


void K3bMixedBurnDialog::saveUserDefaults( KConfigBase* c )
{
  K3bProjectBurnDialog::saveUserDefaults(c);

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
  m_volumeDescWidget->save( o );
  o.save( c );

  if( m_tempDirSelectionWidget->isEnabled() ) {
    m_tempDirSelectionWidget->saveConfig();
  }
}


void K3bMixedBurnDialog::toggleAll()
{
  K3bProjectBurnDialog::toggleAll();

  bool cdrecordOnTheFly = false;
  bool cdrecordCdText = false;
  if ( k3bcore->externalBinManager()->binObject("cdrecord") ) {
    cdrecordOnTheFly = k3bcore->externalBinManager()->binObject("cdrecord")->version
      >= K3bVersion( 2, 1, -1, "a13" );
    cdrecordCdText = k3bcore->externalBinManager()->binObject("cdrecord")->hasFeature( "cdtext" );
  }

  // cdrdao always knows onthefly and cdtext
  bool onTheFly = true;
  bool cdText = true;
  if( m_writingModeWidget->writingMode() == K3b::TAO ||
      m_writingModeWidget->writingMode() == K3b::RAW ||
      m_writerSelectionWidget->writingApp() == K3b::CDRECORD ) {
    onTheFly = cdrecordOnTheFly;
    cdText = cdrecordCdText;
  }

  m_checkOnTheFly->setEnabled( !m_checkOnlyCreateImage->isChecked() && 
			       onTheFly );
  if( !onTheFly )
    m_checkOnTheFly->setChecked( false );

  m_cdtextWidget->setEnabled( !m_checkOnlyCreateImage->isChecked() &&
			      cdText && 
			      m_writingModeWidget->writingMode() != K3b::TAO );
  if( !cdText || m_writingModeWidget->writingMode() == K3b::TAO  )
    m_cdtextWidget->setChecked( false );
}


void K3bMixedBurnDialog::slotNormalizeToggled( bool on )
{
  if( on ) {
    // we are not able to normalize in on-the-fly mode
    if( !k3bcore->externalBinManager()->foundBin( "normalize" ) ) {
      KMessageBox::sorry( this, i18n("<p><b>External program <em>normalize</em> is not installed.</b>"
				     "<p>K3b uses <em>normalize</em> (http://www1.cs.columbia.edu/~cvaill/normalize/) "
				     "to normalize audio tracks. In order to "
				     "use this functionality, please install it first.") );
      m_checkNormalize->setChecked( false );
    }
    else if( m_checkOnTheFly->isChecked() && !m_checkOnlyCreateImage->isChecked() ) {
      if( KMessageBox::warningYesNo( this, i18n("<p>K3b is not able to normalize audio tracks when burning on-the-fly. "
						"The external program used for this task only supports normalizing a set "
						"of audio files."),
				     QString::null,
				     i18n("Disable normalization"),
				     i18n("Disable on-the-fly burning"),
				     "audioProjectNormalizeOrOnTheFly" ) == KMessageBox::Yes )
	m_checkNormalize->setChecked( false );
      else
	m_checkOnTheFly->setChecked( false );
    }
  }
}


void K3bMixedBurnDialog::slotOnTheFlyToggled( bool on )
{
  if( on ) {
    if( m_checkNormalize->isChecked() ) {
      if( KMessageBox::warningYesNo( this, i18n("<p>K3b is not able to normalize audio tracks when burning on-the-fly. "
						"The external program used for this task only supports normalizing a set "
						"of audio files."),
				     QString::null,
				     i18n("Disable normalization"),
				     i18n("Disable on-the-fly burning"),
				     "audioProjectNormalizeOrOnTheFly" ) == KMessageBox::Yes )
	m_checkNormalize->setChecked( false );
      else
	m_checkOnTheFly->setChecked( false );
    }
  }
}


#include "k3bmixedburndialog.moc"

