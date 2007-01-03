/* 
 *
 * $Id: sourceheader 511311 2006-02-19 14:51:05Z trueg $
 * Copyright (C) 2006 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bfirstrun.h"
#include "k3b.h"
#include "k3bservicemenuinstaller.h"
#include <k3bcore.h>

#include <klocale.h>
#include <kiconloader.h>
#include <kconfig.h>

#include <qlayout.h>
#include <qlabel.h>


void K3bFirstRun::run( QWidget* parent )
{
  if( !k3bcore->config()->readBoolEntry( "First run", true ) )
    return;

  k3bcore->config()->writeEntry( "First run", false );
    
  // for now the first run dialog only asks for
  // the konqui integration. So in case it is 
  // already installed there is no need to show the
  // dialog.
  K3bServiceInstaller si;
  if( si.allInstalled() )
    return;

  K3bFirstRun dlg( parent );
  if( dlg.exec() == QDialog::Accepted )
    si.install( parent );
}


K3bFirstRun::K3bFirstRun( QWidget* parent )
  : KDialogBase( Plain,
		 i18n("First Run"),
		 Ok|Cancel,
		 Ok,
		 parent,
		 0,
		 true,
		 true )
{
  setButtonOK( i18n("Enable Konqueror integration") );
  setButtonCancel( i18n("No Konqueror integration") );

  QFrame* plain = plainPage();
  QLabel* label = new QLabel( i18n("<p>K3b can integrate itself into Konqueror. This integration "
				   "allows to start K3b from the context menu in the file manager."
				   "<p><em>The Konqueror integration can always be disabled and "
				   "enabled again from the K3b settings.</em>"), plain );
  QLabel* pixLabel = new QLabel( plain );
  pixLabel->setPixmap( DesktopIcon( "konqueror" ) );

  QHBoxLayout* lay = new QHBoxLayout( plain );
  lay->setMargin( 0 );
  lay->setSpacing( spacingHint() );
  lay->addWidget( pixLabel );
  lay->addWidget( label );
  lay->setStretchFactor( label, 1 );
}


K3bFirstRun::~K3bFirstRun()
{
}

#include "k3bfirstrun.moc"
