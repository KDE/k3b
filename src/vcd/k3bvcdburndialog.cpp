/***************************************************************************
                          k3bvcdburndialog.cpp  -  description
                             -------------------
    begin                : Son Nov 10 2002
    copyright            : (C) 2002 by Sebastian Trueg & Christian Kvasny
    email                : trueg@informatik.uni-freiburg.de
                           chris@ckvsoft.at
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

  QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );
  m_optionGroupLayout->addItem( spacer );

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
  if( K3bDevice* dev = m_writerSelectionWidget->writerDevice() )
    m_checkBurnproof->setEnabled( dev->burnproof() );

  connect( m_checkNonCompliant, SIGNAL(toggled(bool)), this, SLOT(slotNonCompliantToggled()) );
  connect( m_check2336, SIGNAL(toggled(bool)), this, SLOT(slot2336Toggled()) );
  connect( m_spinVolumeCount, SIGNAL(valueChanged(int)), this, SLOT(slotSpinVolumeCount()) );
  connect( m_spinVolumeNumber, SIGNAL(valueChanged(int)), this, SLOT(slotSpinVolumeNumber()) );
    
  // ToolTips
  // -------------------------------------------------------------------------
  QToolTip::add( m_radioVcd11, i18n("Select Video CD Typ %1").arg("(VCD 1.1)") );
  QToolTip::add( m_radioVcd20, i18n("Select Video CD Typ %1").arg("(VCD 2.0)") );
  QToolTip::add( m_radioSvcd10, i18n("Select Video CD Typ %1").arg("(SVCD 1.0)") );
  QToolTip::add( m_checkNonCompliant, i18n("Non-compliant compatibility mode for broken devices") );
  QToolTip::add( m_check2336, i18n("Use 2336 byte sectors for output") );

  QToolTip::add(m_checkApplicationId, i18n("write ISO application id for video cd") );
  QToolTip::add(m_editVolumeId, i18n("specify ISO volume label for video cd") );
  QToolTip::add(m_editAlbumId, i18n("specify album id for video cd set") );
  QToolTip::add(m_spinVolumeNumber, i18n("specify album set sequence number ( < volume-count )") );
  QToolTip::add(m_spinVolumeCount, i18n("specify number of volumes in album set") );

  // What's This info
  // -------------------------------------------------------------------------  
  QWhatsThis::add( m_radioVcd11, i18n("<p>This is the most basic <b>Video CD</b> specification dating back to 1993, which has the following characteristics:"
                                      "<ul><li>One mode2 mixed form ISO-9660 track containing file pointers to the information areas.</li>"
                                      "<li>Up to 98 multiplex-ed MPEG-1 audio/video streams or CD-DA audio tracks.</li>"
                                      "<li>Up to 500 MPEG sequence entry points used as chapter divisions.</li></ul>"
                                      "<p>The Video CD specification requires the multiplex-ed MPEG-1 stream to have a CBR of less than 174300 bytes (1394400 bits) per second in order to accommodate single speed CD-ROM drives.<br>"
                                      "The specification allows for the following two resolutions:"
                                      "<ul><li>352 x 240 @ 29.97 Hz (NTSC SIF).</li>"
                                      "<li>352 x 240 @ 23.976 Hz (FILM SIF).</li></ul>"
                                      "<p>The CBR MPEG-1, layer II audio stream is fixed at 224 kbps with 1 stereo or 2 mono channels."
                                      "<p><b>It is recommended to keep the video bit-rate under 1151929.1 bps.</b>") );
  
  QWhatsThis::add( m_radioVcd20, i18n("<p>About two years after the Video CD 1.1 specification came out, an improved <b>Video CD 2.0</b> standard was published in 1995."
                                      "<p>This one added the following items to the features already available in the Video CD 1.1 specification:"
                                      "<ul><li>Support for MPEG segment play items (<b>\"SPI\"</b>), consisting of still pictures, motion pictures and/or audio (only) streams was added.</li>"
                                      "<li>Note Segment Items::.</li>"
                                      "<li>Support for interactive playback control (<b>\"PBC\"</b>) was added.</li>"
                                      "<li>Support for playing related access by providing a scan point index file was added. (<b>\"/EXT/SCANDATA.DAT\"</b>)</li>"
                                      "<li>Support for closed captions.</li>"
                                      "<li>Support for mixing NTSC and PAL content.</li></ul>"
                                      "<p>By adding PAL support to the Video CD 1.1 specification, the following resolutions became available:"
                                      "<ul><li>352 x 240 @ 29.97 Hz (NTSC SIF).</li>"
                                      "<li>352 x 240 @ 23.976 Hz (FILM SIF).</li>"
                                      "<li>352 x 288 @ 25 Hz (PAL SIF).</li></ul>"
                                      "<p>For segment play items the following audio encodings became available:"
                                      "<ul><li>Joint stereo, stereo or dual channel audio streams at 128, 192, 224 or 384 kbit/sec bit-rate.</li>"
                                      "<li>Mono audio streams at 64, 96 or 192 kbit/sec bit-rate.</li></ul>"
                                      "<p>Also the possibility to have audio only streams and still pictures was provided."
                                      "<p><b>The bit-rate of multiplex-ed streams should be kept under 174300 bytes/sec (except for single still picture items) in order to accommodate single speed drives.</b>") );

   QWhatsThis::add( m_radioSvcd10, i18n("<p>With the upcoming of the DVD-V media, a new VCD standard had to be published in order to be able to keep up with technology, so the Super Video CD specification was called into life 1999."
                                        "<p>In the midst of 2000 a full subset of this <b>Super Video CD</b> specification was published as <b>IEC-62107</b>."
                                        "<p>As the most notable change over Video CD 2.0 is a switch from MPEG-1 CBR to MPEG-2 VBR encoding for the video stream was performed."
                                        "<p>The following new features--building upon the Video CD 2.0 specification--are:"
                                        "<ul><li>Use of MPEG-2 encoding instead of MPEG-1 for the video stream.</li>"
                                        "<li>Allowed VBR encoding of MPEG-1 audio stream.</li>"
                                        "<li>Higher resolutions (see below) for video stream resolution.</li>"
                                        "<li>Up to 4 overlay graphics and text (<b>\"OGT\"</b>) sub-channels for user switchable subtitle displaying in addition to the already existing closed caption facility.</li>"
                                        "<li>Command lists for controlling the SVCD virtual machine.</li></ul>"
                                        "<p>For the <b>Super Video CD</b>, only the following two resolutions are supported for motion video and (low resolution) still pictures:"
                                        "<ul><li>480 x 480 @ 29.97 Hz (NTSC 2/3 D-2).</li>"
                                        "<li>480 x 576 @ 25 Hz (PAL 2/3 D-2).</li></ul>") );

   QWhatsThis::add( m_checkNonCompliant, i18n("<ul><li>Rename <b>\"/MPEG2\"</b> folder on SVCDs to (non-compliant) \"/MPEGAV\".</li>"
                                              "<li>Enables the use of the (deprecated) signature <b>\"ENTRYSVD\"</b> instead of <b>\"ENTRYVCD\"</b> for the file <b>\"/SVCD/ENTRY.SVD\"</b>.</li>"
                                              "<li>Enables the use of the (deprecated) chinese <b>\"/SVCD/TRACKS.SVD\"</b> format which differs from the format defined in the <b>IEC-62107</b> specification.</li></ul>"
                                              "<p><b>The differences are most exposed on SVCDs containing more than one video track.</b>") );

   QWhatsThis::add( m_check2336, i18n("<p>though most devices will have problems with such an out-of-specification media."
                                      "<p><b>You may want use this option for images longer than 80 minutes</b>") );
                                              
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
  m_checkNonCompliant = new QCheckBox( i18n( "Enable Broken SVCD mode" ), m_groupOptions );
  m_check2336 = new QCheckBox( i18n( "Use 2336 byte Sectors" ), m_groupOptions );

  // ----------------------------------------------------------------------
  QGridLayout* grid = new QGridLayout( w );
  grid->setMargin( marginHint() );
  grid->setSpacing( spacingHint() );
  grid->addWidget( m_groupVcdFormat, 0, 0 );
  grid->addWidget( m_groupOptions, 0, 1 );

  // Only available on SVCD Type
  m_checkNonCompliant->setEnabled( false );
  m_checkNonCompliant->setChecked( false );
  
  // TODO: set enabled to false, k3b canot resample now.
  m_groupVcdFormat->setEnabled(false);

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

  m_editVolumeId->setMaxLength(31);
  
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

  //  grid->addRowSpacing( 5, 15 );
  grid->setRowStretch( 6, 1 );

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

  /*
  labelVolumeCount->setEnabled(false);
  labelVolumeNumber->setEnabled(false);
  m_spinVolumeCount->setEnabled(false);
  m_spinVolumeNumber->setEnabled(false);
  */

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
      m_checkNonCompliant->setEnabled( true );
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
  c->writeEntry( "burnproof", m_checkBurnproof->isChecked() );
  c->writeEntry( "remove_image", m_checkRemoveBufferFiles->isChecked() );

  m_tempDirSelectionWidget->saveConfig();
}

void K3bVcdBurnDialog::slotNonCompliantToggled()
{
  // trueg: shouldn't this be done when the user clicks "save" or "burn"?
  vcdDoc()->vcdOptions()->setBrokenSVcdMode(m_checkNonCompliant->isChecked());  
}

void K3bVcdBurnDialog::slot2336Toggled()
{
  // trueg: shouldn't this be done when the user clicks "save" or "burn"?
  vcdDoc()->vcdOptions()->setSector2336(m_check2336->isChecked());
}

void K3bVcdBurnDialog::slotSpinVolumeCount()
{
  int c = m_spinVolumeCount->value();
  m_spinVolumeNumber->setMaxValue(c);
  vcdDoc()->vcdOptions()->setVolumeCount(c);
}

void K3bVcdBurnDialog::slotSpinVolumeNumber()
{
  vcdDoc()->vcdOptions()->setVolumeNumber(m_spinVolumeNumber->value());  
}

#include "k3bvcdburndialog.moc"
