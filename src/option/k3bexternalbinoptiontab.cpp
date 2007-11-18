/*
 *
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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


#include "k3bexternalbinoptiontab.h"
#include <k3bexternalbinmanager.h>
#include "k3bexternalbinwidget.h"

#include <kmessagebox.h>
#include <kdialog.h>
#include <klocale.h>
#include <kiconloader.h>
#include <k3listview.h>

#include <q3groupbox.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <q3whatsthis.h>
#include <qfile.h>
#include <q3ptrlist.h>
//Added by qt3to4:
#include <Q3GridLayout>



K3bExternalBinOptionTab::K3bExternalBinOptionTab( K3bExternalBinManager* manager, QWidget* parent )
  : QWidget( parent )
{
  m_manager = manager;

  Q3GridLayout* frameLayout = new Q3GridLayout( this );
  frameLayout->setSpacing( KDialog::spacingHint() );
  frameLayout->setMargin( 0 );

  m_externalBinWidget = new K3bExternalBinWidget( manager, this );
  frameLayout->addWidget( m_externalBinWidget, 1, 0 );

  QLabel* m_labelInfo = new QLabel( this, "m_labelInfo" );
  m_labelInfo->setText( i18n( "Specify the paths to the external programs that K3b needs to work properly, "
			      "or press \"Search\" to let K3b search for the programs." ) );
  m_labelInfo->setScaledContents( false );
  m_labelInfo->setAlignment( int( Qt::WordBreak | Qt::AlignVCenter | Qt::AlignLeft ) );

  frameLayout->addWidget( m_labelInfo, 0, 0 );
}


K3bExternalBinOptionTab::~K3bExternalBinOptionTab()
{
}


void K3bExternalBinOptionTab::readSettings()
{
  m_externalBinWidget->load();
}


void K3bExternalBinOptionTab::saveSettings()
{
  m_externalBinWidget->save();
}


#include "k3bexternalbinoptiontab.moc"
