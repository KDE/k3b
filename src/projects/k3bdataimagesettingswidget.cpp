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

#include "k3bdataimagesettingswidget.h"

#include "k3bisooptions.h"

#include <qcheckbox.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qlineedit.h>

#include <kmessagebox.h>
#include <klocale.h>


K3bDataImageSettingsWidget::K3bDataImageSettingsWidget( QWidget* parent, const char* name )
  : base_K3bDataImageSettings( parent, name )
{
  connect( m_checkJoliet, SIGNAL(toggled(bool)),
	   this, SLOT(slotJolietToggled(bool)) );
}


K3bDataImageSettingsWidget::~K3bDataImageSettingsWidget()
{
}


void K3bDataImageSettingsWidget::slotJolietToggled( bool on )
{
  if( !on ) {
    KMessageBox::information( this, 
			      i18n("<p>Be aware that without the Joliet extensions Windows "
				   "systems will not be able to display long filenames. You "
				   "will only see the ISO9660 filenames."
				   "<p>If you do not intend to use the CD/DVD on a Windows "
				   "system it is safe to disable Joliet."),
			      i18n("Joliet Extensions Disabled"),
			      "warning_about_joliet" );
  }
}


void K3bDataImageSettingsWidget::load( const K3bIsoOptions& o )
{
  m_checkRockRidge->setChecked( o.createRockRidge() );
  m_checkJoliet->setChecked( o.createJoliet() );
  m_checkUdf->setChecked( o.createUdf() );

  m_checkDiscardAllLinks->setChecked( o.discardSymlinks() );
  m_checkDiscardBrokenLinks->setChecked( o.discardBrokenSymlinks() );

  m_checkPreservePermissions->setChecked( o.preserveFilePermissions() );

  switch( o.whiteSpaceTreatment() ) {
  case K3bIsoOptions::strip:
    m_radioStrip->setChecked(true);
    break;
  case K3bIsoOptions::extended:
    m_radioExtendedStrip->setChecked(true);
    break;
  case K3bIsoOptions::replace:
    m_radioReplace->setChecked(true);
    break;
  default:
    m_radioNoChange->setChecked(true);
  }

  m_editReplace->setText( o.whiteSpaceTreatmentReplaceString() );
}


void K3bDataImageSettingsWidget::save( K3bIsoOptions& o )
{
  o.setCreateRockRidge( m_checkRockRidge->isChecked() );
  o.setCreateJoliet( m_checkJoliet->isChecked() );
  o.setCreateUdf( m_checkUdf->isChecked() );

  o.setDiscardSymlinks( m_checkDiscardAllLinks->isChecked() );
  o.setDiscardBrokenSymlinks( m_checkDiscardBrokenLinks->isChecked() );

  o.setPreserveFilePermissions( m_checkPreservePermissions->isChecked() );

  if( m_radioStrip->isChecked() )
    o.setWhiteSpaceTreatment( K3bIsoOptions::strip );
  else if( m_radioExtendedStrip->isChecked() )
    o.setWhiteSpaceTreatment( K3bIsoOptions::extended );
  else if( m_radioReplace->isChecked() )
    o.setWhiteSpaceTreatment( K3bIsoOptions::replace );
  else
    o.setWhiteSpaceTreatment( K3bIsoOptions::noChange );

  o.setWhiteSpaceTreatmentReplaceString( m_editReplace->text() );
}


#include "k3bdataimagesettingswidget.moc"
