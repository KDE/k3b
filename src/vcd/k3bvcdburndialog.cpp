/***************************************************************************
                             k3b -  description
                             -------------------
    copyright            : (C) 2002 by Sebastian Trueg
    email                : trueg@informatik.uni-freiburg.de
 ***************************************************************************/

/***************************************************************************
                          k3bvcdburndialog.cpp  -  description
                             -------------------
    begin                : Son Nov 10 2002
    copyright            : (C) 2002 by Christian Kvasny
    email                : chris@ckvsoft.at
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
#include "k3bvcdoptions.h"
#include "../device/k3bdevice.h"
#include "../k3bwriterselectionwidget.h"
#include "../k3btempdirselectionwidget.h"
#include "../tools/k3bglobals.h"

#include <qcheckbox.h>
#include <qgroupbox.h>
#include <qspinbox.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qlayout.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qgrid.h>
#include <qtoolbutton.h>
#include <qfileinfo.h>

#include <klocale.h>
#include <kconfig.h>
#include <kmessagebox.h>


K3bVcdBurnDialog::K3bVcdBurnDialog(K3bVcdDoc* _doc, QWidget *parent, const char *name, bool modal )
  : K3bProjectBurnDialog( _doc, parent, name, modal )
{

  m_vcdDoc = _doc;

  prepareGui();

  m_checkDao->hide();
  m_checkOnTheFly->hide();

  setupVideoCdTab();
  setupLabelTab();

  QFileInfo fi( m_tempDirSelectionWidget->tempPath() );
  QString path;

  if( fi.isFile() )
    path = fi.dirPath();
  else
    path = fi.filePath();

  if( path[path.length()-1] != '/' )
    path.append("/");

  path.append( vcdDoc()->vcdOptions()->volumeId() + ".bin" );
  m_tempDirSelectionWidget->setTempPath( path );

  m_tempDirSelectionWidget->setNeededSize( doc()->size() );

  readSettings();
}


K3bVcdBurnDialog::~K3bVcdBurnDialog()
{
}


void K3bVcdBurnDialog::setupVideoCdTab()
{
  QWidget* w = new QWidget( k3bMainWidget() );

  // ---------------------------------------------------- Format group ----
  m_groupVcdFormat = new QButtonGroup( 4, Qt::Vertical, i18n("Format"), w );
  m_radioVcd11 = new QRadioButton( i18n( "VideoCD 1.1" ), m_groupVcdFormat );
  m_radioVcd20 = new QRadioButton( i18n( "VideoCD 2.0" ), m_groupVcdFormat );
  m_radioSvcd10 = new QRadioButton( i18n( "Super-VideoCD" ), m_groupVcdFormat );
  m_groupVcdFormat->setExclusive(true);

  // ---------------------------------------------------- Options group ---
  m_groupOptions = new QGroupBox( 4, Qt::Vertical, i18n("Options"), w );
  m_checkNonCompliant = new QCheckBox( i18n( "Non-compliant compatibility mode for broken devices" ), m_groupOptions );
  m_check2336 = new QCheckBox( i18n( "Use 2336 byte sectors for output" ), m_groupOptions );

  // ----------------------------------------------------------------------
  QGridLayout* grid = new QGridLayout( w );
  grid->setMargin( marginHint() );
  grid->setSpacing( spacingHint() );
  grid->addWidget( m_groupVcdFormat, 0, 0 );
  grid->addWidget( m_groupOptions, 0, 1 );

  // TODO: set enabled to false, k2b canot resample now.
  m_groupVcdFormat->setEnabled(false);
  m_groupOptions->setEnabled(false);

  addPage( w, i18n("Settings") );
}

void K3bVcdBurnDialog::setupLabelTab()
{
  QWidget* w = new QWidget( k3bMainWidget() );

  m_checkApplicationId = new QCheckBox( i18n( "Write Application Id" ), w, "m_checkApplicationId" );

  // ----------------------------------------------------------------------
  QLabel* labelVolumeId = new QLabel( i18n( "&Volume Label:" ), w, "labelVolumeId" );
  QLabel* labelAlbumId = new QLabel( i18n( "&Album Id:" ), w, "labelAlbumId" );
  QLabel* labelVolumeCount = new QLabel( i18n( "Number of CDs in &Album:" ), w, "labelVolumeCount" );
  QLabel* labelVolumeNumber = new QLabel( i18n( "CD is &Number:" ), w, "labelVolumeNumber" );

  m_editVolumeId = new QLineEdit( w, "m_editDisc_id" );
  m_editAlbumId = new QLineEdit( w, "m_editAlbumId" );
  m_spinVolumeNumber = new QSpinBox( w, "m_editVolumeNumber" );
  m_spinVolumeCount = new QSpinBox( w, "m_editVolumeCount" );

  m_spinVolumeNumber->setMinValue(1);
  m_spinVolumeCount->setMinValue(1);
  m_spinVolumeNumber->setMaxValue(m_spinVolumeCount->value());

  QFrame* line = new QFrame( w );
  line->setFrameStyle( QFrame::HLine | QFrame::Sunken );

  // ----------------------------------------------------------------------
  QGridLayout* grid = new QGridLayout( w );
  grid->setMargin( marginHint() );
  grid->setSpacing( spacingHint() );

  grid->addMultiCellWidget( m_checkApplicationId, 0, 0, 0, 1 );
  grid->addMultiCellWidget( line, 1, 1, 0, 1 );

  grid->addWidget( labelVolumeId, 2, 0 );
  grid->addWidget( m_editVolumeId, 2, 1 );
  grid->addWidget( labelAlbumId, 3, 0 );
  grid->addWidget( m_editAlbumId, 3, 1 );
  grid->addWidget( labelVolumeCount, 4, 0 );
  grid->addWidget( m_spinVolumeCount, 4, 1 );
  grid->addWidget( labelVolumeNumber, 5, 0 );
  grid->addWidget( m_spinVolumeNumber, 5, 1 );

  grid->addRowSpacing( 5, 15 );
  grid->setRowStretch( 5, 1 );

  // buddies
  labelVolumeId->setBuddy( m_editVolumeId );
  labelAlbumId->setBuddy( m_editAlbumId );
  labelVolumeCount->setBuddy( m_spinVolumeCount );
  labelVolumeNumber->setBuddy( m_spinVolumeNumber );

  // tab order
  setTabOrder( m_editVolumeId, m_editAlbumId);
  setTabOrder( m_editAlbumId, m_spinVolumeCount );
  setTabOrder( m_spinVolumeCount, m_spinVolumeNumber );

  // TODO: enable this in the future :)
  m_checkApplicationId->setEnabled(false);
  labelVolumeCount->setEnabled(false);
  labelVolumeNumber->setEnabled(false);
  m_spinVolumeCount->setEnabled(false);
  m_spinVolumeNumber->setEnabled(false);

  addPage( w, i18n("Label") );
}


void K3bVcdBurnDialog::slotOk()
{
  // check if enough space in tempdir
  if( doc()->size()/1024 > m_tempDirSelectionWidget->freeTempSpace() ) {
    KMessageBox::sorry( this, i18n("Not enough space in temporary directory. Either change the directory or select on-the-fly burning.") );
    return;
  }
  else {
    QFileInfo fi( m_tempDirSelectionWidget->tempPath() );
    if( fi.isDir() )
      m_tempDirSelectionWidget->setTempPath( fi.filePath() + "/image.bin" );

    if( QFile::exists( m_tempDirSelectionWidget->tempPath() ) ) {
      if( KMessageBox::questionYesNo( this, i18n("Do you want to overwrite %1").arg(m_tempDirSelectionWidget->tempPath()), i18n("File exists...") )
        != KMessageBox::Yes )
      return;
    }
  }

  K3bProjectBurnDialog::slotOk();
}


void K3bVcdBurnDialog::loadDefaults()
{
  m_checkSimulate->setChecked( false );
  m_checkBurnproof->setChecked( true );

  m_checkRemoveBufferFiles->setChecked( true );

  m_checkApplicationId->setChecked( true );

  m_editVolumeId->setText( vcdDoc()->vcdOptions()->volumeId() );
  m_editAlbumId->setText( vcdDoc()->vcdOptions()->albumId() );

  // TODO: for the future
  // m_editPublisher->setText( o->publisher() );
  // m_editPreparer->setText( o->preparer() );
  // m_editSystem->setText( o->systemId() );
}

void K3bVcdBurnDialog::saveSettings()
{
  doc()->setTempDir( m_tempDirSelectionWidget->tempPath() );
  doc()->setDao( true );
  doc()->setDummy( m_checkSimulate->isChecked() );
  doc()->setOnTheFly( false );
  ((K3bVcdDoc*)doc())->setDeleteImage( m_checkRemoveBufferFiles->isChecked() );

  // -- saving current speed --------------------------------------
  doc()->setSpeed( m_writerSelectionWidget->writerSpeed() );

  // -- saving current device --------------------------------------
  doc()->setBurner( m_writerSelectionWidget->writerDevice() );

  // save image file path (.bin)
  ((K3bVcdDoc*)doc())->setVcdImage( m_tempDirSelectionWidget->tempPath() );

  // TODO: save vcdType
  vcdDoc()->vcdOptions()->setVolumeId( m_editVolumeId->text() );
  vcdDoc()->vcdOptions()->setAlbumId( m_editAlbumId->text() );
}


void K3bVcdBurnDialog::readSettings()
{
  m_checkSimulate->setChecked( doc()->dummy() );
  m_checkRemoveBufferFiles->setChecked( ((K3bVcdDoc*)doc())->deleteImage() );

  // read vcdType
  switch( ((K3bVcdDoc*)doc())->vcdType() ) {
  case K3bVcdDoc::VCD11:
      m_radioVcd11->setChecked( true );
    break;
  case K3bVcdDoc::VCD20:
      m_radioVcd20->setChecked( true );
    break;
  case K3bVcdDoc::SVCD10:
      m_radioSvcd10->setChecked( true );
    break;
  // case K3bVcdDoc::HQVCD:
  //   m_radioHqVcd->setChecked( true );
  //  break;
  default:
      m_radioVcd20->setChecked( true );
    break;
  }


  m_editVolumeId->setText( vcdDoc()->vcdOptions()->volumeId() );
  m_editAlbumId->setText( vcdDoc()->vcdOptions()->albumId() );

  K3bProjectBurnDialog::readSettings();
}

void K3bVcdBurnDialog::loadUserDefaults()
{
  KConfig* c = k3bMain()->config();

  c->setGroup( "default vcd settings" );

  m_checkSimulate->setChecked( c->readBoolEntry( "dummy_mode", false ) );
  m_checkBurnproof->setChecked( c->readBoolEntry( "burnproof", true ) );
  m_checkRemoveBufferFiles->setChecked( c->readBoolEntry( "remove_image", true ) );
}


void K3bVcdBurnDialog::saveUserDefaults()
{
  KConfig* c = k3bMain()->config();

  c->setGroup( "default vcd settings" );

  c->writeEntry( "dummy_mode", m_checkSimulate->isChecked() );

  c->writeEntry( "remove_image", m_checkRemoveBufferFiles->isChecked() );
}

#include "k3bvcdburndialog.moc"
