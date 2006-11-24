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


#include "k3bmiscoptiontab.h"

#include <k3bpluginmanager.h>
#include <k3baudiooutputplugin.h>
#include <k3baudioserver.h>
#include <k3bcore.h>
#include <k3bservicemenuinstaller.h>

#include <qcheckbox.h>
#include <qfileinfo.h>
#include <qradiobutton.h>

#include <kapplication.h>
#include <klocale.h>
#include <kconfig.h>
#include <kdialog.h>
#include <kstandarddirs.h>
#include <kmessagebox.h>
#include <kurlrequester.h>
#include <kcombobox.h>


K3bMiscOptionTab::K3bMiscOptionTab(QWidget *parent, const char *name )
  : base_K3bMiscOptionTab(parent,name)
{
  m_editTempDir->setMode( KFile::Directory );
  connect( m_buttonConfigureAudioOutput, SIGNAL(clicked()),
	   this, SLOT(slotConfigureAudioOutput()) );
}


K3bMiscOptionTab::~K3bMiscOptionTab()
{
}


void K3bMiscOptionTab::readSettings()
{
  KConfig* c = kapp->config();
  c->setGroup( "General Options" );
  m_checkSaveOnExit->setChecked( c->readBoolEntry( "ask_for_saving_changes_on_exit", true ) );
  m_checkShowSplash->setChecked( c->readBoolEntry("Show splash", true) );
  m_checkShowProgressOSD->setChecked( c->readBoolEntry( "Show progress OSD", true ) );
  m_checkHideMainWindowWhileWriting->setChecked( c->readBoolEntry( "hide main window while writing", false ) );
  m_checkKeepDialogsOpen->setChecked( c->readBoolEntry( "keep action dialogs open", false ) );
  m_checkSystemConfig->setChecked( c->readBoolEntry( "check system config", true ) );

  QString tempdir = c->readPathEntry( "Temp Dir", KGlobal::dirs()->resourceDirs( "tmp" ).first() );
  m_editTempDir->setURL( tempdir );

//   if( c->readEntry( "Multiple Instances", "smart" ) == "smart" )
//     m_radioMultipleInstancesSmart->setChecked(true);
//   else
//     m_radioMultipleInstancesNew->setChecked(true);

  // Audio Output
  m_comboAudioOutputSystem->clear();
  QPtrList<K3bPlugin> fl = k3bcore->pluginManager()->plugins( "AudioOutput" );
  for( QPtrListIterator<K3bPlugin> it( fl ); it.current(); ++it ) {
    K3bAudioOutputPlugin* f = static_cast<K3bAudioOutputPlugin*>( it.current() );
    m_comboAudioOutputSystem->insertItem( QString::fromLocal8Bit(f->soundSystem()) );
  }

  m_comboAudioOutputSystem->setCurrentItem( c->readEntry( "Audio Output System", "arts" ), false );
  m_buttonConfigureAudioOutput->setEnabled( m_comboAudioOutputSystem->count() > 0 );

  K3bServiceInstaller si;
  m_checkKonqiIntegration->setChecked( si.allInstalled() );
}


bool K3bMiscOptionTab::saveSettings()
{
  KConfig* c = kapp->config();
  c->setGroup( "General Options" );
  c->writeEntry( "ask_for_saving_changes_on_exit", m_checkSaveOnExit->isChecked() );
  c->writeEntry( "Show splash", m_checkShowSplash->isChecked() );
  c->writeEntry( "Show progress OSD", m_checkShowProgressOSD->isChecked() );
  c->writeEntry( "hide main window while writing", m_checkHideMainWindowWhileWriting->isChecked() );
  c->writeEntry( "keep action dialogs open", m_checkKeepDialogsOpen->isChecked() );
  c->writeEntry( "check system config", m_checkSystemConfig->isChecked() );

  QString tempDir = m_editTempDir->url();
  QFileInfo fi( tempDir );

  if( fi.isRelative() ) {
    fi.setFile( fi.absFilePath() );
  }

  if( !fi.exists() ) {
    if( KMessageBox::questionYesNo( this, i18n("Directory (%1) does not exist. Create?").arg(tempDir),
				    i18n("Create Directory"), i18n("Create"), KStdGuiItem::cancel() ) == KMessageBox::Yes ) {
      if( !KStandardDirs::makeDir( fi.absFilePath() ) ) {
	KMessageBox::error( this, i18n("Unable to create directory %1").arg(tempDir) );
	return false;
      }
    }
    else {
      // the dir does not exist and the user doesn't want to create it
      return false;
    }
  }

  if( fi.isFile() ) {
    KMessageBox::information( this, i18n("You specified a file for the temporary directory. "
					 "K3b will use its base path as the temporary directory."), 
			      i18n("Warning"), 
			      "temp file only using base path" );
    fi.setFile( fi.dirPath() );
  }

  // check for writing permission
  if( !fi.isWritable() ) {
    KMessageBox::error( this, i18n("You do not have permission to write to %1.").arg(fi.absFilePath()) );
    return false;
  }

  m_editTempDir->setURL( fi.absFilePath() );

  c->writePathEntry( "Temp Dir", m_editTempDir->url() );

//   if( m_radioMultipleInstancesSmart->isChecked() )
//     c->writeEntry( "Multiple Instances", "smart" );
//   else
//     c->writeEntry( "Multiple Instances", "always_new" );

  // Audio Output System
  if( m_comboAudioOutputSystem->count() > 0 ) {
    c->writeEntry( "Audio Output System", m_comboAudioOutputSystem->currentText() );
    K3bAudioServer::instance()->setOutputMethod( m_comboAudioOutputSystem->currentText().local8Bit() );
  }

  K3bServiceInstaller si;
  if( m_checkKonqiIntegration->isChecked() )
    si.install( this );
  else
    si.remove( this );

  return true;
}


void K3bMiscOptionTab::slotConfigureAudioOutput()
{
  QString system = m_comboAudioOutputSystem->currentText();
  if( K3bAudioOutputPlugin* plugin = K3bAudioServer::findOutputPlugin( system.local8Bit() ) ) {
    k3bcore->pluginManager()->execPluginDialog( plugin, this );
  }
}

#include "k3bmiscoptiontab.moc"
