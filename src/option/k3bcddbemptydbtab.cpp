/***************************************************************************
                          k3bcddbemptydbtab.cpp  -  description
                             -------------------
    begin                : Tue Feb 19 2002
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

#include "k3bcddbemptydbtab.h"

#include <qframe.h>
#include <qpushbutton.h>
#include <qlayout.h>

#include <klocale.h>
#include <kapp.h>
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
    QPushButton *selectAll = new QPushButton( i18n("all"), this );
    QPushButton *selectNone = new QPushButton( i18n("none"), this );
}