/***************************************************************************
                          k3bprojectburndialog.cpp  -  description
                             -------------------
    begin                : Thu May 17 2001
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

#include "k3bprojectburndialog.h"
#include "k3b.h"
#include "k3bdoc.h"
#include "k3bburnprogressdialog.h"
#include "k3bjob.h"
#include "k3btempdirselectionwidget.h"
#include "k3bwriterselectionwidget.h"
#include "k3bstdguiitems.h"
#include "device/k3bdevice.h"


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
  : KDialogBase( parent, name, modal, i18n("Write CD"), Ok|User1|User2, User1, false,
                 KGuiItem( i18n("Save"), "filesave", i18n("Save Settings and close"),
                           i18n("Saves the settings to the project and closes the burn dialog.") ),
                 KStdGuiItem::cancel() ),
    m_writerSelectionWidget(0)
{
  m_doc = doc;

  setButtonBoxOrientation( Vertical );
  setButtonText( Ok, i18n("Write") );
  m_job = 0;

  QWidget* box = new QWidget( this );
  setMainWidget( box );

  m_k3bMainWidget = new QVBox( box );

  QGridLayout* grid = new QGridLayout( box );
  grid->setSpacing( spacingHint() );
  grid->setMargin( 0 );

  m_buttonLoadDefaults = new QPushButton( i18n("Defaults"), box );
  m_buttonLoadUserDefaults = new QPushButton( i18n("User Defaults"), box );
  m_buttonSaveUserDefaults = new QPushButton( i18n("Save User Defaults"), box );

  grid->addMultiCellWidget( m_k3bMainWidget, 0, 0, 0, 3 );
  grid->addWidget( m_buttonLoadDefaults, 1, 0 );
  grid->addWidget( m_buttonLoadUserDefaults, 1, 2 );
  grid->addWidget( m_buttonSaveUserDefaults, 1, 3 );
  grid->setRowStretch( 0, 1 );
  grid->setColStretch( 1, 1 );

  connect( m_buttonLoadDefaults, SIGNAL(clicked()), this, SLOT(loadDefaults()) );
  connect( m_buttonLoadUserDefaults, SIGNAL(clicked()), this, SLOT(loadUserDefaults()) );
  connect( m_buttonSaveUserDefaults, SIGNAL(clicked()), this, SLOT(saveUserDefaults()) );


  // ToolTips
  // -------------------------------------------------------------------------
  QToolTip::add( m_buttonLoadDefaults, i18n("Load K3b default settings") );
  QToolTip::add( m_buttonLoadUserDefaults, i18n("Load user default settings") );
  QToolTip::add( m_buttonSaveUserDefaults, i18n("Save user default settings for new projects") );

  // What's This info
  // -------------------------------------------------------------------------
  QWhatsThis::add( m_buttonLoadDefaults, i18n("<p>This sets all options back to K3b defaults.") );
  QWhatsThis::add( m_buttonLoadUserDefaults, i18n("<p>This loads the settings saved with the <em>Save User Defaults</em> "
						  "button.") );
  QWhatsThis::add( m_buttonSaveUserDefaults, i18n("<p>Saves the current settings as the default for all new projects."
						  "<p>These settings can also be loaded with the <em>User Defaults</em> "
						  "button."
						  "<p><b>The K3b defaults are not overwritten by this!</b>") );
}


K3bProjectBurnDialog::~K3bProjectBurnDialog(){
}



void K3bProjectBurnDialog::slotWriterChanged()
{
  if( K3bDevice* dev = m_writerSelectionWidget->writerDevice() ) {
    if( dev->burnproof() )
      m_checkBurnproof->setEnabled( true );
    else {
      m_checkBurnproof->setEnabled( false );
      m_checkBurnproof->setChecked( false );
    }
    if( dev->dao() )
      m_checkDao->setEnabled( true );
    else {
      m_checkDao->setEnabled( false );
      m_checkDao->setChecked( false );
    }
  }
}


int K3bProjectBurnDialog::exec( bool burn )
{
  if( burn && m_job == 0 ) {
    showButton(Ok, true );
    actionButton(Ok)->setDefault(true);
    actionButton(User1)->setDefault(false);
    actionButton(User1)->clearFocus();
  }
  else {
    showButton(Ok, false );
    actionButton(User1)->setDefault(false);
    actionButton(Ok)->setDefault(true);
    actionButton(Ok)->clearFocus();
  }

  readSettings();

  return KDialogBase::exec();
}


void K3bProjectBurnDialog::slotUser1()
{
  saveSettings();
  m_doc->updateAllViews();
  done( Saved );
}


void K3bProjectBurnDialog::slotUser2()
{
  done( Canceled );
}


void K3bProjectBurnDialog::slotCancel()
{
  done( Canceled );
}

void K3bProjectBurnDialog::slotOk()
{
  if( m_job ) {
    KMessageBox::sorry( k3bMain(), i18n("K3b is already working on this project!"), i18n("Error") );
    return;
  }

  saveSettings();

  m_job = m_doc->newBurnJob();

  if( m_writerSelectionWidget )
    m_job->setWritingApp( m_writerSelectionWidget->writingApp() );
  prepareJob( m_job );

  K3bBurnProgressDialog d( k3bMain() );


  d.setJob( m_job );
  m_job->start();

  hide();
  d.exec();


  delete m_job;

  done( Burn );
}


void K3bProjectBurnDialog::prepareGui()
{
  m_tabWidget = new QTabWidget( m_k3bMainWidget );
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

  m_optionGroupLayout->addWidget(m_checkSimulate);
  m_optionGroupLayout->addWidget(m_checkOnTheFly);
  m_optionGroupLayout->addWidget(m_checkDao);
  m_optionGroupLayout->addWidget(m_checkBurnproof);
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
  connect( m_checkOnTheFly, SIGNAL(toggled(bool)), m_checkRemoveBufferFiles, SLOT(setDisabled(bool)) );
  connect( m_checkOnTheFly, SIGNAL(toggled(bool)), m_tempDirSelectionWidget, SLOT(setDisabled(bool)) );

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
