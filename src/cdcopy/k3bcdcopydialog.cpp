/***************************************************************************
                          k3bcdcopydialog.cpp  -  description
                             -------------------
    begin                : Sun Mar 17 2002
    copyright            : (C) 2002 by Sebastian Trueg
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

#include "k3bcdcopydialog.h"

#include <kguiitem.h>
#include <klocale.h>

K3bCdCopyDialog::K3bCdCopyDialog( QWidget *parent, const char *name, bool modal )
  : KDialogBase( Plain, i18n("K3b Cd Copy"), User1|Cancel, User1, parent, name, modal, true, 
		 KGuiItem( i18n("Copy"), "copy", i18n("Start cd copy") )  )
{
}


K3bCdCopyDialog::~K3bCdCopyDialog()
{
}


#include "k3bcdcopydialog.moc"
