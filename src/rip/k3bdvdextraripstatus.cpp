/***************************************************************************
                          k3bdvdextraripstatus.cpp  -  description
                             -------------------
    begin                : Sun Mar 31 2002
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

#include "k3bdvdextraripstatus.h"

#include <qlabel.h>
#include <qdatetime.h>
#include <qgroupbox.h>
#include <qlayout.h>

#include <klocale.h>
#include <kdialog.h>

K3bDvdExtraRipStatus::K3bDvdExtraRipStatus(QWidget *parent, const char *name ) : QWidget(parent,name) {
    setupGui();
}

K3bDvdExtraRipStatus::~K3bDvdExtraRipStatus(){
}

void K3bDvdExtraRipStatus::setupGui(){

    QGridLayout *mainLayout = new QGridLayout( this );

    QGroupBox *groupPattern = new QGroupBox( i18n( "Information" ), this, "pattern" );
    groupPattern->setColumnLayout(0, Qt::Vertical );
    QGridLayout *infoLayout = new QGridLayout( groupPattern->layout() );
    infoLayout->setSpacing( KDialog::spacingHint() );
    infoLayout->setMargin( KDialog::marginHint() );
    QLabel *datarate = new QLabel( i18n("Data rate: "), groupPattern );
    m_rate = new QLabel( groupPattern );
    QLabel *estimatetime = new QLabel( i18n("Estimated time: "), groupPattern );
    m_estimatetime = new QLabel( groupPattern );

    infoLayout->addMultiCellWidget( datarate, 0, 0, 0, 0);
    infoLayout->addMultiCellWidget( m_rate, 0, 0, 1, 1);
    infoLayout->addMultiCellWidget( estimatetime, 0, 0, 2, 2);
    infoLayout->addMultiCellWidget( m_estimatetime, 0, 0, 3, 3);

    mainLayout->addWidget( groupPattern, 0, 0 );
}

void K3bDvdExtraRipStatus::slotDataRate( float rate ){
     m_rate->setText( QString().sprintf("%.2f MB/s", rate ) );
}

void K3bDvdExtraRipStatus::slotEstimatedTime( unsigned int secs ){
     QDateTime time;
     time = time.addSecs( secs );
     m_estimatetime->setText( time.toString("hh:mm:ss") );
}

#include "k3bdvdextraripstatus.moc"
