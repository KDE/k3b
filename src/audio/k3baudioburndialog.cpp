/***************************************************************************
                          k3baudioburndialog.cpp  -  description
                             -------------------
    begin                : Sun Apr 1 2001
    copyright            : (C) 2001 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "k3baudioburndialog.h"
#include "../k3b.h"
#include "k3baudiodoc.h"
#include "../device/k3bdevice.h"
#include "../k3bwriterselectionwidget.h"
#include "../k3btempdirselectionwidget.h"
#include "k3baudiocdtextwidget.h"
#include <tools/k3bglobals.h>

#include <qcheckbox.h>
#include <qcombobox.h>
#include <qgroupbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qmultilineedit.h>
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

  QGroupBox* advancedGimmickGroup = new QGroupBox( 1, Qt::Vertical, i18n("Gimmicks"), advancedTab );
  m_checkHideFirstTrack = new QCheckBox( i18n( "Hide first track" ), advancedGimmickGroup, "m_checkHideFirstTrack" );

  advancedTabGrid->addWidget( advancedGimmickGroup, 0, 0 );
  advancedTabGrid->setRowStretch( 1, 1 );

  addPage( advancedTab, i18n("Advanced") );

  connect( m_checkDao, SIGNAL(toggled(bool)), this, SLOT(slotToggleEverything()) );
  connect( m_writerSelectionWidget, SIGNAL(writingAppChanged(int)), this, SLOT(slotToggleEverything()) );

  readSettings();

  // ToolTips
  // -------------------------------------------------------------------------
  QToolTip::add( m_checkHideFirstTrack, i18n("Hide the first track in the first pregap") );

  // What's This info
  // -------------------------------------------------------------------------
  QWhatsThis::add( m_checkHideFirstTrack, i18n("<p>If this option is checked K3b will <em>hide</em> the first track."
					       "<p>The audio CD standard uses pregaps before every track on the CD. "
					       "By default these last for 2 seconds and are silent. In DAO mode it "
					       "is possible to have longer pregaps that contain some audio. In this case "
					       "the first pregap will contain the complete first track."
					       "<p>You will need to seek back from the beginning of the CD to listen to "
					       "the first track. Try it, it's quite amusing!"
					       "<p><b>This feature is only available in DAO mode when writing with cdrdao.") );
}

K3bAudioBurnDialog::~K3bAudioBurnDialog(){
}


void K3bAudioBurnDialog::saveSettings()
{
  m_doc->setTempDir( m_tempDirSelectionWidget->tempPath() );
  m_doc->setDao( m_checkDao->isChecked() );
  m_doc->setDummy( m_checkSimulate->isChecked() );
  m_doc->setOnTheFly( m_checkOnTheFly->isChecked() );
  m_doc->setBurnproof( m_checkBurnproof->isChecked() );
  m_doc->setHideFirstTrack( m_checkHideFirstTrack->isChecked() );
  m_doc->setRemoveBufferFiles( m_checkRemoveBufferFiles->isChecked() );

  // -- saving current speed --------------------------------------
  m_doc->setSpeed( m_writerSelectionWidget->writerSpeed() );

  // -- saving current device --------------------------------------
  m_doc->setBurner( m_writerSelectionWidget->writerDevice() );

  // -- save Cd-Text ------------------------------------------------
  m_cdtextWidget->save( m_doc );
}


void K3bAudioBurnDialog::readSettings()
{
  m_checkDao->setChecked( m_doc->dao() );
  m_checkOnTheFly->setChecked( m_doc->onTheFly() );
  m_checkSimulate->setChecked( m_doc->dummy() );
  m_checkHideFirstTrack->setChecked( m_doc->hideFirstTrack() );
  m_checkRemoveBufferFiles->setChecked( m_doc->removeBufferFiles() );
  m_checkBurnproof->setChecked( doc()->burnproof() );

  // read CD-Text ------------------------------------------------------------
  m_cdtextWidget->load( m_doc );

  slotWriterChanged();
}


void K3bAudioBurnDialog::loadDefaults()
{
  m_checkSimulate->setChecked( false );
  m_checkDao->setChecked( true );
  m_checkOnTheFly->setChecked( true );
  m_checkBurnproof->setChecked( true );

  m_cdtextWidget->setChecked( true );
  m_checkHideFirstTrack->setChecked( false );
  m_checkRemoveBufferFiles->setChecked( true );

  slotWriterChanged();
  slotToggleEverything();
}


void K3bAudioBurnDialog::loadUserDefaults()
{
  KConfig* c = k3bMain()->config();

  c->setGroup( "default audio settings" );

  m_checkSimulate->setChecked( c->readBoolEntry( "dummy_mode", false ) );
  m_checkDao->setChecked( c->readBoolEntry( "dao", true ) );
  m_checkOnTheFly->setChecked( c->readBoolEntry( "on_the_fly", true ) );
  m_checkBurnproof->setChecked( c->readBoolEntry( "burnproof", true ) );

  m_cdtextWidget->setChecked( c->readBoolEntry( "cd_text", true ) );
  m_checkHideFirstTrack->setChecked( c->readBoolEntry( "hide_first_track", false ) );
  m_checkRemoveBufferFiles->setChecked( c->readBoolEntry( "remove_buffer_files", true ) );

  slotWriterChanged();
  slotToggleEverything();
}


void K3bAudioBurnDialog::saveUserDefaults()
{
  KConfig* c = k3bMain()->config();

  c->setGroup( "default audio settings" );

  c->writeEntry( "dummy_mode", m_checkSimulate->isChecked() );
  c->writeEntry( "dao", m_checkDao->isChecked() );
  c->writeEntry( "on_the_fly", m_checkOnTheFly->isChecked() );
  c->writeEntry( "burnproof", m_checkBurnproof->isChecked() );
  c->writeEntry( "cd_text", m_cdtextWidget->isChecked() );
  c->writeEntry( "hide_first_track", m_checkHideFirstTrack->isChecked() );
  c->writeEntry( "remove_buffer_files", m_checkRemoveBufferFiles->isChecked() );

  if( m_tempDirSelectionWidget->isEnabled() ) {
    m_tempDirSelectionWidget->saveConfig();
  }
}


void K3bAudioBurnDialog::slotToggleEverything()
{
  // currently we do not support writing on the fly with cdrecord
  if( !m_checkDao->isChecked() || m_writerSelectionWidget->writingApp() == K3b::CDRECORD ) {
    m_checkOnTheFly->setEnabled( false );
    m_checkOnTheFly->setChecked( false );
    m_checkHideFirstTrack->setEnabled(false);
    m_checkHideFirstTrack->setChecked(false);
    m_cdtextWidget->setChecked(false);
    m_cdtextWidget->setEnabled(false);
  }
  else {
    m_checkOnTheFly->setEnabled( true );
    m_checkHideFirstTrack->setEnabled(true);
    m_cdtextWidget->setEnabled(true);
  }
}

#include "k3baudioburndialog.moc"
