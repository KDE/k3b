/*
 *
 * $Id$
 * Copyright (C) 2003 Thomas Froescher <tfroescher@k3b.org>
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


#include "k3bdivxextsettings.h"
#include "k3bdivxcodecdata.h"

#include <qcheckbox.h>
#include <qlayout.h>
#include <qwhatsthis.h>
//#include <qsizepolicy.h>
//#include <qhbuttongroup.h>
//#include <qradiobutton.h>
#include <qlabel.h>

#include <kdialog.h>
#include <klocale.h>
#include <klineedit.h>
#include <kdebug.h>

K3bDivxExtSettings::K3bDivxExtSettings( K3bDivxCodecData *data, QWidget *parent, const char *name ) 
  : QGroupBox( parent, name ) {
    m_data = data;
    setupGui();
}

K3bDivxExtSettings::~K3bDivxExtSettings() {}

void K3bDivxExtSettings::setupGui() {
    setColumnLayout( 0, Qt::Vertical );
    setTitle( i18n( "Expert Settings" ) );
    layout() ->setMargin( 0 );
    QGridLayout *mainLayout = new QGridLayout( layout() );
    mainLayout->setSpacing( KDialog::spacingHint() );
    mainLayout->setMargin( KDialog::marginHint() );

    m_checkShutdown = new QCheckBox( i18n("Shutdown after encoding process finished"), this );

    // TODO: enable me after message freeze
    QWhatsThis::add( m_checkShutdown, i18n("If enabled, K3b shuts the system down after encoding has finished."
        "This only works in when using Kdm.") );

    m_checkWithoutAudio = new QCheckBox( i18n("Start encoding without detecting normalize parameter for audio"), this);
    m_checkOnlyFirstPass = new QCheckBox( i18n("Do only the first pass of a two-pass encoding"), this);
    m_checkOnlySecondPass = new QCheckBox( i18n("Do only the second pass of a two-pass encoding"), this);
    m_lineTwoPassLog = new KLineEdit( m_data->getProjectDir()+"/tmp/divx4.log", this);

    // TODO
    m_checkOnlyFirstPass->setEnabled( false );
    m_checkOnlySecondPass->setEnabled( false );
    m_lineTwoPassLog->setEnabled( false );

    mainLayout->addMultiCellWidget( m_checkShutdown, 0, 0, 0, 3 );
    mainLayout->addWidget( m_checkWithoutAudio, 1, 0 );
    mainLayout->addWidget( m_checkOnlyFirstPass, 2, 0 );
    mainLayout->addWidget( m_checkOnlySecondPass, 3, 0 );
    mainLayout->addMultiCellWidget( m_lineTwoPassLog, 4, 4, 0, 3 );
    //mainLayout->addMultiCellWidget( transcodemode, 5, 5, 0, 1 );
    //mainLayout->addMultiCellWidget( m_tcDvdModeGroup, 5, 5, 2, 2 );
    //mainLayout->addItem( spacer, 5, 3 );
    mainLayout->setRowStretch(5, 50);
    
    connect( m_checkWithoutAudio, SIGNAL( stateChanged( int ) ), SLOT( slotNoAudio( int ) ) );
    connect( m_checkShutdown, SIGNAL( stateChanged( int ) ), SLOT( slotShutdown( int ) ) );
    //connect( m_tcDvdModeGroup, SIGNAL( clicked( int ) ), SLOT( slotTcDvdModeGroup( int ) ) );
}

void K3bDivxExtSettings::slotUpdateView() {
    m_lineTwoPassLog->setText( m_data->getProjectDir() + "/tmp/divx4.log" );
}

void K3bDivxExtSettings::slotNoAudio( int mode ) {
    if ( mode == 0 ) {
        m_data->setNormalize( true );
    } else {
        m_data->setNormalize( false );
    }
}

void K3bDivxExtSettings::slotShutdown( int mode ) {
    if ( mode == 0 ) {
        m_data->setShutdown( false );
    } else {
        m_data->setShutdown( true );
    }
}




#include "k3bdivxextsettings.moc"

