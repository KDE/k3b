/*
 *
 * $Id$
 * Copyright (C) 2003 Christian Kvasny <chris@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2003 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bvcdburndialog.h"
#include "../k3b.h"
#include "k3bvcddoc.h"
#include "k3bvcdoptions.h"
#include "../device/k3bdevice.h"
#include "../k3bwriterselectionwidget.h"
#include "../k3btempdirselectionwidget.h"
#include "../k3bstdguiitems.h"
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
#include <kstandarddirs.h>


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

  slotSetImagePath();
    
  readSettings();

  connect( m_spinVolumeCount, SIGNAL(valueChanged(int)), this, SLOT(slotSpinVolumeCount()) );
  connect( m_editVolumeId, SIGNAL(textChanged(const QString&)), this, SLOT(slotSetImagePath()) );
  connect( m_groupVcdFormat, SIGNAL(clicked(int)), this, SLOT(slotVcdTypeClicked(int)) );
  connect( m_checkCdiSupport, SIGNAL(toggled(bool)), this, SLOT(slotCdiSupportChecked(bool)) );
  connect( m_checkAutoDetect, SIGNAL(toggled(bool)), this, SLOT(slotAutoDetect(bool)) );

  // ToolTips
  // -------------------------------------------------------------------------
  QToolTip::add( m_radioVcd11, i18n("Select Video CD type %1").arg("(VCD 1.1)") );
  QToolTip::add( m_radioVcd20, i18n("Select Video CD type %1").arg("(VCD 2.0)") );
  QToolTip::add( m_radioSvcd10, i18n("Select Video CD type %1").arg("(SVCD 1.0)") );
  QToolTip::add( m_checkAutoDetect, i18n("Automatic video type recognition.") );
  QToolTip::add( m_checkNonCompliant, i18n("Non-compliant compatibility mode for broken devices") );
  QToolTip::add( m_check2336, i18n("Use 2336 byte sectors for output") );

  QToolTip::add(m_editVolumeId, i18n("Specify ISO volume label for Video CD") );
  QToolTip::add(m_editAlbumId, i18n("Specify album id for VideoCD set") );
  QToolTip::add(m_spinVolumeNumber, i18n("Specify album set sequence number ( <= volume-count )") );
  QToolTip::add(m_spinVolumeCount, i18n("Specify number of volumes in album set") );

  QToolTip::add(m_checkCdiSupport, i18n("Enable CD-i Application Support for VideoCD Type 1.1 & 2.0") );
  QToolTip::add(m_editCdiCfg, i18n("Configuration parameters (only for VCD 2.0)") );
  
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

   QWhatsThis::add( m_checkAutoDetect, i18n("<p>If Autodetect is:</p>"
                                            "<ul><li>ON, than k3b will set the right VideoCD Type.</li>"
                                            "<li>OFF then the User must set the right VideoCD type himself.</li></ul>"
					    "<p>You are not sure about the right VideoCD Type? In this case it is a good option to turn Autodetect ON.</p>"
					    "<p>You will force the VideoCD Type? Then you must turn Autodetect OFF. Usefull for some standalone DVD Players without SVCD support</p>") );
   
   QWhatsThis::add( m_checkNonCompliant, i18n("<ul><li>Rename <b>\"/MPEG2\"</b> folder on SVCDs to (non-compliant) \"/MPEGAV\".</li>"
                                              "<li>Enables the use of the (deprecated) signature <b>\"ENTRYSVD\"</b> instead of <b>\"ENTRYVCD\"</b> for the file <b>\"/SVCD/ENTRY.SVD\"</b>.</li>"
                                              "<li>Enables the use of the (deprecated) chinese <b>\"/SVCD/TRACKS.SVD\"</b> format which differs from the format defined in the <b>IEC-62107</b> specification.</li></ul>"
                                              "<p><b>The differences are most exposed on SVCDs containing more than one video track.</b>") );

   QWhatsThis::add( m_check2336, i18n("<p>though most devices will have problems with such an out-of-specification media."
                                      "<p><b>You may want use this option for images longer than 80 minutes</b>") );
                                              
   QWhatsThis::add( m_checkCdiSupport, i18n("<p>To allow the play of Video-CD's on a CD-i player, the Video-CD standard requires that a CD-i application program must be present."
                                            "<p>This program is designed to:"
                                            "<ul><li>provide full play back control as defined in the PSD of the standard</l>"
                                            "<li>be extremely simple to use and easy-to-learn for the end-user</li></ul>"
                                            "<p>The program runs on CD-i players equipped with the CDRTOS 1.1(.1) operating system and a Digital Video extension cartridge.") );

   QWhatsThis::add( m_editCdiCfg, i18n("Configuration parameters only available for VideoCD 2.0"
                                       "<p>The engine works perfectly well when used as is."
                                       "<p>You have the option to configure the VCD application."
                                       "<p>You can adapt the colour and / or the shape of the cursor and lots more.") );

}


K3bVcdBurnDialog::~K3bVcdBurnDialog()
{
}


void K3bVcdBurnDialog::setupVideoCdTab()
{
  QWidget* w = new QWidget( k3bMainWidget() );

  // ---------------------------------------------------- Format group ----
  m_groupVcdFormat = new QButtonGroup( 4, Qt::Vertical, i18n("Type"), w );
  m_radioVcd11 = new QRadioButton( i18n( "VideoCD 1.1" ), m_groupVcdFormat );
  m_radioVcd20 = new QRadioButton( i18n( "VideoCD 2.0" ), m_groupVcdFormat );
  m_radioSvcd10 = new QRadioButton( i18n( "Super-VideoCD" ), m_groupVcdFormat );
  m_groupVcdFormat->setExclusive(true);

  // ---------------------------------------------------- Options group ---

  m_groupOptions = new QGroupBox( 4, Qt::Vertical, i18n("Options"), w );
  m_checkAutoDetect = new QCheckBox( i18n( "Autodetect VideoCD type" ), m_groupOptions );
  
  m_checkNonCompliant = new QCheckBox( i18n( "Enable Broken SVCD mode" ), m_groupOptions );
  // Only available on SVCD Type
  m_checkNonCompliant->setEnabled( false );
  m_checkNonCompliant->setChecked( false );

  m_check2336 = new QCheckBox( i18n( "Use 2336 byte Sectors" ), m_groupOptions );

  m_checkCdiSupport = new QCheckBox( i18n( "Enable CD-i support" ), m_groupOptions );
  
  // ------------------------------------------------- CD-i Application ---
  m_groupCdi = new QGroupBox( 4, Qt::Vertical, i18n("Video on CD-i"), w );
  m_editCdiCfg = new QMultiLineEdit( m_groupCdi, "m_editCdiCfg" );
  m_editCdiCfg->setFrameShape( QTextEdit::NoFrame );

  // ----------------------------------------------------------------------
  QGridLayout* grid = new QGridLayout( w );
  grid->setMargin( marginHint() );
  grid->setSpacing( spacingHint() );
  grid->addMultiCellWidget( m_groupVcdFormat, 0, 1, 0, 0 );
  grid->addWidget( m_groupOptions, 0, 1 );
  grid->addWidget( m_groupCdi, 1, 1 );
  
  addPage( w, i18n("Settings") );
}

void K3bVcdBurnDialog::setupLabelTab()
{
  QWidget* w = new QWidget( k3bMainWidget() );

  // ----------------------------------------------------------------------
  // noEdit
  QLabel* labelSystemId = new QLabel( i18n( "System Id:" ), w, "labelSystemId" );
  QLabel* labelApplicationId = new QLabel( i18n( "Application Id:" ), w, "labelApplicationId" );
  QLabel* labelInfoSystemId = new QLabel( vcdDoc()->vcdOptions()->systemId(), w, "labelInfoSystemId" );
  QLabel* labelInfoApplicationId = new QLabel( vcdDoc()->vcdOptions()->applicationId(), w, "labelInfoApplicationId" );

  labelInfoSystemId->setFrameShape( QLabel::LineEditPanel );
  labelInfoSystemId->setFrameShadow( QLabel::Sunken );

  labelInfoApplicationId->setFrameShape( QLabel::LineEditPanel );
  labelInfoApplicationId->setFrameShadow( QLabel::Sunken );
  QToolTip::add(labelInfoApplicationId, i18n("ISO application id for VideoCD") );

  // ----------------------------------------------------------------------

  QLabel* labelVolumeId = new QLabel( i18n( "Volume &Label:" ), w, "labelVolumeId" );
  QLabel* labelAlbumId = new QLabel( i18n( "&Album Id:" ), w, "labelAlbumId" );
  QLabel* labelVolumeCount = new QLabel( i18n( "Number of &Volumes in Album:" ), w, "labelVolumeCount" );
  QLabel* labelVolumeNumber = new QLabel( i18n( "This CD is &Sequence Number:" ), w, "labelVolumeNumber" );

  
  m_editVolumeId = new QLineEdit( w, "m_editVolumeId" );
  m_editAlbumId = new QLineEdit( w, "m_editAlbumId" );
  m_spinVolumeNumber = new QSpinBox( w, "m_editVolumeNumber" );
  m_spinVolumeCount = new QSpinBox( w, "m_editVolumeCount" );

  m_editVolumeId->setMaxLength(31);
  
  m_spinVolumeNumber->setMinValue(1);
  m_spinVolumeNumber->setMaxValue(1);
  m_spinVolumeCount->setMinValue(1);

  QFrame* line = new QFrame( w );
  line->setFrameStyle( QFrame::HLine | QFrame::Sunken );

  // ----------------------------------------------------------------------
  QGridLayout* grid = new QGridLayout( w );
  grid->setMargin( marginHint() );
  grid->setSpacing( spacingHint() );

  grid->addWidget( labelSystemId, 1, 0 );
  grid->addWidget( labelInfoSystemId, 1, 1 );
  grid->addWidget( labelApplicationId, 2, 0 );
  grid->addWidget( labelInfoApplicationId, 2, 1 );

  grid->addMultiCellWidget( line, 3, 3, 0, 1 );

  grid->addWidget( labelVolumeId, 4, 0 );
  grid->addWidget( m_editVolumeId, 4, 1 );
  grid->addWidget( labelAlbumId, 5, 0 );
  grid->addWidget( m_editAlbumId, 5, 1 );
  grid->addWidget( labelVolumeCount, 6, 0 );
  grid->addWidget( m_spinVolumeCount, 6, 1 );
  grid->addWidget( labelVolumeNumber, 7, 0 );
  grid->addWidget( m_spinVolumeNumber, 7, 1 );

  //  grid->addRowSpacing( 5, 15 );
  grid->setRowStretch( 8, 1 );

  // buddies
  labelVolumeId->setBuddy( m_editVolumeId );
  labelAlbumId->setBuddy( m_editAlbumId );
  labelVolumeCount->setBuddy( m_spinVolumeCount );
  labelVolumeNumber->setBuddy( m_spinVolumeNumber );

  // tab order
  setTabOrder( m_editVolumeId, m_editAlbumId);
  setTabOrder( m_editAlbumId, m_spinVolumeCount );
  setTabOrder( m_spinVolumeCount, m_spinVolumeNumber );

  addPage( w, i18n("Label") );
}


void K3bVcdBurnDialog::slotOk()
{
    
  if( QFile::exists( vcdDoc()->vcdImage() ) ) {
    if( KMessageBox::questionYesNo( this, i18n("Do you want to overwrite %1").arg(vcdDoc()->vcdImage()), i18n("File exists...") )
        != KMessageBox::Yes )
      return;
  }

  K3bProjectBurnDialog::slotOk();
}


void K3bVcdBurnDialog::loadDefaults()
{
  m_checkDao->setChecked( true );
  m_checkSimulate->setChecked( false );
  m_checkBurnproof->setChecked( true );
  m_checkRemoveBufferFiles->setChecked( true );
  m_checkOnlyCreateImage->setChecked( false );

  m_editVolumeId->setText( "VIDEOCD" );
  m_editAlbumId->setText( "" );

  m_checkAutoDetect->setChecked( true );
  m_groupVcdFormat->setDisabled( true );
  
  m_check2336->setChecked( false );
  m_checkNonCompliant->setChecked( false );

  m_spinVolumeNumber->setValue( 1 );
  m_spinVolumeCount->setValue( 1 );

  m_groupCdi->setEnabled( false );

  if (!m_radioSvcd10->isChecked()) {
    m_checkCdiSupport->setEnabled(vcdDoc()->vcdOptions()->checkCdiFiles());
    m_checkCdiSupport->setChecked(m_checkCdiSupport->isEnabled());
  }
  else {
    m_checkCdiSupport->setEnabled(false);
    m_checkCdiSupport->setChecked(false);
  }

  loadDefaultCdiConfig();    

  // TODO: for the future
  // m_editPublisher->setText( o->publisher() );
  // m_editPreparer->setText( o->preparer() );
  // m_editSystem->setText( o->systemId() );
}

void K3bVcdBurnDialog::saveSettings()
{
  slotSetImagePath();

  doc()->setTempDir( m_tempDirSelectionWidget->tempPath() );
  doc()->setDao( true );
  doc()->setDummy( m_checkSimulate->isChecked() );
  doc()->setOnTheFly( false );
  doc()->setBurnproof( m_checkBurnproof->isChecked() );
  
  // -- saving current speed --------------------------------------
  doc()->setSpeed( m_writerSelectionWidget->writerSpeed() );

  // -- saving current device --------------------------------------
  doc()->setBurner( m_writerSelectionWidget->writerDevice() );

  vcdDoc()->setDeleteImage( m_checkRemoveBufferFiles->isChecked() );
  // save image file & path (.bin)
  vcdDoc()->setVcdImage( m_tempDirSelectionWidget->tempPath() + m_editVolumeId->text() + ".bin");

  int id = m_groupVcdFormat->id(m_groupVcdFormat->selected());
  vcdDoc()->setVcdType(id);
  switch(id) {
    case 0:
      vcdDoc()->vcdOptions()->setVcdClass("vcd");
      vcdDoc()->vcdOptions()->setVcdVersion("1.1");
      break;
    case 1:
      //vcd 2.0
      vcdDoc()->vcdOptions()->setVcdClass("vcd");
      vcdDoc()->vcdOptions()->setVcdVersion("2.0");
      break;
    case 2:
      //svcd 1.0
      vcdDoc()->vcdOptions()->setVcdClass("svcd");
      vcdDoc()->vcdOptions()->setVcdVersion("1.0");
      break;
  }
  
  vcdDoc()->vcdOptions()->setVolumeId( m_editVolumeId->text() );
  vcdDoc()->vcdOptions()->setAlbumId( m_editAlbumId->text() );

  vcdDoc()->vcdOptions()->setAutoDetect(m_checkAutoDetect->isChecked());
  vcdDoc()->vcdOptions()->setBrokenSVcdMode(m_checkNonCompliant->isChecked());
  vcdDoc()->vcdOptions()->setSector2336(m_check2336->isChecked());

  vcdDoc()->vcdOptions()->setCdiSupport( m_checkCdiSupport->isChecked() );
  vcdDoc()->setOnlyCreateImage( m_checkOnlyCreateImage->isChecked() );

  vcdDoc()->vcdOptions()->setVolumeNumber(m_spinVolumeNumber->value());
  vcdDoc()->vcdOptions()->setVolumeCount(m_spinVolumeCount->value());

  if (m_editCdiCfg->edited())
    saveCdiConfig();
}


void K3bVcdBurnDialog::readSettings()
{
  m_checkDao->setChecked( true );
  m_checkSimulate->setChecked( doc()->dummy() );
  m_checkRemoveBufferFiles->setChecked( vcdDoc()->deleteImage() );
  m_checkBurnproof->setChecked( doc()->burnproof() );
  
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

  m_spinVolumeCount->setValue( vcdDoc()->vcdOptions()->volumeCount() );
  m_spinVolumeNumber->setValue( vcdDoc()->vcdOptions()->volumeNumber() );

  m_checkAutoDetect->setChecked( vcdDoc()->vcdOptions()->AutoDetect() );
  m_groupVcdFormat->setDisabled( vcdDoc()->vcdOptions()->AutoDetect() );
  
  m_check2336->setChecked( vcdDoc()->vcdOptions()->Sector2336() );

  m_checkCdiSupport->setEnabled( false );
  m_checkCdiSupport->setChecked( false );
  m_groupCdi->setEnabled( false );

  if ( m_radioSvcd10->isChecked() ) {
    m_checkNonCompliant->setChecked( vcdDoc()->vcdOptions()->BrokenSVcdMode() );
  }
  else {
    m_checkNonCompliant->setChecked( false );
    m_checkNonCompliant->setEnabled( false );
    if (vcdDoc()->vcdOptions()->checkCdiFiles()) {
      m_checkCdiSupport->setEnabled( true );
      m_checkCdiSupport->setChecked( vcdDoc()->vcdOptions()->CdiSupport() );
    }
  }

  m_editVolumeId->setText( vcdDoc()->vcdOptions()->volumeId() );
  m_editAlbumId->setText( vcdDoc()->vcdOptions()->albumId() );

  K3bProjectBurnDialog::readSettings();

  loadCdiConfig();
}

void K3bVcdBurnDialog::loadUserDefaults()
{
  KConfig* c = k3bMain()->config();

  c->setGroup( "Videocd settings" );

  K3bVcdOptions o = K3bVcdOptions::load( c );
  
  m_checkAutoDetect->setChecked( o.AutoDetect() );
  m_check2336->setChecked( o.Sector2336() );
    
  m_checkCdiSupport->setChecked( false );
  m_checkCdiSupport->setEnabled( false );
  m_groupCdi->setEnabled( false );

  if ( m_radioSvcd10->isChecked() ) {
    m_checkNonCompliant->setChecked( o.BrokenSVcdMode() );
  }
  else {
    m_checkNonCompliant->setChecked( false );
    m_checkNonCompliant->setEnabled( false );
    if (vcdDoc()->vcdOptions()->checkCdiFiles()) {
      m_checkCdiSupport->setEnabled( true );
      m_checkCdiSupport->setChecked( o.CdiSupport() );
    }
  }

  m_spinVolumeCount->setValue( o.volumeCount() );
  m_spinVolumeNumber->setValue( o.volumeNumber() );

  m_editVolumeId->setText( o.volumeId() );
  m_editAlbumId->setText( o.albumId() );
        
  m_checkSimulate->setChecked( c->readBoolEntry( "dummy_mode", false ) );
  m_checkBurnproof->setChecked( c->readBoolEntry( "burnproof", true ) );
  m_checkRemoveBufferFiles->setChecked( c->readBoolEntry( "remove_image", true ) );
  m_checkOnlyCreateImage->setChecked( c->readBoolEntry( "only_create_image", false ) );

  loadCdiConfig();
}


void K3bVcdBurnDialog::saveUserDefaults()
{
  KConfig* c = k3bMain()->config();
  K3bVcdOptions o;

  c->setGroup( "Videocd settings" );

  c->writeEntry( "dummy_mode", m_checkSimulate->isChecked() );
  c->writeEntry( "burnproof", m_checkBurnproof->isChecked() );
  c->writeEntry( "remove_image", m_checkRemoveBufferFiles->isChecked() );
  c->writeEntry( "only_create_image", m_checkOnlyCreateImage->isChecked() );

  o.setVolumeId( m_editVolumeId->text() );
  o.setAlbumId( m_editAlbumId->text() );
  o.setAutoDetect(m_checkAutoDetect->isChecked());
  o.setBrokenSVcdMode(m_checkNonCompliant->isChecked());
  o.setSector2336(m_check2336->isChecked());
  o.setVolumeCount(m_spinVolumeCount->value());
  o.setVolumeNumber(m_spinVolumeNumber->value());  
  o.setCdiSupport(m_checkCdiSupport->isChecked());
  o.save( c );

  m_tempDirSelectionWidget->saveConfig();

  saveCdiConfig();
}

void K3bVcdBurnDialog::saveCdiConfig()
{

    QString filename = locateLocal( "appdata", "cdi/cdi_vcd.cfg");
    if (QFile::exists(filename))
      QFile::remove(filename);
      
    QFile cdi(filename);
    if ( !cdi.open( IO_WriteOnly ))
      return;

    QTextStream s( &cdi );
    int i = m_editCdiCfg->numLines();

    for ( int j = 0; j < i; j++)
        s << QString("%1").arg(m_editCdiCfg->textLine(j)) << "\n";

    cdi.close();

    m_editCdiCfg->setEdited(false);
}

void K3bVcdBurnDialog::loadCdiConfig()
{
    QString filename = locateLocal( "appdata", "cdi/cdi_vcd.cfg");
    if (QFile::exists(filename)) {
      QFile cdi(filename);
      if ( !cdi.open( IO_ReadOnly )) {
        loadDefaultCdiConfig();
        return;
      }

      QTextStream s( &cdi );

      m_editCdiCfg->clear();
      
      while ( !s.atEnd() )
            m_editCdiCfg->insertLine( s.readLine() );

      cdi.close();
      m_editCdiCfg->setEdited(false);
      m_editCdiCfg->setCursorPosition(0,0,false);
      m_groupCdi->setEnabled(m_checkCdiSupport->isChecked());
    }
    else
      loadDefaultCdiConfig();
    
}

void K3bVcdBurnDialog::loadDefaultCdiConfig()
{
    QString filename = locate( "data", "k3b/cdi/cdi_vcd.cfg");
    if (QFile::exists(filename)) {
      QFile cdi(filename);
      if ( !cdi.open( IO_ReadOnly )) {
        m_checkCdiSupport->setChecked( false );
        m_checkCdiSupport->setEnabled( false );
        return;
      }

      QTextStream s( &cdi );

      m_editCdiCfg->clear();
      
      while ( !s.atEnd() )
            m_editCdiCfg->insertLine( s.readLine() );

      cdi.close();
      m_editCdiCfg->setEdited(false);
      m_editCdiCfg->setCursorPosition(0,0,false);
      m_groupCdi->setEnabled(m_checkCdiSupport->isChecked());
    }
}

void K3bVcdBurnDialog::slotSetImagePath()
{
  QFileInfo fi( m_tempDirSelectionWidget->tempPath() );
  QString path;

  if( fi.isDir() || fi.extension(false) != "bin" )
    path = fi.filePath();
  else
    path = fi.dirPath();

  if( path[path.length()-1] != '/' )
    path.append("/");

  if ( vcdDoc()->vcdOptions()->volumeId().length() < 1 ) {
    vcdDoc()->vcdOptions()->setVolumeId("VIDEOCD");
  }

  // path.append( vcdDoc()->vcdOptions()->volumeId() + ".bin" );
  m_tempDirSelectionWidget->setTempPath( path );
}


void K3bVcdBurnDialog::slotSpinVolumeCount()
{
  m_spinVolumeNumber->setMaxValue(m_spinVolumeCount->value());
}

void K3bVcdBurnDialog::slotVcdTypeClicked( int i)
{

  switch(i) {
    case 0:
      // vcd 1.1 no support for version 3.x.
      // v4 work also for vcd 1.1 but without CD-i menues.
      // Do anybody use vcd 1.1 with cd-i????
    case 1:
      //vcd 2.0
      m_checkCdiSupport->setEnabled( vcdDoc()->vcdOptions()->checkCdiFiles() );
      m_groupCdi->setEnabled( m_checkCdiSupport->isChecked() );
      
      m_checkNonCompliant->setEnabled( false );
      m_checkNonCompliant->setChecked( false );
      break;
    case 2:
      //svcd 1.0
      m_checkCdiSupport->setEnabled( false );
      m_checkCdiSupport->setChecked( false );
      m_groupCdi->setEnabled( false );
      
      m_checkNonCompliant->setEnabled( true );
      break;
  }
  
}

void K3bVcdBurnDialog::slotCdiSupportChecked( bool b)
{
  m_groupCdi->setEnabled( b );
}

void K3bVcdBurnDialog::slotAutoDetect( bool b)
{
	if (b) {
	// TODO: here i must check for the Mpeg Filetype to set the right VCD Type
	// in Autodetection Mode
	}
	m_groupVcdFormat->setDisabled( b );

}
#include "k3bvcdburndialog.moc"
