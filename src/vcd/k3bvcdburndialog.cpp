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

  setupBurnTab( f1 );

  tab->addTab( f1, i18n("Burning") );

  // create advanced tab
  QWidget* advancedTab = new QWidget( tab );
  QGridLayout* advancedTabGrid = new QGridLayout( advancedTab );
  advancedTabGrid->setSpacing( spacingHint() );
  advancedTabGrid->setMargin( marginHint() );

  QGroupBox* advancedOptionGroup = new QGroupBox( 1, Qt::Vertical, i18n("Options"), advancedTab );

  advancedTabGrid->addWidget( advancedOptionGroup, 0, 0 );
  advancedTabGrid->setRowStretch( 1, 1 );

  tab->addTab( advancedTab, i18n("Advanced") );


  // connect( m_checkOnTheFly, SIGNAL(toggled(bool)), m_tempDirSelectionWidget, SLOT(setDisabled(bool)) );
  // connect( m_checkOnTheFly, SIGNAL(toggled(bool)), m_checkRemoveBufferFiles, SLOT(setDisabled(bool)) );

  m_tempDirSelectionWidget->setNeededSize( doc()->size() );

  // loadDefaults();
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
