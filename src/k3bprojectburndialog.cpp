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




K3bProjectBurnDialog::K3bProjectBurnDialog(K3bDoc* doc, QWidget *parent, const char *name, bool modal )
  : KDialogBase( KDialogBase::Tabbed, i18n("Write CD"), User1|User2|Cancel, 
		 User2, parent, name, modal, true, i18n("Write"), i18n("Save") )
{
  m_doc = doc;
	
  setButtonBoxOrientation( Vertical );

  m_job = 0;
}


K3bProjectBurnDialog::~K3bProjectBurnDialog(){
}


int K3bProjectBurnDialog::exec( bool burn )
{
  if( burn && m_job == 0 ) {
    showButton(User1, true );
    actionButton(User1)->setDefault(true);
    actionButton(User2)->setDefault(false);
    actionButton(User2)->clearFocus();
  }
  else {
    showButton(User1, false );
    actionButton(User2)->setDefault(false);
    actionButton(User1)->setDefault(true);
    actionButton(User1)->clearFocus();
  }

  readSettings();
		
  return KDialogBase::exec();
}


void K3bProjectBurnDialog::slotUser2()
{
  saveSettings();
  m_doc->updateAllViews();
  done( Saved );
}


void K3bProjectBurnDialog::slotCancel()
{
  done( Canceled );
}

void K3bProjectBurnDialog::slotUser1()
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
