/***************************************************************************
                          k3bvcdburndialog.cpp  -  description
                             -------------------
    begin                : Son Nov 10 2002
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

#include "k3bvcdburndialog.h"
#include "../k3b.h"
#include "k3bvcddoc.h"
#include "../device/k3bdevice.h"
#include "../k3bwriterselectionwidget.h"
#include "../k3btempdirselectionwidget.h"

#include <qcheckbox.h>
#include <qcombobox.h>
#include <qgroupbox.h>
#include <qspinbox.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qmultilineedit.h>
#include <qpushbutton.h>
#include <qtabwidget.h>
#include <qlayout.h>
#include <qvariant.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qgrid.h>
#include <qtoolbutton.h>
#include <qptrlist.h>
#include <qstringlist.h>
#include <qpoint.h>
#include <qtabwidget.h>

#include <klocale.h>
#include <kstandarddirs.h>
#include <kconfig.h>
#include <kmessagebox.h>


K3bVcdBurnDialog::K3bVcdBurnDialog(K3bVcdDoc* _doc, QWidget *parent, const char *name, bool modal )
  : K3bProjectBurnDialog( _doc, parent, name, modal )
{
  QTabWidget* tab = new QTabWidget( k3bMainWidget() );
  QFrame* f1 = new QFrame( tab );
  QFrame* f2 = new QFrame( tab );
  QFrame* f3 = new QFrame( tab );
  
  tab->addTab( f1, i18n("Burning") );
  tab->addTab( f2, i18n("VideoCD") );
  tab->addTab( f3, i18n("Label") );
  
  setupBurnTab( f1 );
  setupVideoCdTab( f2 );
  setupLabelTab( f3 );
    
  // connect( m_checkOnTheFly, SIGNAL(toggled(bool)), m_tempDirSelectionWidget, SLOT(setDisabled(bool)) );
  // connect( m_checkOnTheFly, SIGNAL(toggled(bool)), m_checkRemoveBufferFiles, SLOT(setDisabled(bool)) );

  m_tempDirSelectionWidget->setNeededSize( doc()->size() );

  loadDefaults();
  readSettings();
}


K3bVcdBurnDialog::~K3bVcdBurnDialog()
{
}


void K3bVcdBurnDialog::setupBurnTab( QFrame* frame )
{
  QGridLayout* frameLayout = new QGridLayout( frame );
  frameLayout->setSpacing( spacingHint() );
  frameLayout->setMargin( marginHint() );


  // ---- options group ------------------------------------------------
  QGroupBox* m_groupOptions = new QGroupBox( frame, "m_groupOptions" );
  m_groupOptions->setTitle( i18n( "Options" ) );
  m_groupOptions->setColumnLayout(0, Qt::Vertical );
  m_groupOptions->layout()->setSpacing( 0 );
  m_groupOptions->layout()->setMargin( 0 );
  QVBoxLayout* m_groupOptionsLayout = new QVBoxLayout( m_groupOptions->layout() );
  m_groupOptionsLayout->setAlignment( Qt::AlignTop );
  m_groupOptionsLayout->setSpacing( spacingHint() );
  m_groupOptionsLayout->setMargin( marginHint() );

  // m_checkDao = new QCheckBox( m_groupOptions, "m_checkDao" );
  // m_checkDao->setText( i18n( "Disk at once" ) );

  m_checkSimulate = new QCheckBox( m_groupOptions, "m_checkSimulate" );
  m_checkSimulate->setText( i18n( "Simulate Writing" ) );

  // m_checkOnTheFly = new QCheckBox( m_groupOptions, "m_checkOnTheFly" );
  // m_checkOnTheFly->setText( i18n( "Writing on the fly" ) );

  m_checkRemoveBufferFiles = new QCheckBox( m_groupOptions, "m_checkRemoveBufferFiles" );
  m_checkRemoveBufferFiles->setText( i18n("Remove image files") );

  m_groupOptionsLayout->addWidget( m_checkSimulate );
  // m_groupOptionsLayout->addWidget( m_checkOnTheFly );
  m_groupOptionsLayout->addWidget( m_checkRemoveBufferFiles );
  // m_groupOptionsLayout->addWidget( m_checkDao );
  // --------------------------------------------------- options group ---

  m_tempDirSelectionWidget = new K3bTempDirSelectionWidget( frame );
  m_writerSelectionWidget = new K3bWriterSelectionWidget( frame );

  frameLayout->addWidget( m_tempDirSelectionWidget, 1, 1 );
  frameLayout->addWidget( m_groupOptions, 1, 0 );
  frameLayout->addMultiCellWidget( m_writerSelectionWidget, 0, 0, 0, 1 );

  frameLayout->setRowStretch( 1, 1 );
  frameLayout->setColStretch( 1, 1 );
}

void K3bVcdBurnDialog::setupVideoCdTab( QFrame* frame )
{

  QGridLayout* frameLayout = new QGridLayout( frame );
  frameLayout->setSpacing( spacingHint() );
  frameLayout->setMargin( marginHint() );

  // ---- VcdFormat group ------------------------------------------------
  QGroupBox* m_groupVcdFormat = new QButtonGroup( frame, "m_groupVcdFormat" );
  m_groupVcdFormat->setTitle( i18n( "Format" ) );
  m_groupVcdFormat->setColumnLayout(0, Qt::Vertical );
  m_groupVcdFormat->layout()->setSpacing( 0 );
  m_groupVcdFormat->layout()->setMargin( 0 );
  QVBoxLayout* m_groupVcdFormatLayout = new QVBoxLayout( m_groupVcdFormat->layout() );
  m_groupVcdFormatLayout->setAlignment( Qt::AlignTop );
  m_groupVcdFormatLayout->setSpacing( spacingHint() );
  m_groupVcdFormatLayout->setMargin( marginHint() );

  m_checkVcd11 = new QRadioButton( m_groupVcdFormat, "m_checkVcd11" );
  m_checkVcd11->setText( i18n( "VideoCD 1.1" ) );
  m_checkVcd20 = new QRadioButton( m_groupVcdFormat, "m_checkVcd20" );
  m_checkVcd20->setText( i18n( "VideoCD 2.0" ) );
  m_checkSvcd10 = new QRadioButton( m_groupVcdFormat, "m_checkSvcd10" );
  m_checkSvcd10->setText( i18n( "Super-VideoCD" ) );

  m_groupVcdFormatLayout->addWidget( m_checkVcd11 );
  m_groupVcdFormatLayout->addWidget( m_checkVcd20 );
  m_groupVcdFormatLayout->addWidget( m_checkSvcd10 );    
  // --------------------------------------------------- VcdFormat group ---

  // ---- option group ------------------------------------------------
  QGroupBox* m_groupOptions = new QGroupBox( frame, "m_groupOptions" );
  m_groupOptions->setTitle( i18n( "Options" ) );
  m_groupOptions->setColumnLayout(0, Qt::Vertical );
  m_groupOptions->layout()->setSpacing( 0 );
  m_groupOptions->layout()->setMargin( 0 );
  QVBoxLayout* m_groupOptionsLayout = new QVBoxLayout( m_groupOptions->layout() );
  m_groupOptionsLayout->setAlignment( Qt::AlignTop );
  m_groupOptionsLayout->setSpacing( spacingHint() );
  m_groupOptionsLayout->setMargin( marginHint() );

  m_checkNonCompliant = new QCheckBox( m_groupOptions, "m_checkNonCompliant" );
  m_checkNonCompliant->setText( i18n( "Non-compliant compatibility mode for broken devices" ) );
  m_check2336 = new QCheckBox( m_groupOptions, "m_check2336" );
  m_check2336->setText( i18n( "Use 2336 byte sectors for output" ) );

  m_groupOptionsLayout->addWidget( m_checkNonCompliant );
  m_groupOptionsLayout->addWidget( m_check2336 );
  // --------------------------------------------------- Options group ---

  // ---------------------------------------------------------------------

  frameLayout->addWidget( m_groupVcdFormat, 0, 0 );
  frameLayout->addWidget( m_groupOptions, 0, 1 );

  frameLayout->setRowStretch( 1, 1 );
  frameLayout->setColStretch( 1, 1 );

 
}

void K3bVcdBurnDialog::setupLabelTab( QFrame* frame )
{

  QGridLayout* mainGrid = new QGridLayout( frame );
  mainGrid->setSpacing( spacingHint() );
  mainGrid->setMargin( marginHint() );

  m_checkApplicationId = new QCheckBox( i18n( "Write Application Id" ), frame, "m_checkApplicationId" );

  QLabel* labelVolume = new QLabel( i18n( "ISO &Volume Label:" ), frame, "labelVolume" );
  QLabel* labelAlbumId = new QLabel( i18n( "&Album Id:" ), frame, "labelAlbumId" );
  QLabel* labelVolumeCount = new QLabel( i18n( "Number of CDs in &Album:" ), frame, "labelVolumeCount" );
  QLabel* labelVolumeNumber = new QLabel( i18n( "CD is &Number:" ), frame, "labelVolumeNumber" );

  m_editVolume = new QLineEdit( frame, "m_editDisc_id" );
  m_editAlbumId = new QLineEdit( frame, "m_editAlbumId" );
  m_spinVolumeCount = new QSpinBox( frame, "m_editVolumeCount" );
  m_spinVolumeNumber = new QSpinBox( frame, "m_editVolumeNumber" );
  
  QFrame* line = new QFrame( frame );
  line->setFrameStyle( QFrame::HLine | QFrame::Sunken );

  mainGrid->addMultiCellWidget( m_checkApplicationId, 0, 0, 0, 1 );
  mainGrid->addMultiCellWidget( line, 1, 1, 0, 1 );

  mainGrid->addWidget( labelVolume, 2, 0 );
  mainGrid->addWidget( m_editVolume, 2, 1 );
  mainGrid->addWidget( labelAlbumId, 3, 0 );
  mainGrid->addWidget( m_editAlbumId, 3, 1 );
  mainGrid->addWidget( labelVolumeCount, 4, 0 );
  mainGrid->addWidget( m_spinVolumeCount, 4, 1 );
  mainGrid->addWidget( labelVolumeNumber, 5, 0 );
  mainGrid->addWidget( m_spinVolumeNumber, 5, 1 );

  mainGrid->addRowSpacing( 5, 15 );
  mainGrid->setRowStretch( 5, 1 );

  // buddies
  labelVolume->setBuddy( m_editVolume );
  labelAlbumId->setBuddy( m_editAlbumId );
  labelVolumeCount->setBuddy( m_spinVolumeCount );
  labelVolumeNumber->setBuddy( m_spinVolumeNumber );

  // tab order
  setTabOrder( m_editVolume, m_editAlbumId);
  setTabOrder( m_editAlbumId, m_spinVolumeCount );
  setTabOrder( m_spinVolumeCount, m_spinVolumeNumber );

}


void K3bVcdBurnDialog::slotOk()
{
  // check if enough space in tempdir if not on-the-fly
  if( doc()->size()/1024 > m_tempDirSelectionWidget->freeTempSpace() )
    KMessageBox::sorry( this, i18n("Not enough space in temporary directory.") );
  else
    K3bProjectBurnDialog::slotOk();
}


void K3bVcdBurnDialog::loadDefaults()
{
  m_checkSimulate->setChecked( false );
  // m_checkDao->setChecked( true );
  // m_checkOnTheFly->setChecked( false );
  // m_checkBurnProof->setChecked( true );
  m_checkRemoveBufferFiles->setChecked( true );

  m_checkApplicationId->setChecked( true );
  m_editVolume->setText( "VIDEOCD" );
  m_checkVcd20->setChecked( true );
  
}

void K3bVcdBurnDialog::saveSettings()
{
  doc()->setTempDir( m_tempDirSelectionWidget->tempPath() );
  // doc()->setDao( m_checkDao->isChecked() );
  doc()->setDao( true );
  doc()->setDummy( m_checkSimulate->isChecked() );
  // doc()->setOnTheFly( m_checkOnTheFly->isChecked() );
  doc()->setOnTheFly( false );
  // ((K3bVcdDoc*)doc())->setRemoveBufferFiles( m_checkRemoveBufferFiles->isChecked() );

  // -- saving current speed --------------------------------------
  doc()->setSpeed( m_writerSelectionWidget->writerSpeed() );

  // -- saving current device --------------------------------------
  doc()->setBurner( m_writerSelectionWidget->writerDevice() );
}


void K3bVcdBurnDialog::readSettings()
{
  // m_checkDao->setChecked( doc()->dao() );
  // m_checkOnTheFly->setChecked( doc()->onTheFly() );
  m_checkSimulate->setChecked( doc()->dummy() );
  // m_checkRemoveBufferFiles->setChecked( ((K3bVcdDoc*)doc())->removeBufferFiles() );

 
  K3bProjectBurnDialog::readSettings();
}


void K3bVcdBurnDialog::loadUserDefaults()
{
}


void K3bVcdBurnDialog::saveUserDefaults()
{
}

#include "k3bvcdburndialog.moc"
