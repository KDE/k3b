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

#include <kdialog.h>
#include <klocale.h>
#include <ktabctl.h>

K3bDvdView::K3bDvdView( K3bDvdDoc* pDoc, QWidget* parent, const char *name)
    : K3bView( pDoc, parent, name ) {
  m_doc = pDoc;
  setupGui();
}

K3bDvdView::~K3bDvdView(){
}

void K3bDvdView::setupGui(){
    m_codingData = new K3bDvdCodecData();
    QGridLayout *mainLayout = new QGridLayout( this );
    mainLayout->setSpacing( KDialog::spacingHint() );
    mainLayout->setMargin( KDialog::marginHint() );

    KTabCtl *mainTabPool = new KTabCtl( this );
    m_baseTab = new K3bDvdBaseTab( m_codingData, mainTabPool );
    m_sizeTab = new K3bDvdSizeTab( m_codingData, mainTabPool );
    mainTabPool->addTab( m_baseTab, i18n("Codec settings") );
    mainTabPool->addTab( m_sizeTab, i18n("Re/Size settings") );
    mainTabPool->show();

    mainLayout->addWidget( mainTabPool, 0,0 );

}

K3bProjectBurnDialog* K3bDvdView::burnDialog(){
      return 0;
}

#include "k3bdvdview.moc"