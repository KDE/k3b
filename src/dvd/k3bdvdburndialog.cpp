/* 
 *
 * $Id$
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

#include "k3bdvdburndialog.h"
#include "k3bdvddoc.h"

#include <device/k3bdevice.h>
#include <k3bwriterselectionwidget.h>
#include <k3btempdirselectionwidget.h>
#include <k3bcore.h>
#include <tools/k3bwritingmodewidget.h>
#include <k3bglobals.h>

#include <kconfig.h>
#include <klocale.h>
#include <kio/global.h>

#include <qlayout.h>


K3bDvdBurnDialog::K3bDvdBurnDialog( K3bDvdDoc* doc, QWidget *parent, const char *name, bool modal )
  : K3bDataBurnDialog( doc, parent, name, modal ),
    m_doc( doc )
{
  setTitle( i18n("Dvd Project"), i18n("Size: %1").arg( KIO::convertSize(doc->size()) ) );

  // no data mode setting for dvds
  m_groupDataMode->hide();

  m_writerSelectionWidget->setDvd( true );

  readSettings();
}


K3bDvdBurnDialog::~K3bDvdBurnDialog()
{
}


void K3bDvdBurnDialog::toggleAllOptions()
{
  K3bProjectBurnDialog::toggleAllOptions();

  m_writingModeWidget->setSupportedModes( K3b::DAO );
}

#include "k3bdvdburndialog.moc"
