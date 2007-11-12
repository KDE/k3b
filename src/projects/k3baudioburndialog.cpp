/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
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
#include "k3baudiotrack.h"
#include "k3baudiocdtracksource.h"
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
#include <q3groupbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qtabwidget.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>

#include <q3grid.h>
#include <qtoolbutton.h>
#include <q3ptrlist.h>
#include <qstringlist.h>
#include <qpoint.h>
#include <q3hbox.h>
#include <qspinbox.h>
//Added by qt3to4:
#include <QShowEvent>
#include <Q3GridLayout>

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
  // ----------------------------------------------------------
  QWidget* advancedTab = new QWidget( this );
  Q3GridLayout* advancedTabGrid = new Q3GridLayout( advancedTab );
  advancedTabGrid->setSpacing( spacingHint() );
  advancedTabGrid->setMargin( marginHint() );

  Q3GroupBox* advancedSettingsGroup = new Q3GroupBox( 1, Qt::Vertical, i18n("Settings"), advancedTab );
  m_checkNormalize = K3bStdGuiItems::normalizeCheckBox( advancedSettingsGroup );

  Q3GroupBox* advancedGimmickGroup = new Q3GroupBox( 1, Qt::Vertical, i18n("Gimmicks"), advancedTab );
  m_checkHideFirstTrack = new QCheckBox( i18n( "Hide first track" ), advancedGimmickGroup, "m_checkHideFirstTrack" );

  m_audioRippingGroup = new Q3GroupBox( 3, Qt::Vertical, i18n("Audio Ripping"), advancedTab );
  Q3HBox* box = new Q3HBox( m_audioRippingGroup );
  box->setSpacing( spacingHint() );
  box->setStretchFactor(new QLabel( i18n("Paranoia mode:"), box ), 1 );
  m_comboParanoiaMode = K3bStdGuiItems::paranoiaModeComboBox( box );
  box = new Q3HBox( m_audioRippingGroup );
  box->setSpacing( spacingHint() );
  box->setStretchFactor( new QLabel( i18n("Read retries:"), box ), 1 );
  m_spinAudioRippingReadRetries = new QSpinBox( 1, 128, 1, box );
  m_checkAudioRippingIgnoreReadErrors = new QCheckBox( i18n("Ignore read errors"), m_audioRippingGroup );

  advancedTabGrid->addWidget( advancedSettingsGroup, 0, 0 );
  advancedTabGrid->addWidget( advancedGimmickGroup, 1, 0 );
  advancedTabGrid->addWidget( m_audioRippingGroup, 2, 0 );
  advancedTabGrid->setRowStretch( 3, 1 );

  addPage( advancedTab, i18n("Advanced") );

  connect( m_writerSelectionWidget, SIGNAL(writingAppChanged(int)), this, SLOT(slotToggleAll()) );
  connect( m_checkNormalize, SIGNAL(toggled(bool)), this, SLOT(slotNormalizeToggled(bool)) );
  connect( m_checkCacheImage, SIGNAL(toggled(bool)), this, SLOT(slotCacheImageToggled(bool)) );
  connect( m_writingModeWidget, SIGNAL(writingModeChanged(int)), this, SLOT(slotToggleAll()) );

  // ToolTips
  // -------------------------------------------------------------------------
  QToolTip::add( m_checkHideFirstTrack, i18n("Hide the first track in the first pregap") );

  // What's This info
  // -------------------------------------------------------------------------
  m_checkHideFirstTrack->setWhatsThis(
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

  // audio ripping
  m_doc->setAudioRippingParanoiaMode( m_comboParanoiaMode->currentText().toInt() );
  m_doc->setAudioRippingRetries( m_spinAudioRippingReadRetries->value() );
  m_doc->setAudioRippingIgnoreReadErrors( m_checkAudioRippingIgnoreReadErrors->isChecked() );

  doc()->setTempDir( m_tempDirSelectionWidget->tempPath() );
}


void K3bAudioBurnDialog::readSettings()
{
  K3bProjectBurnDialog::readSettings();

  m_checkHideFirstTrack->setChecked( m_doc->hideFirstTrack() );
  m_checkNormalize->setChecked( m_doc->normalize() );

  // read CD-Text ------------------------------------------------------------
  m_cdtextWidget->load( m_doc );

  // audio ripping
  m_comboParanoiaMode->setCurrentItem( m_doc->audioRippingParanoiaMode() );
  m_checkAudioRippingIgnoreReadErrors->setChecked( m_doc->audioRippingIgnoreReadErrors() );
  m_spinAudioRippingReadRetries->setValue( m_doc->audioRippingRetries() );

  if( !doc()->tempDir().isEmpty() )
    m_tempDirSelectionWidget->setTempPath( doc()->tempDir() );

  toggleAll();
}


void K3bAudioBurnDialog::loadK3bDefaults()
{
  K3bProjectBurnDialog::loadK3bDefaults();

  m_cdtextWidget->setChecked( true );
  m_checkHideFirstTrack->setChecked( false );
  m_checkNormalize->setChecked(false);

  m_comboParanoiaMode->setCurrentItem( 0 );
  m_checkAudioRippingIgnoreReadErrors->setChecked( true );
  m_spinAudioRippingReadRetries->setValue( 5 );

  toggleAll();
}


void K3bAudioBurnDialog::loadUserDefaults( KConfigBase* c )
{
  K3bProjectBurnDialog::loadUserDefaults( c );

  m_cdtextWidget->setChecked( c->readBoolEntry( "cd_text", true ) );
  m_checkHideFirstTrack->setChecked( c->readBoolEntry( "hide_first_track", false ) );
  m_checkNormalize->setChecked( c->readBoolEntry( "normalize", false ) );

  m_comboParanoiaMode->setCurrentItem( c->readNumEntry( "paranoia mode", 0 ) );
  m_checkAudioRippingIgnoreReadErrors->setChecked( c->readBoolEntry( "ignore read errors", true ) );
  m_spinAudioRippingReadRetries->setValue( c->readNumEntry( "read retries", 5 ) );

  toggleAll();
}


void K3bAudioBurnDialog::saveUserDefaults( KConfigBase* c )
{
  K3bProjectBurnDialog::saveUserDefaults( c );

  c->writeEntry( "cd_text", m_cdtextWidget->isChecked() );
  c->writeEntry( "hide_first_track", m_checkHideFirstTrack->isChecked() );
  c->writeEntry( "normalize", m_checkNormalize->isChecked() );

  c->writeEntry( "paranoia mode", m_comboParanoiaMode->currentText() );
  c->writeEntry( "ignore read errors", m_checkAudioRippingIgnoreReadErrors->isChecked() );
  c->writeEntry( "read retries", m_spinAudioRippingReadRetries->value() );
}

void K3bAudioBurnDialog::toggleAll()
{
  K3bProjectBurnDialog::toggleAll();

  bool cdrecordOnTheFly = false;
  bool cdrecordCdText = false;
  if ( k3bcore->externalBinManager()->binObject("cdrecord") ) {
    cdrecordOnTheFly = k3bcore->externalBinManager()->binObject("cdrecord")->hasFeature( "audio-stdin" );
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
    m_checkHideFirstTrack->setChecked(false);
    m_checkHideFirstTrack->setEnabled(false);
  }
  else {
    m_checkHideFirstTrack->setEnabled( !m_checkOnlyCreateImage->isChecked() );
    m_cdtextWidget->setEnabled( !m_checkOnlyCreateImage->isChecked() );
  }

  m_checkCacheImage->setEnabled( !m_checkOnlyCreateImage->isChecked() && 
				 onTheFly );
  if( !onTheFly )
    m_checkCacheImage->setChecked( true );
  m_cdtextWidget->setEnabled( !m_checkOnlyCreateImage->isChecked() &&
			      cdText && 
			      m_writingModeWidget->writingMode() != K3b::TAO );
  if( !cdText || m_writingModeWidget->writingMode() == K3b::TAO )
    m_cdtextWidget->setChecked(false);
}


void K3bAudioBurnDialog::showEvent( QShowEvent* e )
{
  // we only show the audio ripping options when there are audio cd track sources
  bool showRipOptions = false;
  if( m_doc->firstTrack() ) {
    K3bAudioTrack* track = m_doc->firstTrack();
    K3bAudioDataSource* source = track->firstSource();
    
    while( source ) {
      
      if( dynamic_cast<K3bAudioCdTrackSource*>(source) ) {
	showRipOptions = true;
	break;
      }
      
      // next source
      source = source->next();
      if( !source ) {
	track = track->next();
	if( track )
	  source = track->firstSource();
      }
    }
  }

  m_audioRippingGroup->setShown( showRipOptions );

  K3bProjectBurnDialog::showEvent(e);
}


void K3bAudioBurnDialog::slotNormalizeToggled( bool on )
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
    else if( !m_checkCacheImage->isChecked() && !m_checkOnlyCreateImage->isChecked() ) {
      if( KMessageBox::warningYesNo( this, i18n("<p>K3b is not able to normalize audio tracks when burning on-the-fly. "
						"The external program used for this task only supports normalizing a set "
						"of audio files."),
				     QString::null,
				     i18n("Disable normalization"),
				     i18n("Disable on-the-fly burning"),
				     "audioProjectNormalizeOrOnTheFly" ) == KMessageBox::Yes )
	m_checkNormalize->setChecked( false );
      else
	m_checkCacheImage->setChecked( true );
    }
  }
}


void K3bAudioBurnDialog::slotCacheImageToggled( bool on )
{
  if( !on ) {
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
	m_checkCacheImage->setChecked( true );
    }
  }
}

#include "k3baudioburndialog.moc"
