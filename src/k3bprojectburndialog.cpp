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

#include <qstring.h>
#include <qpushbutton.h>

#include <klocale.h>
#include <kconfig.h>
#include <kmessagebox.h>
#include <kguiitem.h>
#include <kstdguiitem.h>



K3bProjectBurnDialog::K3bProjectBurnDialog(K3bDoc* doc, QWidget *parent, const char *name, bool modal )
  : KDialogBase( KDialogBase::Tabbed, i18n("Write CD"), Ok|User1|User2|User3|Cancel, 
		 User1, parent, name, modal, true, 
		 KGuiItem( i18n("Save"), "filesave", i18n("Save Settings and close"), 
			   i18n("Saves the settings to the project and closes the burn dialog.") ), 
		 KStdGuiItem::defaults(), 
		 KGuiItem( i18n("Save Defaults"), QString::null, i18n("Save current settings as default"),
			   i18n("Saves the current project settings as the default that will be loaded ehen creating a new project.") ) )
{
  m_doc = doc;
	
  setButtonBoxOrientation( Vertical );
  setButtonText( Ok, i18n("Write") );
  m_job = 0;
}


K3bProjectBurnDialog::~K3bProjectBurnDialog(){
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
  loadDefaults();
}


void K3bProjectBurnDialog::slotUser3()
{
  saveDefaults();
}


void K3bProjectBurnDialog::slotCancel()
{
  done( Canceled );
}

void K3bProjectBurnDialog::slotOk()
{
  if( m_job ) {
    KMessageBox::sorry( k3bMain(), i18n("Sorry, K3b is already working on this project!"), i18n("Sorry") );
    return;
  }

  saveSettings();

  m_job = m_doc->newBurnJob();
  //  m_job->setWritingApp( m_writerSelectionWidget->writingApp() );

  K3bBurnProgressDialog d( k3bMain() );
  d.setJob( m_job );

  hide();

  m_job->start();
  d.exec();


  delete m_job;

  done( Burn );
}


#include "k3bprojectburndialog.moc"
