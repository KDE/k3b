/***************************************************************************
                          k3bdivxsizetab.cpp  -  description
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

#include "k3bdivxsizetab.h"
#include "k3bdivxcodecdata.h"
#include "k3bdivxcrop.h"
#include "k3bdivxresize.h"
//#include "k3bdivxinfoextend.h"

#include <qlayout.h>
#include <qsizepolicy.h>

#include <kdialog.h>
#include <klocale.h>
#include <kmessagebox.h>


K3bDivxSizeTab::K3bDivxSizeTab(K3bDivxCodecData *data, QWidget *parent, const char *name ) : QWidget(parent,name) {
     m_data = data;
     setupGui();
}

K3bDivxSizeTab::~K3bDivxSizeTab(){
}

void K3bDivxSizeTab::setupGui(){
    QGridLayout *mainLayout = new QGridLayout( this );
    mainLayout->setSpacing( KDialog::spacingHint() );
    mainLayout->setMargin( 0 ); //KDialog::marginHint() );
    m_crop = new K3bDivxCrop( m_data, this );
    m_resize = new K3bDivxResize( m_data, this );

    //mainLayout->addMultiCellWidget( m_info, 0, 1, 0, 0 );
    mainLayout->addMultiCellWidget( m_crop, 0, 1, 0, 1 );
    mainLayout->addMultiCellWidget( m_resize, 2, 2, 0, 1 );
    mainLayout->setColStretch( 1, 50 );
    mainLayout->setRowStretch( 0, 50 );
    //mainLayout->addItem( spacer, 3, 0);
    connect( m_resize, SIGNAL( sizeChanged() ), m_crop, SLOT( slotUpdateFinalSize() ) );
}

void K3bDivxSizeTab::show(){
    QWidget::show();
    if( this->isEnabled() ){
        m_crop->initPreview( );
        m_resize->initView();
    } else {
        KMessageBox::information( this, i18n("You must load a K3b DVD Project file and \n set a file name for the final AVI.") );
    }
}

void K3bDivxSizeTab::updateView(){
    m_crop->updateView();
}

void K3bDivxSizeTab::resetView(){
    m_crop->resetView();
    m_resize->resetView();
}
#include "k3bdivxsizetab.moc"
