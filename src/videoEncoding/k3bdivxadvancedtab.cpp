/* 
 *
 * $Id$
 * Copyright (C) 2003 Thomas Froescher <tfroescher@k3b.org>
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

/***************************************************************************
                          k3bdivxadvancedtab.cpp  -  description
                             -------------------
    begin                : Tue Jul 30 2002
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

#include "k3bdivxadvancedtab.h"
#include "k3bdivxcodecdata.h"
#include "k3bdivxextsettings.h"

#include <qlayout.h>

#include <kdialog.h>

K3bDivxAdvancedTab::K3bDivxAdvancedTab(K3bDivxCodecData *data, QWidget *parent, const char *name ) : QWidget(parent,name) {
    m_data = data;
    setupGui();
}
K3bDivxAdvancedTab::~K3bDivxAdvancedTab(){
}

void K3bDivxAdvancedTab::setupGui(){
    QGridLayout *mainLayout = new QGridLayout( this );
    mainLayout->setSpacing( KDialog::spacingHint() );
    mainLayout->setMargin( 0 ); //KDialog::marginHint() );
    m_extension = new K3bDivxExtSettings( m_data, this, "K3bDivxAdvancedTab:K3bDivxExtSettings" );

    mainLayout->addWidget( m_extension, 0, 0);
    mainLayout->setRowStretch( 0, 50 );
}

void K3bDivxAdvancedTab::slotUpdateView(){
    m_extension->slotUpdateView();
}

#include "k3bdivxadvancedtab.moc"
