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


#include "k3bprojectburndialog.h"
#include "k3b.h"
#include "k3bdoc.h"
#include "k3bburnprogressdialog.h"
#include "k3bjob.h"
#include "k3btempdirselectionwidget.h"
#include "k3bwriterselectionwidget.h"
#include "k3bstdguiitems.h"
#include "device/k3bdevice.h"
#include "tools/k3bglobals.h"

#include <qstring.h>
#include <qpushbutton.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qlayout.h>
#include <qvbox.h>
#include <qcheckbox.h>
#include <qtabwidget.h>
#include <qgroupbox.h>

#include <klocale.h>
#include <kconfig.h>
#include <kmessagebox.h>
#include <kguiitem.h>
#include <kstdguiitem.h>
#include <kdebug.h>



K3bProjectBurnDialog::K3bProjectBurnDialog(K3bDoc* doc, QWidget *parent, const char *name, bool modal )
  : K3bInteractionDialog( parent, name, i18n("Project"), QString::null, 
			  START_BUTTON|SAVE_BUTTON|CANCEL_BUTTON, START_BUTTON, modal ),
    m_writerSelectionWidget(0),
    m_tempDirSelectionWidget(0)
{
  m_doc = doc;

  setSaveButtonText( i18n("Save"), i18n("Save Settings and close"),
		     i18n("Saves the settings to the project and closes the burn dialog.") );
  setStartButtonText( i18n("Write") );

  m_job = 0;
}


K3bProjectBurnDialog::~K3bProjectBurnDialog(){
}



void K3bProjectBurnDialog::slotWriterChanged()
{
  toggleAllOptions();
}


void K3bProjectBurnDialog::slotWritingAppChanged( int )
{
  toggleAllOptions();
}


void K3bProjectBurnDialog::toggleAllOptions()
{
  if( K3bDevice* dev = m_writerSelectionWidget->writerDevice() ) {
    if( dev->burnproof() ) {
      if( m_writerSelectionWidget->writingApp() == K3b::CDRDAO ) {
	// no possibility to disable burnfree yet
	m_checkBurnproof->setChecked(true);
	m_checkBurnproof->setEnabled(false);
      }
      else {
	m_checkBurnproof->setEnabled(!m_checkOnlyCreateImage->isChecked());
      }
    }
    else {
      m_checkBurnproof->setChecked( false );
      m_checkBurnproof->setEnabled( false );
    }
 
    if( dev->dao() ) {
      if( m_writerSelectionWidget->writingApp() == K3b::CDRDAO ) {
	// cdrdao only writes in DAO mode
	m_checkDao->setChecked(true);
	m_checkDao->setEnabled(false);
      }
      else {
	m_checkDao->setEnabled(!m_checkOnlyCreateImage->isChecked());
      }
    }
    else {
      m_checkDao->setChecked( false );
      m_checkDao->setEnabled( false );
    }

    m_buttonStart->setDisabled(false);
  }
  else
    m_buttonStart->setDisabled(true);

  m_checkSimulate->setDisabled( m_checkOnlyCreateImage->isChecked() );
  m_checkOnTheFly->setDisabled( m_checkOnlyCreateImage->isChecked() );
  m_checkRemoveBufferFiles->setDisabled( m_checkOnlyCreateImage->isChecked() || m_checkOnTheFly->isChecked() );
  if( m_checkOnlyCreateImage->isChecked() ) {
    m_checkRemoveBufferFiles->setChecked(false);
    m_buttonStart->setDisabled(false);
  }
  m_tempDirSelectionWidget->setDisabled( m_checkOnTheFly->isChecked() && !m_checkOnlyCreateImage->isChecked() );
  m_writerSelectionWidget->setDisabled( m_checkOnlyCreateImage->isChecked() );
}


int K3bProjectBurnDialog::exec( bool burn )
{
  if( burn && m_job == 0 ) {
    m_buttonStart->show();
    setDefaultButton( START_BUTTON );
  }
  else {
    m_buttonStart->hide();
    setDefaultButton( SAVE_BUTTON );
  }

  readSettings();

  return K3bInteractionDialog::exec();
}


void K3bProjectBurnDialog::slotSaveClicked()
{
  saveSettings();
  m_doc->updateAllViews();
  done( Saved );
}


void K3bProjectBurnDialog::slotCancelClicked()
{
  done( Canceled );
}


void K3bProjectBurnDialog::slotStartClicked()
{
  if( m_job ) {
    KMessageBox::sorry( k3bMain(), i18n("K3b is already working on this project!"), i18n("Error") );
    return;
  }

  saveSettings();

  // check if enough space in tempdir if not on-the-fly
  if( m_tempDirSelectionWidget )
    if( !doc()->onTheFly() && doc()->size()/1024 > m_tempDirSelectionWidget->freeTempSpace() ) {
      if( KMessageBox::warningYesNo( this, i18n("There seems to be not enough free space in temporary directory. Write anyway?") ) == KMessageBox::No )
	return;
    }


  m_job = m_doc->newBurnJob();

  if( m_writerSelectionWidget )
    m_job->setWritingApp( m_writerSelectionWidget->writingApp() );
  prepareJob( m_job );

  K3bJobProgressDialog* d = 0;
  if( m_checkOnlyCreateImage && m_checkOnlyCreateImage->isChecked() )
    d = new K3bJobProgressDialog( k3bMain() );
  else
    d = new K3bBurnProgressDialog( k3bMain() );

  hide();
  d->startJob(m_job);

  delete m_job;
  delete d;

  done( Burn );
}


void K3bProjectBurnDialog::prepareGui()
{
  m_tabWidget = new QTabWidget( this );
  setMainWidget( m_tabWidget );
  QWidget* w = new QWidget( m_tabWidget );
  m_tabWidget->addTab( w, i18n("Writing") );
  m_writerSelectionWidget = new K3bWriterSelectionWidget( w );
  m_tempDirSelectionWidget = new K3bTempDirSelectionWidget( w );

  m_optionGroup = new QGroupBox( 0, Qt::Vertical, i18n("Options"), w );
  m_optionGroup->layout()->setMargin(0);
  m_optionGroup->layout()->setSpacing(0);
  m_optionGroupLayout = new QVBoxLayout( m_optionGroup->layout() );
  m_optionGroupLayout->setMargin( KDialog::marginHint() );
  m_optionGroupLayout->setSpacing( KDialog::spacingHint() );

  // add the options
  m_checkDao = K3bStdGuiItems::daoCheckbox( m_optionGroup );
  m_checkOnTheFly = K3bStdGuiItems::onTheFlyCheckbox( m_optionGroup );
  m_checkBurnproof = K3bStdGuiItems::burnproofCheckbox( m_optionGroup );
  m_checkSimulate = K3bStdGuiItems::simulateCheckbox( m_optionGroup );
  m_checkRemoveBufferFiles = K3bStdGuiItems::removeImagesCheckbox( m_optionGroup );
  m_checkOnlyCreateImage = K3bStdGuiItems::onlyCreateImagesCheckbox( m_optionGroup );

  m_optionGroupLayout->addWidget(m_checkSimulate);
  m_optionGroupLayout->addWidget(m_checkOnTheFly);
  m_optionGroupLayout->addWidget(m_checkDao);
  m_optionGroupLayout->addWidget(m_checkBurnproof);
  m_optionGroupLayout->addWidget(m_checkOnlyCreateImage);
  m_optionGroupLayout->addWidget(m_checkRemoveBufferFiles);

  // arrange it
  QGridLayout* grid = new QGridLayout( w );
  grid->setMargin( KDialog::marginHint() );
  grid->setSpacing( KDialog::spacingHint() );
  grid->addMultiCellWidget( m_writerSelectionWidget, 0, 0, 0, 1 );
  grid->addWidget( m_optionGroup, 1, 0 );
  grid->addWidget( m_tempDirSelectionWidget, 1, 1 );
  grid->setRowStretch( 1, 1 );
  grid->setColStretch( 1, 1 );

  // some default connections that should always be useful
  connect( m_writerSelectionWidget, SIGNAL(writerChanged()), this, SLOT(slotWriterChanged()) );
  connect( m_writerSelectionWidget, SIGNAL(writingAppChanged(int)), this, SLOT(slotWritingAppChanged(int)) );
  connect( m_checkOnTheFly, SIGNAL(toggled(bool)), this, SLOT(toggleAllOptions()) );
  connect( m_checkOnlyCreateImage, SIGNAL(toggled(bool)), this, SLOT(toggleAllOptions()) );

  m_tempDirSelectionWidget->setNeededSize( doc()->size() );
}


void K3bProjectBurnDialog::addPage( QWidget* page, const QString& title )
{
  m_tabWidget->addTab( page, title );
}



// void K3bProjectBurnDialog::loadDefaults()
// {
//   K3bDocSettings s = m_doc->settings();
//   s.defaults();
//   loadSettings( s );
// }


// void K3bProjectBurnDialog::loadUserDefaults()
// {
//   KConfig* c = k3bMain()->config();
//   c->setGroup( "default " + m_doc->documentType() + " settings" );
//   K3bDocSettings s = m_doc->settings();
//   s.loadUserDefaults( c );
//   loadSettings( s );
// }


// void K3bProjectBurnDialog::saveUserDefaults()
// {
//   KConfig* c = k3bMain()->config();
//   c->setGroup( "default " + m_doc->documentType() + " settings" );
//   settings().saveUserDefaults( c );
// }


#include "k3bprojectburndialog.moc"
