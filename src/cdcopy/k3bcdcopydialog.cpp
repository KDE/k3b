/***************************************************************************
                          k3bcdcopydialog.cpp  -  description
                             -------------------
    begin                : Sun Mar 17 2002
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

#include "k3bcdcopydialog.h"

#include "k3bcdcopyjob.h"
#include "../k3bwriterselectionwidget.h"
#include "../k3btempdirselectionwidget.h"
#include "../k3b.h"
#include "../device/k3bdevice.h"
#include "../device/k3bdevicemanager.h"
#include "../k3bburnprogressdialog.h"

#include <kguiitem.h>
#include <klocale.h>
#include <kstdguiitem.h>
#include <kstandarddirs.h>

#include <qcheckbox.h>
#include <qspinbox.h>
#include <qcombobox.h>
#include <qlayout.h>
#include <qgroupbox.h>
#include <qptrlist.h>
#include <qlabel.h>
#include <qtooltip.h>


K3bCdCopyDialog::K3bCdCopyDialog( QWidget *parent, const char *name, bool modal )
  : KDialogBase( parent, name, modal, i18n("K3b Cd Copy"), User1|User2, User1, false, 
		 KGuiItem( i18n("Copy"), "copy", i18n("Start cd copy") ), KStdGuiItem::close() )
{
  setButtonBoxOrientation( Qt::Vertical );

  QWidget* main = new QWidget( this );
  setMainWidget( main );

  QGridLayout* mainGrid = new QGridLayout( main );
  mainGrid->setSpacing( spacingHint() );
  mainGrid->setMargin( marginHint() );

  m_writerSelectionWidget = new K3bWriterSelectionWidget( main );
  QGroupBox* groupSource = new QGroupBox( 1, Qt::Vertical, i18n("Reading Device"), main );
  groupSource->setInsideSpacing( spacingHint() );
  groupSource->setInsideMargin( marginHint() );
  QGroupBox* groupOptions = new QGroupBox( 4, Qt::Vertical, i18n("Options"), main );
  groupOptions->setInsideSpacing( spacingHint() );
  groupOptions->setInsideMargin( marginHint() );
  QGroupBox* groupCopies = new QGroupBox( 2, Qt::Horizontal, i18n("Copies"), main );
  groupCopies->setInsideSpacing( spacingHint() );
  groupCopies->setInsideMargin( marginHint() );

  m_comboSourceDevice = new QComboBox( groupSource );
  m_checkSimulate = new QCheckBox( i18n("Simulate writing"), groupOptions );
  m_checkOnTheFly = new QCheckBox( i18n("Writing on the fly"), groupOptions );
  m_checkDeleteImages = new QCheckBox( i18n("Delete images"), groupOptions );
  m_checkFastToc = new QCheckBox( i18n("Fast toc"), groupOptions );

  QLabel* pixLabel = new QLabel( groupCopies );
  pixLabel->setPixmap( locate( "appdata", "pics/k3b_cd_copy.png" ) );
  pixLabel->setScaledContents( false );
  m_spinCopies = new QSpinBox( groupCopies );
  m_spinCopies->setMinValue( 1 );
  m_spinCopies->setMaxValue( 99 );

  m_tempDirSelectionWidget = new K3bTempDirSelectionWidget( main );

  mainGrid->addMultiCellWidget( groupSource, 0, 0, 0, 1  );
  mainGrid->addMultiCellWidget( m_writerSelectionWidget, 1, 1, 0, 1  );
  mainGrid->addWidget( groupOptions, 2, 0 );
  mainGrid->addWidget( groupCopies, 3, 0 );
  mainGrid->addMultiCellWidget( m_tempDirSelectionWidget, 2, 3, 1, 1 );
  mainGrid->setRowStretch( 3, 1 );
  mainGrid->setColStretch( 1, 1 );


  // -- read cd-devices ----------------------------------------------
  QPtrList<K3bDevice> devices = k3bMain()->deviceManager()->allDevices();
  K3bDevice* dev = devices.first();
  while( dev ) {
    // cdrdao only supports SCSI devices
    if( dev->interfaceType() == K3bDevice::SCSI )
      m_comboSourceDevice->insertItem( dev->vendor() + " " + dev->description() + " (" + dev->genericDevice() + ")" );
    dev = devices.next();
  }

  connect( m_comboSourceDevice, SIGNAL(activated(int)), this, SLOT(slotSourceSelected()) );
  connect( m_writerSelectionWidget, SIGNAL(writerChanged()), this, SLOT(slotSourceSelected()) );

  connect( m_checkOnTheFly, SIGNAL(toggled(bool)), m_tempDirSelectionWidget, SLOT(setDisabled(bool)) );
  connect( m_checkOnTheFly, SIGNAL(toggled(bool)), m_checkDeleteImages, SLOT(setDisabled(bool)) );

  slotSourceSelected();

  m_checkDeleteImages->setChecked( true );


  QToolTip::add( m_checkFastToc, i18n("Do not extract pre-gaps and index marks") );
}


K3bCdCopyDialog::~K3bCdCopyDialog()
{
  qDebug("------deleted----");
}


void K3bCdCopyDialog::slotSourceSelected()
{
  K3bDevice* writer = m_writerSelectionWidget->writerDevice();
  K3bDevice* reader = readingDevice();

  if( writer == reader )
    m_checkOnTheFly->setChecked( false );
  m_checkOnTheFly->setDisabled( writer == reader );
}


K3bDevice* K3bCdCopyDialog::readingDevice() const
{
  const QString s = m_comboSourceDevice->currentText();

  QString strDev = s.mid( s.find('(') + 1, s.find(')') - s.find('(') - 1 );
 
  K3bDevice* dev =  k3bMain()->deviceManager()->deviceByName( strDev );
  if( !dev )
    qDebug( "(K3bCdCopyDialog) could not find device " + s );
		
  return dev;
}


void K3bCdCopyDialog::slotUser1()
{
  // if dummy true set copies to 1
  K3bCdCopyJob* job = new K3bCdCopyJob( this );

  job->setWriter( m_writerSelectionWidget->writerDevice() );
  job->setSpeed( m_writerSelectionWidget->writerSpeed() );
  job->setReader( readingDevice() );
  job->setDummy( m_checkSimulate->isChecked() );
  job->setOnTheFly( m_checkOnTheFly->isChecked() );
  job->setKeepImage( !m_checkDeleteImages->isChecked() );
  job->setFastToc( m_checkFastToc->isChecked() );
  job->setTempPath( m_tempDirSelectionWidget->tempPath() );
  if( !m_checkSimulate->isChecked() )
    job->setCopies( m_spinCopies->value() );


  // create a progresswidget
  K3bBurnProgressDialog d( k3bMain(), "burnProgress", !m_checkOnTheFly->isChecked() );

  d.setJob( job );

  hide();

  job->start();
  d.exec();

  slotClose();
}


void K3bCdCopyDialog::slotUser2()
{
  slotClose();
}


#include "k3bcdcopydialog.moc"
