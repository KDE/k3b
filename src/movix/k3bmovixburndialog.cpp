/* 
 *
 * $Id: $
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


#include "k3bmovixburndialog.h"
#include "k3bmovixdoc.h"
#include "k3bmovixoptionswidget.h"

#include <klocale.h>
#include <kdebug.h>


K3bMovixBurnDialog::K3bMovixBurnDialog( K3bMovixDoc* doc, QWidget* parent, const char* name, bool modal )
  : K3bProjectBurnDialog( doc, parent, name, modal ),
    m_doc(doc)
{
  prepareGui();

  m_movixOptionsWidget = new K3bMovixOptionsWidget( m_doc, this );
  addPage( m_movixOptionsWidget, i18n("eMovix Options") );
}


K3bMovixBurnDialog::~K3bMovixBurnDialog()
{
}


void K3bMovixBurnDialog::loadDefaults()
{
}


void K3bMovixBurnDialog::loadUserDefaults()
{
}


void K3bMovixBurnDialog::saveUserDefaults()
{
}


void K3bMovixBurnDialog::saveSettings()
{
}
 

void K3bMovixBurnDialog::readSettings()
{
}


#include "k3bmovixburndialog.moc"
