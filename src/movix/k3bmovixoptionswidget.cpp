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


#include "k3bmovixoptionswidget.h"
#include "k3bmovixdoc.h"
#include "k3bmovixinstallation.h"

#include <kcombobox.h>
#include <klocale.h>

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


void K3bMovixOptionsWidget::init( K3bMovixInstallation* installation )
{
  m_comboSubtitleFontset->insertStringList( installation->supportedSubtitleFonts() );
  m_comboBootMessageLanguage->insertStringList( installation->supportedLanguages() );
  m_comboDefaultBootLabel->insertStringList( installation->supportedBootLabels() );
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
}


void K3bMovixOptionsWidget::saveSettings( K3bMovixDoc* doc )
{
  doc->setShutdown( m_checkShutdown->isChecked() );
  doc->setSeboot( m_checkReboot->isChecked() );
  doc->setEjectDisk( m_checkEject->isChecked() );
  doc->setSubtitleFontset( m_comboSubtitleFontset->currentText() );
  doc->setBootMessageLanguage( m_comboBootMessageLanguage->currentText() );
  doc->setDefaultBootLabel( m_comboDefaultBootLabel->currentText() );
  doc->setAdditionalMPlayerOptions( m_editAdditionalMplayerOptions->text() );
  doc->setUnwantedMPlayerOptions( m_editUnwantedMplayerOptions->text() );
  doc->setLoopPlaylist( m_spinLoop->value() );
}


#include "k3bmovixoptionswidget.moc"

