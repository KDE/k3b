/***************************************************************************
                          k3bmiscoptiontab.cpp  -  description
                             -------------------
    begin                : Tue Dec 18 2001
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

#include "k3bmiscoptiontab.h"
#include "../k3b.h"

#include <qcheckbox.h>
#include <qlayout.h>

#include <klocale.h>
#include <kconfig.h>
#include <kdialog.h>


K3bMiscOptionTab::K3bMiscOptionTab(QWidget *parent, const char *name ) 
  : QWidget(parent,name) 
{
  m_checkShowSplash = new QCheckBox( i18n("Show splash screen"), this );

  QGridLayout* mainGrid = new QGridLayout( this );
  mainGrid->setSpacing( KDialog::spacingHint() );
  mainGrid->setMargin( KDialog::marginHint() );

  mainGrid->addWidget( m_checkShowSplash, 0, 0 );
}


K3bMiscOptionTab::~K3bMiscOptionTab()
{
}


void K3bMiscOptionTab::readSettings()
{
  KConfig* c = k3bMain()->config();
  c->setGroup( "General Options" );
  m_checkShowSplash->setChecked( c->readBoolEntry("Show splash", true) );
}


void K3bMiscOptionTab::saveSettings()
{
  KConfig* c = k3bMain()->config();
  c->setGroup( "General Options" );
  c->writeEntry( "Show splash", m_checkShowSplash->isChecked() );
}

#include "k3bmiscoptiontab.moc"
