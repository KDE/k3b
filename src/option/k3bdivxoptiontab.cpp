/* 
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Thomas Froescher <tfroescher@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3bdivxoptiontab.h"

#include <kconfig.h>
#include <kapp.h>
#include <kcombobox.h>
#include <knuminput.h>

#include <qcheckbox.h>
#include <qbuttongroup.h>
#include <qbutton.h>


K3bDivxOptionTab::K3bDivxOptionTab(QWidget* parent,  const char* name )
  : base_K3bDivxOptions( parent, name )
{
}


K3bDivxOptionTab::~K3bDivxOptionTab()
{
}

void K3bDivxOptionTab::readSettings()
{
  KConfig* c = kapp->config();
  c->setGroup( "Divx" );
  m_buttonAutoQuality->setButton( c->readNumEntry("quality mode", 2 ) );
  m_inputWidth->setValue( c->readNumEntry( "width", 640 ));
  m_inputAviSize->setValue( c->readNumEntry( "avi size", 700 ));
  m_inputVideoQuality->setValue( c->readDoubleNumEntry( "video quality", 0.20 ));
}


void K3bDivxOptionTab::saveSettings()
{
  KConfig* c = kapp->config();
  c->setGroup( "Divx" );
  c->writeEntry("quality mode", m_buttonAutoQuality->id( m_buttonAutoQuality->selected() ) );
  c->writeEntry("width", m_inputWidth->value() );
  c->writeEntry("avi size", m_inputAviSize->value() );
  c->writeEntry("video quality", m_inputVideoQuality->value() );
}

#include "k3bdivxoptiontab.moc"
