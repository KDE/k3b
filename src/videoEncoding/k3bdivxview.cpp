/***************************************************************************
                          k3bdivxview.cpp  -  description
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

#include "k3bdivxview.h"
#include "k3bdivxcodecdata.h"
#include "k3bdivxbasetab.h"
#include "k3bdivxsizetab.h"
#include "k3bdivxencodingprocess.h"
#include "../k3bburnprogressdialog.h"

#include <qlayout.h>
#include <qsizepolicy.h>
#include <qgrid.h>
#include <qfile.h>
#include <qdir.h>

#include <kdialog.h>
#include <klocale.h>
#include <ktabctl.h>
#include <kdialogbase.h>
#include <kstdguiitem.h>
#include <kguiitem.h>
#include <kmessagebox.h>

K3bDivxView::K3bDivxView( QWidget* parent, const char *name)
    : KDialogBase( KDialogBase::Tabbed, i18n("Encoding Video"), User1|User2,
		 User1, 0, 0, true, false, KGuiItem( i18n("Encode"), "encode", i18n("Start encoding") ), KStdGuiItem::close() ){

           //KDialogBase::Close|KDialogBase::Apply, KDialogBase::Apply, parent, name ) {

  setButtonBoxOrientation( Qt::Vertical );
  setupGui();
}

K3bDivxView::~K3bDivxView(){
}

void K3bDivxView::setupGui(){
    QGrid *gridBasic = addGridPage(0, Horizontal, i18n("Basic Audio/Video settings") );
    //QGridLayout *basicLayout = new QGridLayout( gridBasic );
    gridBasic->layout()->setSpacing( KDialog::spacingHint() );
    //basicLayout->setMargin( KDialog::marginHint() );
    gridBasic->layout()->setMargin( 0 );//KDialog::marginHint() );

    QGrid *gridSize = addGridPage(1, Horizontal, i18n("Advanced Audio/Video settings") );
    //QGridLayout *sizeLayout = new QGridLayout( gridSize );
    gridSize->layout()->setSpacing( KDialog::spacingHint() );
    gridSize->layout()->setMargin( 0 );//  KDialog::marginHint() );

    m_codingData = new K3bDivxCodecData();

    m_baseTab = new K3bDivxBaseTab( m_codingData, gridBasic, "basetab" );
    m_sizeTab = new K3bDivxSizeTab( m_codingData, gridSize, "sizetab");
    m_baseTab->setMinimumWidth( 750 );
    m_baseTab->setMinimumHeight( 500 );

    m_sizeTab->setDisabled( true );
    enableButton( KDialogBase::User1, false );

    (QGridLayout(gridBasic->layout())).addWidget( m_baseTab, 0,0 );
    (QGridLayout(gridBasic->layout())).addWidget( m_sizeTab, 0,0 );

    connect( m_baseTab, SIGNAL( projectLoaded() ), this, SLOT( slotEnableSizeTab() ) );
}

/**
* 1 - AviFile exists
* 2 - Avifile is directory
*/
int K3bDivxView::checkSettings(){
    QDir d( m_codingData->getAviFile() );
    if( d.exists() ){
        KMessageBox::error ( this, i18n("You must choose an filename for the final video."), i18n("Settings error") );
        return 2;
    }
    if( QFile::exists( m_codingData->getAviFile() )){
        if( KMessageBox::No == KMessageBox::warningYesNo ( this, i18n("File <%1> already exists. Do you want overwrite it?").arg( m_codingData->getAviFile()) ,i18n( "Settings error" ) )){
            return 1;
        }
    }
    return 0;
}

void K3bDivxView::slotEnableSizeTab(){
    m_sizeTab->setEnabled( true );
    m_sizeTab->resetView();
    m_sizeTab->updateView();
    enableButton( KDialogBase::User1, true );
}

void K3bDivxView::slotUser1(){
    if( checkSettings() > 0 ){
         return;
    }
    m_divxJob = new K3bDivXEncodingProcess( m_codingData, this );

    m_divxDialog = new K3bBurnProgressDialog( this, "Encoding", true );
    m_divxDialog->setCaption( i18n("Encoding process") );
    m_divxDialog->setJob( m_divxJob );

    /*
    K3bDivxExtraRipStatus *ripStatus = new K3bDivxExtraRipStatus( m_ripDialog );
    connect( m_ripJob, SIGNAL( dataRate( float )), ripStatus, SLOT( slotDataRate( float )) );
    connect( m_ripJob, SIGNAL( estimatedTime( unsigned int )), ripStatus, SLOT( slotEstimatedTime( unsigned int )) );
    m_ripDialog->setExtraInfo( ripStatus );
    */
    m_divxJob->start();
    m_divxDialog->exec();
}

void K3bDivxView::slotUser2(){
    slotClose();
}

#include "k3bdivxview.moc"
