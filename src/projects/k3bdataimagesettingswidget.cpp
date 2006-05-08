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
#include "base_k3bdatacustomfilesystemswidget.h"

#include "k3bisooptions.h"

#include <qcheckbox.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qpushbutton.h>

#include <kmessagebox.h>
#include <klocale.h>
#include <kdialogbase.h>


// indices for the filesystems combobox
static const int FS_LINUX_ONLY = 0;
static const int FS_LINUX_AND_WIN = 1;
static const int FS_UDF = 2;
static const int FS_CUSTOM = 3;


// indices for the whitespace treatment combobox
static const int WS_NO_CHANGE = 0;
static const int WS_STRIP = 1;
static const int WS_EXTENDED_STRIP = 2;
static const int WS_REPLACE = 3;

//
// FIXME: include all the adavnced settings into this dialog
//        and add presets such as "DOS compatibility"
//
class K3bDataImageSettingsWidget::CustomFilesystemsDialog : public KDialogBase
{
public:
  CustomFilesystemsDialog( QWidget* parent ) 
    : KDialogBase( parent,
		   "custom_filesystems_dialog",
		   true,
		   i18n("Custom Data Project Filesystems"),
		   Ok|Cancel,
		   Ok,
		   true ) {
    w = new base_K3bDataCustomFilesystemsWidget( this );
    setMainWidget( w );
  }

  base_K3bDataCustomFilesystemsWidget* w;
};



K3bDataImageSettingsWidget::K3bDataImageSettingsWidget( QWidget* parent, const char* name )
  : base_K3bDataImageSettings( parent, name )
{
  m_customFsDlg = new CustomFilesystemsDialog( this );

  connect( m_comboFilesystems, SIGNAL(activated(int)),
	   this, SLOT(slotFilesystemsChanged()) );
  connect( m_buttonCustomFilesystems, SIGNAL(clicked()),
	   this, SLOT(slotCustomFilesystems()) );
  connect( m_comboSpaceHandling, SIGNAL(activated(int)),
	   this, SLOT(slotSpaceHandlingChanged(int)) );
}


K3bDataImageSettingsWidget::~K3bDataImageSettingsWidget()
{
}


void K3bDataImageSettingsWidget::slotSpaceHandlingChanged( int i )
{
  m_editReplace->setEnabled( i == WS_REPLACE );
}


void K3bDataImageSettingsWidget::slotCustomFilesystems()
{
  switch( m_comboFilesystems->currentItem() ) {
  case FS_LINUX_ONLY:
    m_customFsDlg->w->m_checkRockRidge->setChecked( true );
    m_customFsDlg->w->m_checkJoliet->setChecked( false );
    m_customFsDlg->w->m_checkUdf->setChecked( false );
    break;
  case FS_LINUX_AND_WIN:
    m_customFsDlg->w->m_checkRockRidge->setChecked( true );
    m_customFsDlg->w->m_checkJoliet->setChecked( true );
    m_customFsDlg->w->m_checkUdf->setChecked( false );
    break;
  case FS_UDF:
    m_customFsDlg->w->m_checkRockRidge->setChecked( true );
    m_customFsDlg->w->m_checkJoliet->setChecked( false );
    m_customFsDlg->w->m_checkUdf->setChecked( true );
    break;
  case FS_CUSTOM:
    // keep the settings
    break;
  }

  if( m_customFsDlg->exec() == QDialog::Accepted ) {
    m_comboFilesystems->setCurrentItem( FS_CUSTOM );
    slotFilesystemsChanged();
  }
}


void K3bDataImageSettingsWidget::slotFilesystemsChanged()
{
  // new custom entry
  QStringList s;
  if( m_customFsDlg->w->m_checkRockRidge->isChecked() )
    s += i18n("Rock Ridge");
  if( m_customFsDlg->w->m_checkJoliet->isChecked() )
    s += i18n("Joliet");
  if( m_customFsDlg->w->m_checkUdf->isChecked() )
    s += i18n("UDF");
  if( s.isEmpty() )
    m_comboFilesystems->changeItem( i18n("Custom (ISO9660 only)"), FS_CUSTOM );
  else
    m_comboFilesystems->changeItem( i18n("Custom (%1)").arg( s.join(", ") ), FS_CUSTOM );

  if( m_comboFilesystems->currentItem() == FS_CUSTOM ) {
    if( !m_customFsDlg->w->m_checkRockRidge->isChecked() ) {
      KMessageBox::information( this, 
				i18n("<p>Be aware that it is not recommended to disable the Rock Ridge "
				     "Extensions. There is no disadvantage in enabling Rock Ridge (except "
				     "for a very small space overhead) but a lot of advantages."
				     "<p>Without Rock Ridge Extensions symbolic links are not supported "
				     "and will always be followed as if the \"Follow Symbolic Links\" option "
				     "was enabled."),
				i18n("Rock Ridge Extensions Disabled"),
				"warning_about_rock_ridge" );
    }

    if( !m_customFsDlg->w->m_checkJoliet->isChecked() )
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
  m_customFsDlg->w->m_checkRockRidge->setChecked( o.createRockRidge() );
  m_customFsDlg->w->m_checkJoliet->setChecked( o.createJoliet() );
  m_customFsDlg->w->m_checkUdf->setChecked( o.createUdf() );

  if( o.createRockRidge() && !o.createJoliet() && !o.createUdf() )
    m_comboFilesystems->setCurrentItem( FS_LINUX_ONLY );
  else if( o.createRockRidge() && o.createJoliet() && !o.createUdf() )
    m_comboFilesystems->setCurrentItem( FS_LINUX_AND_WIN );
  else if( o.createRockRidge() && !o.createJoliet() && o.createUdf() )
    m_comboFilesystems->setCurrentItem( FS_UDF );
  else
    m_comboFilesystems->setCurrentItem( FS_CUSTOM );

  slotFilesystemsChanged();

  m_checkDiscardBrokenLinks->setChecked( o.discardBrokenSymlinks() );
  m_checkDiscardAllLinks->setChecked( o.discardSymlinks() );
  m_checkFollowLinks->setChecked( o.followSymbolicLinks() );

  m_checkPreservePermissions->setChecked( o.preserveFilePermissions() );

  switch( o.whiteSpaceTreatment() ) {
  case K3bIsoOptions::strip:
    m_comboSpaceHandling->setCurrentItem( WS_STRIP );
    break;
  case K3bIsoOptions::extended:
    m_comboSpaceHandling->setCurrentItem( WS_EXTENDED_STRIP );
    break;
  case K3bIsoOptions::replace:
    m_comboSpaceHandling->setCurrentItem( WS_REPLACE );
    break;
  default:
    m_comboSpaceHandling->setCurrentItem( WS_NO_CHANGE );
  }
  slotSpaceHandlingChanged( m_comboSpaceHandling->currentItem() );

  m_editReplace->setText( o.whiteSpaceTreatmentReplaceString() );
}


void K3bDataImageSettingsWidget::save( K3bIsoOptions& o )
{
  switch( m_comboFilesystems->currentItem() ) {
  case FS_LINUX_ONLY:
    o.setCreateRockRidge( true );
    o.setCreateJoliet( false );
    o.setCreateUdf( false );
    break;
  case FS_LINUX_AND_WIN:
    o.setCreateRockRidge( true );
    o.setCreateJoliet( true );
    o.setCreateUdf( false );
    break;
  case FS_UDF:
    o.setCreateRockRidge( true );
    o.setCreateJoliet( false );
    o.setCreateUdf( true );
    break;
  default:
    o.setCreateRockRidge( m_customFsDlg->w->m_checkRockRidge->isChecked() );
    o.setCreateJoliet( m_customFsDlg->w->m_checkJoliet->isChecked() );
    o.setCreateUdf( m_customFsDlg->w->m_checkUdf->isChecked() );
  }

  o.setDiscardSymlinks( m_checkDiscardAllLinks->isChecked() );
  o.setDiscardBrokenSymlinks( m_checkDiscardBrokenLinks->isChecked() );
  o.setFollowSymbolicLinks( m_checkFollowLinks->isChecked() );

  o.setPreserveFilePermissions( m_checkPreservePermissions->isChecked() );

  switch( m_comboSpaceHandling->currentItem() ) {
  case WS_STRIP:
    o.setWhiteSpaceTreatment( K3bIsoOptions::strip );
    break;
  case WS_EXTENDED_STRIP:
    o.setWhiteSpaceTreatment( K3bIsoOptions::extended );
    break;
  case WS_REPLACE:
    o.setWhiteSpaceTreatment( K3bIsoOptions::replace );
    break;
  default:
    o.setWhiteSpaceTreatment( K3bIsoOptions::noChange );
  }
  o.setWhiteSpaceTreatmentReplaceString( m_editReplace->text() );
}


#include "k3bdataimagesettingswidget.moc"
