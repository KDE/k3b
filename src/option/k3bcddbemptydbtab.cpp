/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2004 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3bcddbemptydbtab.h"

#include <qframe.h>
#include <qpushbutton.h>
#include <qlayout.h>

#include <klocale.h>
#include <kapplication.h>
#include <kdialog.h>

K3bCddbEmptyDbTab::K3bCddbEmptyDbTab(QWidget *parent, const char *name ) : QWidget(parent,name) {
    setup();
}
K3bCddbEmptyDbTab::~K3bCddbEmptyDbTab(){
}

void K3bCddbEmptyDbTab::setup(){
    QGridLayout* frameLayout = new QGridLayout( this );
    frameLayout->setSpacing( KDialog::spacingHint() );
    frameLayout->setMargin( KDialog::marginHint() );

    QFrame *line = new QFrame( this );
    QPushButton *selectAll = new QPushButton( i18n("All"), this );
    QPushButton *selectNone = new QPushButton( i18n("None"), this );
}

#include "k3bcddbemptydbtab.moc"
