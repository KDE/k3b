/***************************************************************************
                          k3bdivxbasetab.cpp  -  description
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

#include "k3bdivxbasetab.h"
#include "k3bdivxcodecdata.h"
#include "k3bdivxdirectories.h"
#include "k3bdivxavset.h"
#include "k3bdivxavextend.h"
#include "k3bdivxinfo.h"

#include <qlayout.h>
#include <qsizepolicy.h>

#include <kdialog.h>
#include <klocale.h>
#include <kdebug.h>

K3bDivxBaseTab::K3bDivxBaseTab( K3bDivxCodecData *data, QWidget *parent, const char *name ) : QWidget(parent,name) {
    m_data = data;
    setupGui();
}

K3bDivxBaseTab::~K3bDivxBaseTab(){
}

void K3bDivxBaseTab::setupGui(){
    QGridLayout *mainLayout = new QGridLayout( this );
    mainLayout->setSpacing( KDialog::spacingHint() );
    mainLayout->setMargin( 0 );
    m_directories = new K3bDivxDirectories( m_data, this );
    m_avsettings = new K3bDivxAVSet( m_data, this );
    m_avextended = new K3bDivxAVExtend( m_data, this );
    m_info = new K3bDivxInfo( this );
    //QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );

    mainLayout->addMultiCellWidget( m_directories, 0, 0, 0, 0 );
    mainLayout->addMultiCellWidget( m_avsettings, 1, 1, 0, 0 );
    mainLayout->addMultiCellWidget( m_avextended, 1, 1, 1, 1 );
    mainLayout->addMultiCellWidget( m_info, 0, 0, 1, 1 );
    //mainLayout->addItem( spacer, 2, 0);
    //mainLayout->addItem( spacer2, 2, 1);
    mainLayout->setColStretch( 0, 20 );
    mainLayout->setRowStretch( 2, 20 );

    m_avsettings->init();
    //m_avextended->initView();
    m_avsettings->setDisabled( true );
    m_avextended->setDisabled( true );
    connect( m_directories, SIGNAL( dataChanged( ) ), this, SLOT( slotInitView(  ) ));
    connect( m_avextended, SIGNAL( dataChanged( ) ), this, SLOT( slotUpdateView(  ) ));
}

void K3bDivxBaseTab::slotUpdateView(){
    m_avsettings->updateView(  );
}   

void K3bDivxBaseTab::slotInitView(){
    kdDebug() << "(K3bDivxBaseTab::slotUpdateView)" << endl;
    if( m_data->getProjectDir().length() > 1 ){
        m_avsettings->setDisabled( false );
        m_avextended->setDisabled( false );
        m_info->updateData( m_data );
        m_avsettings->updateView(  );
        m_avextended->initView( );
    }
    if( (m_data->getAviFile().length() > 1) && (m_data->getProjectDir().length() > 1) ){
        emit projectLoaded();
    }
}

#include "k3bdivxbasetab.moc"

