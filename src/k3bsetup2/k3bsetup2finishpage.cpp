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



#include "k3bsetup2finishpage.h"

#include <tools/k3bbusywidget.h>

#include <qlayout.h>
#include <qlabel.h>

#include <klocale.h>
#include <kstandarddirs.h>
#include <kdialog.h>


K3bSetup2FinishPage::K3bSetup2FinishPage( QWidget* parent, const char* name )
  : K3bSetup2Page( 0, i18n("Saving Settings..."), parent, name )
{
  QGridLayout* mainGrid = new QGridLayout( mainWidget() );
  mainGrid->setSpacing( KDialog::spacingHint() );
  mainGrid->setMargin( 0 );

  QLabel* infoLabel = new QLabel( mainWidget() );
  infoLabel->setAlignment( AlignTop | AlignLeft | WordBreak );
  infoLabel->setText( i18n("<qt><p><h1>Congratulations</h1>"
			   "<p>You finished all K3bSetup 2 steps. To apply all tasks you configured "
			   "just press \"finish\" and enjoy using K3b."
			   "<p><b>Caution:</b> If you configured a cdwriting group you should log out and log on again "
			   "to aktivate it. (Sorry for that. I know this should not be necessary in Linux... :(") );

  m_busyWidget = new K3bBusyWidget( mainWidget() );

  mainGrid->addWidget( infoLabel, 0, 0 );
  mainGrid->addRowSpacing( 1, 20 );
  mainGrid->addWidget( m_busyWidget, 2, 0 );
  mainGrid->addRowSpacing( 3, 20 );
}


K3bSetup2FinishPage::~K3bSetup2FinishPage()
{
}


void K3bSetup2FinishPage::showBusy( bool b )
{
  m_busyWidget->showBusy( b );
}



#include "k3bsetup2finishpage.moc"
