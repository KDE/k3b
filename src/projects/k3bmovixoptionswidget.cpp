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


#include "k3bmovixoptionswidget.h"
#include "k3bmovixdoc.h"
#include "k3bmovixprogram.h"

#include <kcombobox.h>
#include <klocale.h>
#include <kconfig.h>

#include <qcheckbox.h>
#include <qspinbox.h>
#include <qstringlist.h>


K3bMovixOptionsWidget::K3bMovixOptionsWidget( QWidget* parent, const char* name )
  : base_K3bMovixOptionsWidget( parent, name )
{

}


K3bMovixOptionsWidget::~K3bMovixOptionsWidget()
{
}


void K3bMovixOptionsWidget::init( const K3bMovixBin* bin )
{
  m_comboSubtitleFontset->insertStringList( bin->supportedSubtitleFonts() );

  // FIXME: use KLocale to get the names of the languages (also change the option loading/saveing accordingly)
  m_comboBootMessageLanguage->insertStringList( bin->supportedLanguages() );
  m_comboDefaultBootLabel->insertStringList( bin->supportedBootLabels() );
}


void K3bMovixOptionsWidget::readSettings( K3bMovixDoc* doc )
{
  m_comboSubtitleFontset->setCurrentItem( doc->subtitleFontset(), false );
  m_spinLoop->setValue( doc->loopPlaylist() );
  m_editAdditionalMplayerOptions->setText( doc->additionalMPlayerOptions() );
  m_editUnwantedMplayerOptions->setText( doc->unwantedMPlayerOptions() );
  m_comboBootMessageLanguage->setCurrentItem( doc->bootMessageLanguage(), false );
  m_comboDefaultBootLabel->setCurrentItem( doc->defaultBootLabel(), false );
  m_checkShutdown->setChecked( doc->shutdown() );
  m_checkReboot->setChecked( doc->reboot() );
  m_checkEject->setChecked( doc->ejectDisk() );
  m_checkRandomPlay->setChecked( doc->randomPlay() );
  m_checkNoDma->setChecked( doc->noDma() );
}


void K3bMovixOptionsWidget::saveSettings( K3bMovixDoc* doc )
{
  doc->setShutdown( m_checkShutdown->isChecked() );
  doc->setReboot( m_checkReboot->isChecked() );
  doc->setEjectDisk( m_checkEject->isChecked() );
  doc->setSubtitleFontset( m_comboSubtitleFontset->currentText() );
  doc->setBootMessageLanguage( m_comboBootMessageLanguage->currentText() );
  doc->setDefaultBootLabel( m_comboDefaultBootLabel->currentText() );
  doc->setAdditionalMPlayerOptions( m_editAdditionalMplayerOptions->text() );
  doc->setUnwantedMPlayerOptions( m_editUnwantedMplayerOptions->text() );
  doc->setLoopPlaylist( m_spinLoop->value() );
  doc->setRandomPlay( m_checkRandomPlay->isChecked() );
  doc->setNoDma( m_checkNoDma->isChecked() );
}


void K3bMovixOptionsWidget::loadDefaults()
{
  m_comboSubtitleFontset->setCurrentItem( 0 ); // default
  m_spinLoop->setValue( 1 );
  m_editAdditionalMplayerOptions->setText( QString::null );
  m_editUnwantedMplayerOptions->setText( QString::null );
  m_comboBootMessageLanguage->setCurrentItem( 0 ); // default
  m_comboDefaultBootLabel->setCurrentItem( 0 );  // default
  m_checkShutdown->setChecked( false );
  m_checkReboot->setChecked( false );
  m_checkEject->setChecked( false );
  m_checkRandomPlay->setChecked( false );
  m_checkNoDma->setChecked( false );
}


void K3bMovixOptionsWidget::loadConfig( KConfig* c )
{
  QString s = c->readEntry("subtitle_fontset");
  if( !s.isEmpty() && s != "none" && m_comboSubtitleFontset->contains(s) )
    m_comboSubtitleFontset->setCurrentItem( s, false );
  else
    m_comboSubtitleFontset->setCurrentItem( 0 ); // none

  m_spinLoop->setValue( c->readNumEntry("loop", 1 ) );
  m_editAdditionalMplayerOptions->setText( c->readEntry( "additional_mplayer_options" ) );
  m_editUnwantedMplayerOptions->setText( c->readEntry( "unwanted_mplayer_options" ) );

  s = c->readEntry("boot_message_language");
  if( !s.isEmpty() && s != "default" && m_comboBootMessageLanguage->contains(s) )
    m_comboBootMessageLanguage->setCurrentItem( s, false );
  else
    m_comboBootMessageLanguage->setCurrentItem( 0 ); // default

  s = c->readEntry( "default_boot_label" );
  if( !s.isEmpty() && s != "default" && m_comboDefaultBootLabel->contains(s) )
    m_comboDefaultBootLabel->setCurrentItem( s, false );
  else
    m_comboDefaultBootLabel->setCurrentItem( 0 );  // default

  m_checkShutdown->setChecked( c->readBoolEntry( "shutdown", false) );
  m_checkReboot->setChecked( c->readBoolEntry( "reboot", false ) );
  m_checkEject->setChecked( c->readBoolEntry( "eject", false ) );
  m_checkRandomPlay->setChecked( c->readBoolEntry( "random_play", false ) );
  m_checkNoDma->setChecked( c->readBoolEntry( "no_dma", false ) );
}


void K3bMovixOptionsWidget::saveConfig( KConfig* c )
{
  if( m_comboSubtitleFontset->currentItem() == 0 )
    c->writeEntry( "subtitle_fontset", "none" );
  else
    c->writeEntry( "subtitle_fontset", m_comboSubtitleFontset->currentText() );

  c->writeEntry( "loop", m_spinLoop->value() );
  c->writeEntry( "additional_mplayer_options", m_editAdditionalMplayerOptions->text() );
  c->writeEntry( "unwanted_mplayer_options", m_editUnwantedMplayerOptions->text() );

  if( m_comboBootMessageLanguage->currentItem() == 0 )
    c->writeEntry( "boot_message_language", "default" );
  else
    c->writeEntry( "boot_message_language", m_comboBootMessageLanguage->currentText() );

  if( m_comboDefaultBootLabel->currentItem() == 0 )
    c->writeEntry( "default_boot_label", "default" );
  else
    c->writeEntry( "default_boot_label", m_comboDefaultBootLabel->currentText() );

  c->writeEntry( "shutdown", m_checkShutdown->isChecked() );
  c->writeEntry( "reboot", m_checkReboot->isChecked() );
  c->writeEntry( "eject", m_checkEject->isChecked() );
  c->writeEntry( "random_play", m_checkRandomPlay->isChecked() );
  c->writeEntry( "no_dma", m_checkNoDma->isChecked() );
}

#include "k3bmovixoptionswidget.moc"

