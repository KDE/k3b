/***************************************************************************
                          k3bdvdinfo.cpp  -  description
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

#include "k3bdivxinfo.h"
#include "k3bdivxcodecdata.h"
#include <qlabel.h>
#include <qslider.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <qframe.h>

#include <kdialog.h>
#include <klocale.h>
#include <klineedit.h>
#include <kcombobox.h>

K3bDivxInfo::K3bDivxInfo(QWidget *parent, const char *name ) : QGroupBox(parent,name) {
     setupGui();
}

K3bDivxInfo::~K3bDivxInfo(){
}

void K3bDivxInfo::setupGui(){
    setColumnLayout(0, Qt::Vertical );
    setTitle( i18n( "DVD Information" ) );
    layout()->setMargin( 0 );
    m_mainLayout = new QGridLayout( layout() );
    m_mainLayout->setSpacing( KDialog::spacingHint() );
    m_mainLayout->setMargin( KDialog::marginHint() );

    QLabel *length = new QLabel( i18n("Play length:"), this );
    QLabel *frames = new QLabel( i18n("Frames:"), this );
    QLabel *fps = new QLabel( i18n("Framerate:"), this );
    QLabel *size = new QLabel( i18n("Video size:"), this );
    QLabel *aspect = new QLabel( i18n("Aspect ratio:"), this );
    QLabel *tv = new QLabel( i18n("TV norm:"), this );

    m_length = new QLabel( "", this );
    m_frames = new QLabel( "", this );
    m_size = new QLabel( "", this );
    m_aspect = new QLabel( "", this );

    QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );

    m_mainLayout->addMultiCellWidget( length, 0, 0, 0, 0);
    m_mainLayout->addMultiCellWidget( m_length, 0, 0, 1, 1);
    m_mainLayout->addMultiCellWidget( frames, 1, 1, 0, 0);
    m_mainLayout->addMultiCellWidget( m_frames, 1, 1, 1, 1);
    m_mainLayout->addMultiCellWidget( size, 2, 2, 0, 0);
    m_mainLayout->addMultiCellWidget( m_size, 2, 2, 1, 1);
    m_mainLayout->addMultiCellWidget( aspect, 3, 3, 0, 0);
    m_mainLayout->addMultiCellWidget( m_aspect, 3, 3, 1, 1);
    m_mainLayout->addMultiCellWidget( tv, 4, 4, 0, 0);
    m_mainLayout->addMultiCellWidget( fps, 5, 5, 0, 0);
    m_mainLayout->addItem( spacer, 0, 2);

}

void K3bDivxInfo::updateData( K3bDivxCodecData *data ){
     m_frames->setText( data->getFrames() );
     m_length->setText( data->getLength() );
     m_size->setText( data->getSize() );
     m_aspect->setText( data->getAspectRatio() );
}

