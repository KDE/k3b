/***************************************************************************
                          k3bdivxextsettings.cpp  -  description
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

#include "k3bdivxextsettings.h"
#include "k3bdivxcodecdata.h"

#include <qcheckbox.h>
#include <qlayout.h>
#include <qwhatsthis.h>
#include <qsizepolicy.h>

#include <kdialog.h>
#include <klocale.h>
#include <klineedit.h>

K3bDivxExtSettings::K3bDivxExtSettings(K3bDivxCodecData *data, QWidget *parent, const char *name ) : QGroupBox(parent,name) {
    m_data = data;
    setupGui();
}

K3bDivxExtSettings::~K3bDivxExtSettings(){
}

void K3bDivxExtSettings::setupGui(){
    setColumnLayout(0, Qt::Vertical );
    setTitle( i18n( "Expert settings" ) );
    layout()->setMargin( 0 );
    QGridLayout *mainLayout = new QGridLayout( layout() );
    mainLayout->setSpacing( KDialog::spacingHint() );
    mainLayout->setMargin( KDialog::marginHint() );

    m_checkShutdown = new QCheckBox( i18n("Shutdown after encoding process finished"), this );
    QWhatsThis::add( m_checkShutdown, i18n("If enabled, K3b shuts the system down after encoding has finished. \
The shutdown command must be setuid to use it as normal user.") );

    m_checkWithoutAudio = new QCheckBox( i18n("Start encoding without detecting normalize parameter for audio."), this);
    m_checkOnlyFirstPass = new QCheckBox( i18n("Do only the first pass of a two-pass encoding."), this);
    m_checkOnlySecondPass = new QCheckBox( i18n("Do only the second pass of a two-pass encoding."), this);
    m_lineTwoPassLog = new KLineEdit( m_data->getProjectDir()+"/tmp/divx4.log", this);

    // TODO
    m_checkShutdown->setEnabled( false );
    m_checkOnlyFirstPass->setEnabled( false );
    m_checkOnlySecondPass->setEnabled( false );
    m_lineTwoPassLog->setEnabled( false );
    
    mainLayout->addWidget( m_checkShutdown, 0, 0);
    mainLayout->addWidget( m_checkWithoutAudio, 1, 0);
    mainLayout->addWidget( m_checkOnlyFirstPass, 2, 0);
    mainLayout->addWidget( m_checkOnlySecondPass, 3, 0);
    mainLayout->addWidget( m_lineTwoPassLog, 4, 0);
    
    QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    mainLayout->addItem( spacer, 5, 0);

    connect( m_checkWithoutAudio, SIGNAL( stateChanged( int )), SLOT( slotNoAudio( int )));
    connect( m_checkShutdown, SIGNAL( stateChanged( int )), SLOT( slotShutdown( int )));
}

void K3bDivxExtSettings::slotUpdateView(){
    m_lineTwoPassLog->setText( m_data->getProjectDir() + "/tmp/divx4.log");
}
    
void K3bDivxExtSettings::slotNoAudio( int mode ){
    if( mode == 0 ){
        m_data->setNormalize( true );
    } else {
        m_data->setNormalize( false );
    }
}

void K3bDivxExtSettings::slotShutdown( int mode ){
    if( mode == 0){
        m_data->setShutdown( false );
    } else {
        m_data->setShutdown( true );
    }
}

#include "k3bdivxextsettings.moc"

