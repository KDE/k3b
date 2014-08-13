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


#include "k3bbootimagedialog.h"
#include "k3bbootimageview.h"

#include <KI18n/KLocalizedString>


K3b::BootImageDialog::BootImageDialog( K3b::DataDoc* doc, 
					QWidget* parent )
  : KDialog( parent )
{

  setWindowTitle(i18n("Boot Images"));
  setButtons(Ok);
  connect(this,SIGNAL(okClicked()),this,SLOT(slotOk()));
  m_bootImageView = new K3b::BootImageView( doc, this );
  setMainWidget( m_bootImageView );
}


K3b::BootImageDialog::~BootImageDialog()
{
}


void K3b::BootImageDialog::slotOk()
{
  //  m_bootImageView->save();
  done( Ok );
}


