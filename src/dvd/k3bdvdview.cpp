/***************************************************************************
                          k3bdvdview.cpp  -  description
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

#include "k3bdvdview.h"
#include "k3bdvddoc.h"
#include "k3bdvdcodecdata.h"
#include "k3bdvdbasetab.h"
#include "k3bdvdsizetab.h"

#include <qlayout.h>
#include <qsizepolicy.h>
#include <qgrid.h>

#include <kdialog.h>
#include <klocale.h>
#include <ktabctl.h>
#include <kdialogbase.h>
#include <kstdguiitem.h>
#include <kguiitem.h>

K3bDvdView::K3bDvdView( QWidget* parent, const char *name)
    : KDialogBase( KDialogBase::Tabbed, i18n("Encoding Video"), User1|User2,
		 User1, 0, 0, true, false, KGuiItem( i18n("Encode"), "encode", i18n("Start encoding") ), KStdGuiItem::close() ){

           //KDialogBase::Close|KDialogBase::Apply, KDialogBase::Apply, parent, name ) {

  setButtonBoxOrientation( Qt::Vertical );
  setupGui();
}

K3bDvdView::~K3bDvdView(){
}

void K3bDvdView::setupGui(){
    setMinimumWidth( 500 );
    QGrid *gridBasic = addGridPage(0, Horizontal, i18n("Basic Audio/Video settings") );
    QGridLayout *basicLayout = new QGridLayout( gridBasic );
    basicLayout->setSpacing( KDialog::spacingHint() );
    basicLayout->setMargin( KDialog::marginHint() );
    QGrid *gridSize = addGridPage(1, Horizontal, i18n("Advanced Audio/Video settings") );
    QGridLayout *sizeLayout = new QGridLayout( gridSize );
    sizeLayout->setSpacing( KDialog::spacingHint() );
    sizeLayout->setMargin( KDialog::marginHint() );

    m_codingData = new K3bDvdCodecData();

    m_baseTab = new K3bDvdBaseTab( m_codingData, gridBasic, "basetab" );
    m_sizeTab = new K3bDvdSizeTab( m_codingData, gridSize, "sizetab");
    //mainTabPool->addTab( m_baseTab, i18n("Codec settings") );
    //mainTabPool->addTab( m_sizeTab, i18n("Re/Size settings") );
    //mainTabPool->show();

    basicLayout->addWidget( m_baseTab, 0,0 );
    sizeLayout->addWidget( m_sizeTab, 0,0 );

}

void K3bDvdView::slotUser1(){
}

void K3bDvdView::slotUser2(){
    slotClose();
}

#include "k3bdvdview.moc"