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


#include "k3baudioburndialog.h"
#include "k3baudioview.h"
#include "k3baudiotrackplayer.h"
#include <k3bcore.h>
#include "k3baudiodoc.h"
#include <k3bdevice.h>
#include <k3bwriterselectionwidget.h>
#include <k3btempdirselectionwidget.h>
#include "k3baudiocdtextwidget.h"
#include <k3bglobals.h>
#include <k3bstdguiitems.h>
#include <k3bwritingmodewidget.h>
#include <k3bexternalbinmanager.h>

#include <qcheckbox.h>
#include <qcombobox.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qtabwidget.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qgrid.h>
#include <qtoolbutton.h>
#include <qptrlist.h>
#include <qstringlist.h>
#include <qpoint.h>

#include <klocale.h>
#include <kstandarddirs.h>
#include <kconfig.h>
#include <kmessagebox.h>


K3bAudioBurnDialog::K3bAudioBurnDialog(K3bAudioDoc* _doc, QWidget *parent, const char *name, bool modal )
  : K3bProjectBurnDialog( _doc, parent, name, modal ),
    m_doc(_doc)
{
  prepareGui();

  setTitle( i18n("Audio Project"),
	    i18n("1 track (%1 minutes)", "%n tracks (%1 minutes)",
		 m_doc->numOfTracks() ).arg(m_doc->length().toString()) );

  QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );
  m_optionGroupLayout->addItem( spacer );

  // create cd-text page
  m_cdtextWidget = new K3bAudioCdTextWidget( this );
  addPage( m_cdtextWidget, i18n("CD-Text") );

  // create advanced tab
  QWidget* advancedTab = new QWidget( this );
  QGridLayout* advancedTabGrid = new QGridLayout( advancedTab );
  advancedTabGrid->setSpacing( spacingHint() );
  advancedTabGrid->setMargin( marginHint() );

  QGroupBox* advancedSettingsGroup = new QGroupBox( 1, Qt::Vertical, i18n("Settings"), advancedTab );
  m_checkNormalize = K3bStdGuiItems::normalizeCheckBox( advancedSettingsGroup );

  QGroupBox* advancedGimmickGroup = new QGroupBox( 1, Qt::Vertical, i18n("Gimmicks"), advancedTab );
  m_checkHideFirstTrack = new QCheckBox( i18n( "Hide first track" ), advancedGimmickGroup, "m_checkHideFirstTrack" );

  advancedTabGrid->addWidget( advancedSettingsGroup, 0, 0 );
  advancedTabGrid->addWidget( advancedGimmickGroup, 1, 0 );
  advancedTabGrid->setRowStretch( 2, 1 );

  addPage( advancedTab, i18n("Advanced") );

  connect( m_writerSelectionWidget, SIGNAL(writingAppChanged(int)), this, SLOT(toggleAllOptions()) );
  connect( m_checkNormalize, SIGNAL(toggled(bool)), this, SLOT(toggleAllOptions()) );
  connect( m_writingModeWidget, SIGNAL(writingModeChanged(int)), this, SLOT(toggleAllOptions()) );

  readSettings();

  // ToolTips
  // -------------------------------------------------------------------------
  QToolTip::add( m_checkHideFirstTrack, i18n("Hide the first track in the first pregap") );

  // What's This info
  // -------------------------------------------------------------------------
  QWhatsThis::add( m_checkHideFirstTrack,
		   i18n("<p>If this option is checked K3b will <em>hide</em> the first track."
			"<p>The audio CD standard uses pregaps before every track on the CD. "
			"By default these last for 2 seconds and are silent. In DAO mode it "
			"is possible to have longer pregaps that contain some audio. In this case "
			"the first pregap will contain the complete first track."
			"<p>You will need to seek back from the beginning of the CD to listen to "
			"the first track. Try it, it is quite amusing."
			"<p><b>This feature is only available in DAO mode when writing with cdrdao.") );
}

K3bAudioBurnDialog::~K3bAudioBurnDialog(){
}


void K3bAudioBurnDialog::slotStartClicked()
{
  // FIXME: this should not be done via the doc. So remove all gui stuff from the doc
  static_cast<K3bAudioView*>(m_doc->view())->player()->stop();
  K3bProjectBurnDialog::slotStartClicked();
}


void K3bAudioBurnDialog::saveSettings()
{
  K3bProjectBurnDialog::saveSettings();

  m_doc->setTempDir( m_tempDirSelectionWidget->tempPath() );
  m_doc->setHideFirstTrack( m_checkHideFirstTrack->isChecked() );
  m_doc->setNormalize( m_checkNormalize->isChecked() );

  // -- save Cd-Text ------------------------------------------------
  m_cdtextWidget->save( m_doc );

  doc()->setTempDir( m_tempDirSelectionWidget->tempPath() );
}


void K3bAudioBurnDialog::readSettings()
{
  K3bProjectBurnDialog::readSettings();

  m_checkHideFirstTrack->setChecked( m_doc->hideFirstTrack() );
  m_checkNormalize->setChecked( m_doc->normalize() );

  // read CD-Text ------------------------------------------------------------
  m_cdtextWidget->load( m_doc );

  if( !doc()->tempDir().isEmpty() )
    m_tempDirSelectionWidget->setTempPath( doc()->tempDir() );

  toggleAllOptions();
}


void K3bAudioBurnDialog::loadK3bDefaults()
{
  K3bProjectBurnDialog::loadK3bDefaults();

  m_cdtextWidget->setChecked( true );
  m_checkHideFirstTrack->setChecked( false );
  m_checkNormalize->setChecked(false);

  toggleAllOptions();
}


void K3bAudioBurnDialog::loadUserDefaults( KConfig* c )
{
  K3bProjectBurnDialog::loadUserDefaults( c );

  m_cdtextWidget->setChecked( c->readBoolEntry( "cd_text", true ) );
  m_checkHideFirstTrack->setChecked( c->readBoolEntry( "hide_first_track", false ) );
  m_checkNormalize->setChecked( c->readBoolEntry( "normalize", false ) );

  toggleAllOptions();
}


void K3bAudioBurnDialog::saveUserDefaults( KConfig* c )
{
  K3bProjectBurnDialog::saveUserDefaults( c );

  c->writeEntry( "cd_text", m_cdtextWidget->isChecked() );
  c->writeEntry( "hide_first_track", m_checkHideFirstTrack->isChecked() );
  c->writeEntry( "normalize", m_checkNormalize->isChecked() );
}

void K3bAudioBurnDialog::toggleAllOptions()
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
    m_checkHideFirstTrack->setChecked(false);
    m_checkHideFirstTrack->setEnabled(false);
    m_cdtextWidget->setEnabled( cdrecordCdText && m_writingModeWidget->writingMode() != K3b::TAO );
    if( !cdrecordCdText || m_writingModeWidget->writingMode() == K3b::TAO )
      m_cdtextWidget->setChecked(false);
  }
  else {
    m_checkOnTheFly->setEnabled( !m_checkOnlyCreateImage->isChecked() && !m_checkNormalize->isChecked() );
    if( m_checkNormalize->isChecked() )
      m_checkOnTheFly->setChecked( false );
    m_checkHideFirstTrack->setEnabled( !m_checkOnlyCreateImage->isChecked() );
    m_cdtextWidget->setEnabled(true);
  }

  // we are not able to normalize in on-the-fly mode
  m_checkNormalize->setDisabled( m_checkOnTheFly->isChecked() && !m_checkOnlyCreateImage->isChecked() );
}

#include "k3baudioburndialog.moc"
