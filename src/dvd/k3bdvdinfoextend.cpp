/***************************************************************************
                          k3bdvdinfoextend.cpp  -  description
                             -------------------
    begin                : Thu Apr 4 2002
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

#include "k3bdvdinfoextend.h"
#include <qlabel.h>
#include <qlayout.h>
#include <qsizepolicy.h>

#include <klocale.h>

K3bDvdInfoExtend::K3bDvdInfoExtend(QWidget *parent, const char *name ) : K3bDvdInfo(parent,name) {
    setupGui();
}

K3bDvdInfoExtend::~K3bDvdInfoExtend(){
}

void K3bDvdInfoExtend::setupGui(){
    QLabel *quality = new QLabel( i18n("Quality:"), this );
    m_quality = new QLabel( "", this );

   QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );

    m_mainLayout->addMultiCellWidget( quality, 4, 4, 0, 0);
    m_mainLayout->addMultiCellWidget( m_quality, 4, 4, 1, 1);
    m_mainLayout->addItem ( spacer, 5, 0 );
    m_mainLayout->setRowStretch( 5, 20 );
}

#include "k3bdvdinfoextend.moc"
