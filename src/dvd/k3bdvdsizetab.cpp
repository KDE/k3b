/***************************************************************************
                          k3bdvdsizetab.cpp  -  description
                             -------------------
    begin                : Mon Apr 1 2002
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

#include "k3bdvdsizetab.h"
#include "k3bdvdcodecdata.h"
#include "k3bdvdcrop.h"

#include <qlayout.h>
#include <qsizepolicy.h>

#include <kdialog.h>
#include <klocale.h>


K3bDvdSizeTab::K3bDvdSizeTab(K3bDvdCodecData *data, QWidget *parent, const char *name ) : QWidget(parent,name) {
     m_datas = data;
     setupGui();
}

K3bDvdSizeTab::~K3bDvdSizeTab(){
}

void K3bDvdSizeTab::setupGui(){
    QGridLayout *mainLayout = new QGridLayout( this );
    mainLayout->setSpacing( KDialog::spacingHint() );
    mainLayout->setMargin( KDialog::marginHint() );
    m_crop = new K3bDvdCrop( this );
    mainLayout->addMultiCellWidget( m_crop, 0, 0, 0, 0 );

}

#include "k3bdvdsizetab.moc"
