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


#include "k3bmiscoptiontab.h"

#include <qcheckbox.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qtoolbutton.h>
#include <qlabel.h>
#include <qgroupbox.h>

#include <kapplication.h>
#include <klocale.h>
#include <kconfig.h>
#include <kdialog.h>
#include <kfiledialog.h>
#include <kstandarddirs.h>
#include <kmessagebox.h>
#include <kiconloader.h>
#include <kio/netaccess.h>


K3bMiscOptionTab::K3bMiscOptionTab(QWidget *parent, const char *name )
  : QWidget(parent,name)
{
  m_checkShowSplash = new QCheckBox( i18n("Show splash screen"), this );
  m_checkShowSystemTrayProgress = new QCheckBox( i18n("Show progress in system tray"), this );

  QGroupBox* groupTempDir = new QGroupBox( 2, Qt::Horizontal, i18n("Default Temporary Directory"), this );
  groupTempDir->layout()->setMargin( KDialog::marginHint() );
  groupTempDir->layout()->setSpacing( KDialog::spacingHint() );


  // FIXME: Use KURLRequester!

  m_editTempDir = new QLineEdit( groupTempDir );
  m_buttonTempDir = new QToolButton( groupTempDir );
  m_buttonTempDir->setIconSet( SmallIconSet( "fileopen" ) );


  QGridLayout* mainGrid = new QGridLayout( this );
  mainGrid->setAlignment( Qt::AlignTop );
  mainGrid->setSpacing( KDialog::spacingHint() );
  mainGrid->setMargin( 0 );

  mainGrid->addWidget( m_checkShowSplash, 0, 0 );
  mainGrid->addWidget( m_checkShowSystemTrayProgress, 1, 0 );
  mainGrid->addWidget( groupTempDir, 2, 0 );

  connect( m_buttonTempDir, SIGNAL(clicked()), this, SLOT(slotGetTempDir()) );
}


K3bMiscOptionTab::~K3bMiscOptionTab()
{
}


void K3bMiscOptionTab::readSettings()
{
  KConfig* c = kapp->config();
  c->setGroup( "General Options" );
  m_checkShowSplash->setChecked( c->readBoolEntry("Show splash", true) );
  m_checkShowSystemTrayProgress->setChecked( c->readBoolEntry( "Show progress in system tray", true ) );

  QString tempdir = c->readEntry( "Temp Dir", KGlobal::dirs()->resourceDirs( "tmp" ).first() );
  m_editTempDir->setText( tempdir );
}


bool K3bMiscOptionTab::saveSettings()
{
  KConfig* c = kapp->config();
  c->setGroup( "General Options" );
  c->writeEntry( "Show splash", m_checkShowSplash->isChecked() );
  c->writeEntry( "Show progress in system tray", m_checkShowSystemTrayProgress->isChecked() );
;
  QString tempDir = m_editTempDir->text();
  QFileInfo fi( tempDir );

  if( fi.isRelative() ) {
    fi.setFile( fi.absFilePath() );
  }

  if( !fi.exists() ) {
    if( KMessageBox::questionYesNo( this, i18n("Directory does not exist. Create?"),
				    i18n("Create Directory") ) == KMessageBox::Yes ) {
      if( !KIO::NetAccess::mkdir( fi.absFilePath() ) ) {
	KMessageBox::error( this, i18n("Unable to create directory\n(%1)").arg(fi.absFilePath()) );
	return false;
      }
    }
    else {
      // the dir does not exist and the user doesn't want to create it
      return false;
    }
  }

  if( fi.isFile() ) {
    KMessageBox::information( this, i18n("You specified a file for the temporary directory. K3b will use its base path as the temporary directory."), i18n("Warning"), i18n("Don't show again.") );
    fi.setFile( fi.dirPath() );
  }

  // check for writing permission
  if( !fi.isWritable() ) {
    KMessageBox::error( this, i18n("You don't have permission to write to %1.").arg(fi.absFilePath()) );
    return false;
  }

  c->writeEntry( "Temp Dir", m_editTempDir->text() );

  return true;
}


void K3bMiscOptionTab::slotGetTempDir()
{
  QString dir = KFileDialog::getExistingDirectory( m_editTempDir->text(), this, i18n("Select Temp Directory") );
  if( !dir.isEmpty() ) {
    m_editTempDir->setText( dir );
  }
}


#include "k3bmiscoptiontab.moc"
