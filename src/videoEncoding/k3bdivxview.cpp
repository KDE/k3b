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


#include "k3bdivxview.h"
#include "k3bdivxcodecdata.h"
#include "k3bdivxbasetab.h"
#include "k3bdivxsizetab.h"
#include "k3bdivxadvancedtab.h"
#include "k3bdivxencodingprocess.h"
#include <k3bjobprogressdialog.h>

#include <qlayout.h>
#include <qsizepolicy.h>
#include <qgrid.h>
#include <qfile.h>
#include <qdir.h>
#include <qtabwidget.h>
#include <qpushbutton.h>

#include <kdialog.h>
#include <klocale.h>
#include <ktabctl.h>
#include <kdialogbase.h>
#include <kstdguiitem.h>
#include <kguiitem.h>
#include <kmessagebox.h>
#include <kdebug.h>
#include <kapplication.h>

K3bDivxView::K3bDivxView( QWidget* parent, const char *name)
  : K3bInteractionDialog( parent, name, i18n("Encoding Video") ),
    m_divxJob(0)
{
  m_codingData = new K3bDivxCodecData();
  setupGui();
}

K3bDivxView::K3bDivxView(K3bDivxCodecData *data, QWidget* parent, const char *name)
  : K3bInteractionDialog( parent, name, i18n("Encoding Video") ),
    m_divxJob(0)
{
  m_codingData = data; 
  setupGui();
  m_baseTab->slotInitView();
  slotEnableSizeTab();
}

K3bDivxView::~K3bDivxView(){
    delete m_codingData;
    delete m_advancedTab;
}

void K3bDivxView::setupGui()
{
  setStartButtonText( i18n("Encode"), i18n("Start encoding") );

  QTabWidget* mainTab = new QTabWidget( this );
  setMainWidget( mainTab );
  /*
  QGrid *gridBasic = addGridPage(0, Horizontal, i18n("Basic Audio/Video settings") );
  gridBasic->layout()->setSpacing( KDialog::spacingHint() );
  gridBasic->layout()->setMargin( 0 );//KDialog::marginHint() );

  QGrid *gridSize = addGridPage(1, Horizontal, i18n("Advanced Audio/Video settings") );
  gridSize->layout()->setSpacing( KDialog::spacingHint() );
  gridSize->layout()->setMargin( 0 );//  KDialog::marginHint() );

  QGrid *gridAdvanced = addGridPage(1, Horizontal, i18n("Expert settings") );
  gridAdvanced->layout()->setSpacing( KDialog::spacingHint() );
  gridAdvanced->layout()->setMargin( 0 );//  KDialog::marginHint() );
  */
  QGrid *gridBasic = new QGrid( 0, mainTab );
  mainTab->addTab( gridBasic, i18n("Basic Audio/Video Settings") );
  gridBasic->layout()->setSpacing( spacingHint() );
  gridBasic->layout()->setMargin( marginHint() );

  QGrid *gridSize = new QGrid( 0, mainTab );
  mainTab->addTab( gridSize, i18n("Advanced Audio/Video Settings") );
  gridSize->layout()->setSpacing( spacingHint() );
  gridSize->layout()->setMargin( marginHint() );

  QGrid *gridAdvanced = new QGrid( 0, mainTab );
  mainTab->addTab( gridAdvanced, i18n("Expert Settings") );
  gridAdvanced->layout()->setSpacing( spacingHint() );
  gridAdvanced->layout()->setMargin( marginHint() );
  
  m_baseTab = new K3bDivxBaseTab( m_codingData, gridBasic, "basetab" );
  m_sizeTab = new K3bDivxSizeTab( m_codingData, gridSize, "sizetab");
  m_advancedTab = new K3bDivxAdvancedTab( m_codingData, gridAdvanced, "advancedtab");
//   m_baseTab->setMinimumWidth( 750 );
//   m_baseTab->setMinimumHeight( 500 );

  m_sizeTab->setDisabled( true );
  m_buttonStart->setDisabled( true );

  (QGridLayout(gridBasic->layout())).addWidget( m_baseTab, 0,0 );
  (QGridLayout(gridSize->layout())).addWidget( m_sizeTab, 0,0 );
  (QGridLayout(gridAdvanced->layout())).addWidget( m_advancedTab, 0,0 );

  connect( m_baseTab, SIGNAL( projectLoaded() ), this, SLOT( slotEnableSizeTab() ) );
}

/**
 * 1 - AviFile exists
 * 2 - Avifile is directory
 */
int K3bDivxView::checkSettings()
{
  QDir d( m_codingData->getAviFile() );
  if( d.exists() ){
    KMessageBox::error ( this, i18n("You must choose a filename for the final video."), 
			 i18n("Settings Error") );
    return 2;
  }
  if( QFile::exists( m_codingData->getAviFile() )){
    if( KMessageBox::warningYesNo( this, 
				   i18n("File <%1> already exists. "
					"Do you want to overwrite it?").arg( m_codingData->getAviFile()) ,
				   i18n( "Settings Error" ) ) == KMessageBox::No ) {
      return 1;
    }
  }
  return 0;
}

void K3bDivxView::slotEnableSizeTab()
{
  kdDebug() << "(K3bDivxView::slotEnableSizeTab)" << endl;
  m_sizeTab->setEnabled( true );
  m_advancedTab->slotUpdateView();
  m_buttonStart->setEnabled(true);
}

void K3bDivxView::slotStartClicked()
{
  if( checkSettings() > 0 ){
    return;
  }
  if( !m_divxJob )
    m_divxJob = new K3bDivXEncodingProcess( m_codingData, this );

  K3bJobProgressDialog d( kapp->mainWidget(), "Encoding", true );

  hide();

  d.startJob( m_divxJob );

  close();

  /*
    K3bDivxExtraRipStatus *ripStatus = new K3bDivxExtraRipStatus( m_ripDialog );
    connect( m_ripJob, SIGNAL( dataRate( float )), ripStatus, SLOT( slotDataRate( float )) );
    connect( m_ripJob, SIGNAL( estimatedTime( unsigned int )), ripStatus, SLOT( slotEstimatedTime( unsigned int )) );
    m_ripDialog->setExtraInfo( ripStatus );
  */
}


void K3bDivxView::slotUpdateView()
{
  kdDebug() << "(K3bDivxView::slotUpdateView)" << endl;
  m_baseTab->slotUpdateView();
}

#include "k3bdivxview.moc"
