/*
 *
 * $Id$
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

#include "k3bsetupwizard.h"

#include "k3bsetup.h"
#include "k3bsetupwizardtabs.h"

#include <qvariant.h>   // first for gcc 2.7.2
#include <qpushbutton.h>

#include <klocale.h>
#include <kmessagebox.h>



K3bSetupWizard::K3bSetupWizard( K3bSetup* setup, QWidget* parent,  const char* name, bool modal, WFlags fl )
  : KWizard( parent, name, modal, fl ),
    m_accepted(false)
{
  // create the K3bSetup instance
  m_setup = setup;


  setCaption( i18n( "K3b Setup" ) );

  cancelButton()->setText( i18n("Close") );
  cancelButton()->disconnect();
  connect( cancelButton(), SIGNAL(clicked()), this, SLOT(close()) );


  setHelpEnabled(new WelcomeTab( 1, 6, this ), false);
  setHelpEnabled(new ExternalBinTab( 2, 6, this ), false);
  setHelpEnabled(new DeviceTab( 3, 6, this ), false);
  setHelpEnabled(new NoWriterTab( 3, 6, this ), false);
  m_fstabTab = new FstabEntriesTab( 4, 6, this );
  setHelpEnabled( m_fstabTab, false);
  setHelpEnabled(new PermissionTab( 5, 6, this ), false);

  FinishTab * tmpTab = new FinishTab( 6, 6, this );
  setHelpEnabled(tmpTab, false);
  setFinishEnabled( tmpTab, true );

  setSizePolicy( QSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum ) );
}


K3bSetupWizard::~K3bSetupWizard()
{
}


void K3bSetupWizard::showPage( QWidget* page )
{
  ((K3bSetupTab*)page)->aboutToShow();
  KWizard::showPage( page );
}


void K3bSetupWizard::closeEvent( QCloseEvent* e )
{
  if( !m_accepted ) {
    if( KMessageBox::questionYesNo( this,
				    i18n("Do you really want to discard all changes?"),
				  i18n("Close") )
	== KMessageBox::Yes )
      e->accept();
    else
      e->ignore();
  }
  else
    e->accept();
}


void K3bSetupWizard::keyPressEvent( QKeyEvent* e )
{
  if( e->key() == Key_Escape )
    close();   // QDialog calls reject by default which will close our wizard without asking
  else
    KWizard::keyPressEvent( e );
}


bool K3bSetupWizard::appropriate( QWidget* page ) const
{
  return ((K3bSetupTab*)page)->appropriate();
}


void K3bSetupWizard::next()
{
  K3bSetupTab* currentTab = (K3bSetupTab*)currentPage();
  if( currentTab->saveSettings() )
    KWizard::next();
}


void K3bSetupWizard::accept()
{
  m_setup->saveConfig();
  // HACK
  m_fstabTab->writeFstabEntries();

  QString finishMessage = i18n("All settings have been saved.\n");
  finishMessage.append( i18n("If you added new user to your config you may "
			     "need to login again to let the changes take effect.\n") );
  finishMessage.append( i18n("If the configuration of your system changes, "
			     "just run K3b Setup again.\n"
			     "Thank you for using K3b. Have a lot of fun!") );

  KMessageBox::information( this, finishMessage, i18n("K3b Setup Finished") );

  m_accepted = true;

  KWizard::accept();
}

#include "k3bsetupwizard.moc"
