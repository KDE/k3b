/***************************************************************************
                          k3bdvdbasetab.cpp  -  description
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

#include "k3bdvdbasetab.h"
#include "k3bdvdcodecdata.h"
#include "k3bdvddirectories.h"
#include "k3bdvdavset.h"
#include "k3bdvdavextend.h"
#include "k3bdvdinfo.h"

#include <qlayout.h>
#include <qsizepolicy.h>

#include <kdialog.h>
#include <klocale.h>

K3bDvdBaseTab::K3bDvdBaseTab( K3bDvdCodecData *data, QWidget *parent, const char *name ) : QWidget(parent,name) {
     m_datas = data;
     setupGui();
}

K3bDvdBaseTab::~K3bDvdBaseTab(){
}

void K3bDvdBaseTab::setupGui(){
    QGridLayout *mainLayout = new QGridLayout( this );
    mainLayout->setSpacing( KDialog::spacingHint() );
    mainLayout->setMargin( KDialog::marginHint() );
    m_directories = new K3bDvdDirectories( this );
    m_avsettings = new K3bDvdAVSet( this );
    m_avextended = new K3bDvdAVExtend( this );
    m_info = new K3bDvdInfo( this );
    QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    QSpacerItem* spacer2 = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );

    mainLayout->addMultiCellWidget( m_directories, 0, 0, 0, 0 );
    mainLayout->addMultiCellWidget( m_avsettings, 1, 1, 0, 0 );
    mainLayout->addMultiCellWidget( m_avextended, 1, 1, 1, 1 );
    mainLayout->addMultiCellWidget( m_info, 0, 0, 1, 1 );
    mainLayout->addItem( spacer, 2, 0);
    mainLayout->addItem( spacer2, 2, 1);
    mainLayout->setRowStretch( 2, 20 );
}

#include "k3bdvdbasetab.moc"

