/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
 *                    Klaus-Dieter Krannich <kd@k3b.org>
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


#include "k3bcdcopydialog.h"

#include "k3bcdcopyjob.h"
#include <k3bwriterselectionwidget.h>
#include <k3btempdirselectionwidget.h>
#include <k3bcore.h>
#include <k3bstdguiitems.h>
#include <device/k3bdevice.h>
#include <device/k3bdevicemanager.h>
#include <k3bburnprogressdialog.h>
#include <tools/k3bglobals.h>
#include <tools/k3bexternalbinmanager.h>

#include <kguiitem.h>
#include <klocale.h>
#include <kstdguiitem.h>
#include <kstandarddirs.h>
#include <kmessagebox.h> 
#include <kconfig.h>
#include <kapplication.h>

#include <qcheckbox.h>
#include <qspinbox.h>
#include <qcombobox.h>
#include <qlayout.h>
#include <qgroupbox.h>
#include <qptrlist.h>
#include <qlabel.h>
#include <qtooltip.h>
#include <qtabwidget.h>
#include <qwhatsthis.h>
#include <qhbox.h>
#include <qpushbutton.h>


K3bCdCopyDialog::K3bCdCopyDialog( QWidget *parent, const char *name, bool modal )
  : K3bInteractionDialog( parent, name, i18n("CD Copy"), QString::null,
			  START_BUTTON|CANCEL_BUTTON,
			  START_BUTTON,
			  modal )
{
  setStartButtonText( i18n("Start CD Copy") );

  QWidget* main = mainWidget();

  QGridLayout* mainGrid = new QGridLayout( main );
  mainGrid->setSpacing( spacingHint() );
  mainGrid->setMargin( 0 );

  m_writerSelectionWidget = new K3bWriterSelectionWidget( main );
  m_writerSelectionWidget->setSupportedWritingApps( K3b::CDRDAO );
  QGroupBox* groupSource = new QGroupBox( 1, Qt::Vertical, i18n("CD Reader Device"), main );
  groupSource->setInsideSpacing( spacingHint() );
  groupSource->setInsideMargin( marginHint() );

  m_comboSourceDevice = new QComboBox( groupSource );


  // tab widget --------------------
  QTabWidget* tabWidget = new QTabWidget( main );

  // option tab --------------------
  QWidget* optionTab = new QWidget( tabWidget );
  QGridLayout* optionTabGrid = new QGridLayout( optionTab );
  optionTabGrid->setSpacing( spacingHint() );
  optionTabGrid->setMargin( marginHint() );

  QGroupBox* groupOptions = new QGroupBox( 5, Qt::Vertical, i18n("Options"), optionTab );
  groupOptions->setInsideSpacing( spacingHint() );
  groupOptions->setInsideMargin( marginHint() );

  QGroupBox* groupCopies = new QGroupBox( 2, Qt::Horizontal, i18n("Copies"), optionTab );
  groupCopies->setInsideSpacing( spacingHint() );
  groupCopies->setInsideMargin( marginHint() );

  m_checkSimulate = K3bStdGuiItems::simulateCheckbox( groupOptions );
  m_checkOnTheFly = K3bStdGuiItems::onTheFlyCheckbox( groupOptions );
  m_checkOnlyCreateImage = K3bStdGuiItems::onlyCreateImagesCheckbox( groupOptions );
  m_checkDeleteImages = K3bStdGuiItems::removeImagesCheckbox( groupOptions );

  QLabel* pixLabel = new QLabel( groupCopies );
  pixLabel->setPixmap( locate( "appdata", "pics/k3b_cd_copy.png" ) );
  pixLabel->setScaledContents( false );
  m_spinCopies = new QSpinBox( groupCopies );
  m_spinCopies->setMinValue( 1 );
  m_spinCopies->setMaxValue( 99 );

  m_tempDirSelectionWidget = new K3bTempDirSelectionWidget( optionTab );


  optionTabGrid->addWidget( groupOptions, 0, 0 );
  optionTabGrid->addWidget( groupCopies, 1, 0 );
  optionTabGrid->addMultiCellWidget( m_tempDirSelectionWidget, 0, 1, 1, 1 );
  optionTabGrid->setRowStretch( 1, 1 );
  optionTabGrid->setColStretch( 1, 1 );


  tabWidget->addTab( optionTab, i18n("&Options") );


  // advanced tab ------------------
  QWidget* advancedTab = new QWidget( tabWidget );
  QGridLayout* advancedTabGrid = new QGridLayout( advancedTab );
  advancedTabGrid->setSpacing( spacingHint() );
  advancedTabGrid->setMargin( marginHint() );

  QGroupBox* groupAudio = new QGroupBox( 2, Qt::Vertical, i18n("Audio Extraction"), advancedTab ); 
  groupAudio->setInsideSpacing( spacingHint() );
  groupAudio->setInsideMargin( marginHint() );
  m_checkFastToc = new QCheckBox( i18n("Fast TOC"), groupAudio );
  QHBox *p = new QHBox( groupAudio );
  p->setStretchFactor(new QLabel( i18n("Paranoia mode:"), p ), 1 );
  m_comboParanoiaMode = K3bStdGuiItems::paranoiaModeComboBox( p );

  QGroupBox* groupRaw   = new QGroupBox( 2, Qt::Vertical, i18n("Read Raw"), advancedTab ); 
  groupRaw->setInsideSpacing( spacingHint() );
  groupRaw->setInsideMargin( marginHint() );
  m_checkRawCopy = new QCheckBox( i18n("Raw copy"), groupRaw );
  QHBox *s = new QHBox( groupRaw );
  s->setStretchFactor(new QLabel( i18n("Read subchan mode:"), s ), 1 );
  m_comboSubchanMode = new QComboBox( s );
  m_comboSubchanMode->insertItem( "none" );
  m_comboSubchanMode->insertItem( "rw" );
  m_comboSubchanMode->insertItem( "rw_raw" );
 
  QGroupBox* groupTao   = new QGroupBox( 2, Qt::Vertical, i18n("Track at Once Source"), advancedTab ); 
  groupTao->setInsideSpacing( spacingHint() );
  groupTao->setInsideMargin( marginHint() );
  m_checkTaoSource = new QCheckBox( i18n("TAO source"), groupTao );
  QHBox *t = new QHBox( groupTao );
  QLabel* taoSourceAdjustLabel = new QLabel( i18n("Tao source adjust:"), t);
  t->setStretchFactor( taoSourceAdjustLabel, 1 );
  m_spinTaoSourceAdjust = new QSpinBox( t );
  m_spinTaoSourceAdjust->setMinValue( 1 );
  m_spinTaoSourceAdjust->setMaxValue( 99 );
  m_spinTaoSourceAdjust->setValue( 2 );
  m_spinTaoSourceAdjust->setDisabled( true );
  taoSourceAdjustLabel->setDisabled( true );

  QGroupBox* groupOther = new QGroupBox( 2, Qt::Vertical, i18n("Other"), advancedTab ); 
  groupOther->setInsideSpacing( spacingHint() );
  groupOther->setInsideMargin( marginHint() );
  m_checkForce = new QCheckBox( i18n("Force write"), groupOther );

  advancedTabGrid->addWidget( groupAudio, 0, 0 );  
  advancedTabGrid->addWidget( groupRaw,   0, 1 );
  advancedTabGrid->addWidget( groupTao,   1, 0 );
  advancedTabGrid->addWidget( groupOther, 1, 1 );


  tabWidget->addTab( advancedTab, i18n("&Advanced") );

  mainGrid->addWidget( groupSource, 0, 0  );
  mainGrid->addWidget( m_writerSelectionWidget, 1, 0  );
  mainGrid->addWidget( tabWidget, 2, 0 );
  mainGrid->setRowStretch( 2, 1 );


  connect( m_comboSourceDevice, SIGNAL(activated(int)), this, SLOT(slotSourceSelected()) );
  connect( m_writerSelectionWidget, SIGNAL(writerChanged()), this, SLOT(slotSourceSelected()) );

  connect( m_checkOnTheFly, SIGNAL(toggled(bool)), m_tempDirSelectionWidget, SLOT(setDisabled(bool)) );
  connect( m_checkOnTheFly, SIGNAL(toggled(bool)), m_checkDeleteImages, SLOT(setDisabled(bool)) );
  connect( m_checkOnTheFly, SIGNAL(toggled(bool)), m_checkRawCopy, SLOT(setDisabled(bool)) );

  connect( m_checkOnlyCreateImage, SIGNAL(toggled(bool)), this, SLOT(slotOnlyCreateImageChecked(bool)) );
  connect( m_checkOnlyCreateImage, SIGNAL(toggled(bool)), m_writerSelectionWidget, SLOT(setDisabled(bool)) );
  connect( m_checkOnlyCreateImage, SIGNAL(toggled(bool)), m_spinCopies, SLOT(setDisabled(bool)) );
  connect( m_checkOnlyCreateImage, SIGNAL(toggled(bool)), m_checkSimulate, SLOT(setDisabled(bool)) );
  connect( m_checkOnlyCreateImage, SIGNAL(toggled(bool)), m_checkDeleteImages, SLOT(setDisabled(bool)) );

  connect( m_checkTaoSource, SIGNAL(toggled(bool)), m_spinTaoSourceAdjust, SLOT(setEnabled(bool)) );
  connect( m_checkTaoSource, SIGNAL(toggled(bool)), taoSourceAdjustLabel, SLOT(setEnabled(bool)) );

  initReadingDevices();
  slotSourceSelected();
  slotLoadUserDefaults();


  // ToolTips
  // --------------------------------------------------------------------------------
  QToolTip::add( m_checkFastToc, i18n("Do not extract pre-gaps and index marks") );
  QToolTip::add( m_comboSourceDevice, i18n("Select the drive with the CD to duplicate") );
  QToolTip::add( m_spinCopies, i18n("Number of copies") );
  QToolTip::add( m_checkRawCopy, i18n("Write all data sectors as 2352 byte blocks") );
  QToolTip::add( m_comboSubchanMode, i18n("Set the sub-channel data to be extracted") );
  QToolTip::add( m_checkTaoSource, i18n("Set this for discs written in TAO mode") );
  QToolTip::add( m_spinTaoSourceAdjust, i18n("Set the number of link blocks for TAO sources") );
  QToolTip::add( m_checkForce, i18n("Force write operation") );


  // What's This info
  // --------------------------------------------------------------------------------
  QWhatsThis::add( m_checkFastToc, i18n("<p>If this option is checked, K3b will ignore any pregaps and index marks "
					"on an audio CD. "
					"The copy will sound exactly like the original. The only difference is that "
					"the display of your audio cd player might show different values."
					"<p><b>Caution:</b> This may result in faster reading but does not guarantee "
					"an exact copy.") );
  QWhatsThis::add( m_comboSourceDevice, i18n("<p>Here you should select the drive which contains the CD to copy.") );
  QWhatsThis::add( m_spinCopies, i18n("<p>Select how many copies you want K3b to create from the CD.") );
  QWhatsThis::add( m_checkRawCopy, i18n("<p>If this option is checked, K3b will write all data sectors as 2352 byte "
					"blocks. No error correction will be applied. Use this if you have problems "
					"with reading data cds."
					"<p>Has no effect on audio cds."
					"<p>Does not work in on-the-fly mode.") );
  QWhatsThis::add( m_comboSubchanMode, i18n("<p>Specifies the type of sub-channel data " 
              "that is extracted from the source CD."
              "<ul><li><b>rw</b>: packed R-W sub-channel data, deinterleaved and error corrected.</li>"
              "<li><b>rw_raw</b>: raw R-W sub-channel data, not de-interleaved, not error corrected, "
              "L-EC data included in the track image.</li></ul>"
              "<p>If this option is not selected, no sub-channel data will be extracted." ) );
  QWhatsThis::add( m_checkTaoSource, i18n("<p>Select this option, if the source CD was written in TAO mode. "
              "It will be  assumed  that the  pre-gap length between all tracks (except between two audio "
              "tracks) is the standard 150  blocks  plus  the  number  of  link blocks  (usually 2). "
              "The number of link blocks can be controlled with option <b>TAO&nbsp Source&nbsp Adjust</b>. "
              "<p>Use this option only if you get error  messages in the transition areas between two tracks. "
              "<p>If you use this option with pressed CDs or CDs written in DAO mode "
              "you will get wrong results." ) );
  QWhatsThis::add( m_spinTaoSourceAdjust, i18n("see <b>TAO&nbsp Source") );
  QWhatsThis::add( m_checkForce, i18n("<p>Forces the execution of an operation that otherwise would not be performed") );
}


K3bCdCopyDialog::~K3bCdCopyDialog()
{
}


void K3bCdCopyDialog::initReadingDevices()
{
  // simple thing: if the used cdrdao version and the kernel do support
  // ATAPI we use it, otherwise we can only use SCSI devices
  const K3bExternalBin* cdrdaoBin = k3bcore->externalBinManager()->binObject("cdrdao");

  if( cdrdaoBin ) {
    QPtrList<K3bDevice> devices = k3bcore->deviceManager()->allDevices();
    K3bDevice* dev = devices.first();
    while( dev ) {
      if( dev->interfaceType() == K3bDevice::SCSI ||
	  (K3bCdDevice::plainAtapiSupport() && cdrdaoBin->hasFeature("plain-atapi")) ||
	  (K3bCdDevice::hackedAtapiSupport() && cdrdaoBin->hasFeature("hacked-atapi")) )
	m_comboSourceDevice->insertItem( dev->vendor() + " " + dev->description() + " (" + dev->blockDeviceName() + ")" );
      dev = devices.next();
    }
      
    if ( !devices.first() )
      m_buttonStart->setEnabled(false);
  }
  else {
    kdError() << "(K3bCdCopyDialog) Could not find cdrdao." << endl;
    m_buttonStart->setEnabled(false);
  }
}

void K3bCdCopyDialog::slotSourceSelected()
{
  K3bDevice* writer = m_writerSelectionWidget->writerDevice();
  K3bDevice* reader = readingDevice();

  if( writer == reader || m_checkOnlyCreateImage->isChecked() )
    m_checkOnTheFly->setChecked( false );
  m_checkOnTheFly->setDisabled( writer == reader || m_checkOnlyCreateImage->isChecked() );
}


K3bDevice* K3bCdCopyDialog::readingDevice() const
{
  const QString s = m_comboSourceDevice->currentText();

  QString strDev = s.mid( s.find('(') + 1, s.find(')') - s.find('(') - 1 );
 
  K3bDevice* dev =  k3bcore->deviceManager()->deviceByName( strDev );
  if( !dev )
    kdDebug() << "(K3bCdCopyDialog) could not find device " << s << endl;
		
  return dev;
}


void K3bCdCopyDialog::slotStartClicked()
{
  K3bCdCopyJob* job = new K3bCdCopyJob( this );
  
  job->setWriter( m_writerSelectionWidget->writerDevice() );
  job->setSpeed( m_writerSelectionWidget->writerSpeed() );
  job->setReader( readingDevice() );
  job->setDummy( m_checkSimulate->isChecked() );
  job->setOnTheFly( m_checkOnTheFly->isChecked() );
  job->setKeepImage( !m_checkDeleteImages->isChecked() );
  job->setOnlyCreateImage( m_checkOnlyCreateImage->isChecked() );
  job->setFastToc( m_checkFastToc->isChecked() );
  job->setTempPath( m_tempDirSelectionWidget->tempPath() );
  if( !m_checkSimulate->isChecked() )
    job->setCopies( m_spinCopies->value() );
  job->setReadRaw( m_checkRawCopy->isChecked() );
  job->setParanoiaMode( m_comboParanoiaMode->currentText().toInt() );
  if ( m_checkTaoSource->isChecked() ) {
    job->setTaoSource(true);
    if (m_spinTaoSourceAdjust->value() != 2)
      job->setTaoSourceAdjust( m_spinTaoSourceAdjust->value() );
  }
  QString submode = m_comboSubchanMode->currentText(); 
  if ( submode == "rw" )
    job->setReadSubchan(K3bCdrdaoWriter::RW);
  else if ( submode == "rw_raw" )
    job->setReadSubchan(K3bCdrdaoWriter::RW_RAW);
  job->setForce(m_checkForce->isChecked());

  // create a progresswidget
  K3bBurnProgressDialog d( this, "burnProgress", 
			   true /*!m_checkOnTheFly->isChecked() && !m_checkOnlyCreateImage->isChecked()*/,
			   !m_checkOnlyCreateImage->isChecked() );

  hide();

  d.startJob(job);

  close();
}


void K3bCdCopyDialog::slotOnlyCreateImageChecked( bool c )
{
  if( c ) 
    m_checkDeleteImages->setChecked( false );
  if ( !m_writerSelectionWidget->writerDevice() )
    m_buttonStart->setEnabled(c);
    
  // check if we can enable on-the-fly
  slotSourceSelected();
}


void K3bCdCopyDialog::slotLoadUserDefaults()
{
  KConfig* c = kapp->config();
  c->setGroup( "CD Copy" );

  m_checkSimulate->setChecked( c->readBoolEntry( "simulate", false ) );
  m_checkOnTheFly->setChecked( c->readBoolEntry( "on_the_fly", false ) );
  m_checkDeleteImages->setChecked( c->readBoolEntry( "delete_images", true ) );
  m_checkFastToc->setChecked( c->readBoolEntry( "fast_toc", false ) );
  m_checkRawCopy->setChecked( c->readBoolEntry( "raw_copy", false ) );
  m_checkTaoSource->setChecked( c->readBoolEntry( "tao_source", false ) );
  m_checkForce->setChecked( c->readBoolEntry( "force", false ) );
  m_spinTaoSourceAdjust->setValue( c->readNumEntry( "tao_source_adjust", 2 ) );
  m_checkOnlyCreateImage->setChecked( c->readBoolEntry( "only_create_image", false ) );
  m_comboParanoiaMode->setCurrentItem( c->readNumEntry( "paranoia_mode", 3 ) );

  QString subChMode = c->readEntry( "subchannel_mode", "none" );
  if( subChMode == "rw" )
    m_comboSubchanMode->setCurrentItem(1);
  else if( subChMode == "rw_raw" )
    m_comboSubchanMode->setCurrentItem(2);
  else
    m_comboSubchanMode->setCurrentItem(0); // none

  m_spinCopies->setValue( c->readNumEntry( "copies", 1 ) );
}

void K3bCdCopyDialog::slotSaveUserDefaults()
{
  KConfig* c = kapp->config();
  c->setGroup( "CD Copy" );

  c->writeEntry( "simulate", m_checkSimulate->isChecked() );
  c->writeEntry( "on_the_fly", m_checkOnTheFly->isChecked() );
  c->writeEntry( "delete_images", m_checkDeleteImages->isChecked() );
  c->writeEntry( "fast_toc", m_checkFastToc->isChecked() );
  c->writeEntry( "raw_copy", m_checkRawCopy->isChecked() );
  c->writeEntry( "tao_source", m_checkTaoSource->isChecked() );
  c->writeEntry( "force", m_checkForce->isChecked() );
  c->writeEntry( "tao_source_adjust", m_spinTaoSourceAdjust->value() );
  c->writeEntry( "only_create_image", m_checkOnlyCreateImage->isChecked() );
  c->writeEntry( "paranoia_mode", m_comboParanoiaMode->currentText().toInt() );
  c->writeEntry( "subchannel_mode", m_comboSubchanMode->currentText() );
  c->writeEntry( "copies", m_spinCopies->value() );
}

void K3bCdCopyDialog::slotLoadK3bDefaults()
{
  m_checkSimulate->setChecked( false );
  m_checkOnTheFly->setChecked( false );
  m_checkDeleteImages->setChecked( true );
  m_checkFastToc->setChecked( false );
  m_checkRawCopy->setChecked( false );
  m_checkTaoSource->setChecked( false );
  m_checkForce->setChecked( false );
  m_spinTaoSourceAdjust->setValue(2);
  m_checkOnlyCreateImage->setChecked( false );
  m_comboParanoiaMode->setCurrentItem(3);
  m_comboSubchanMode->setCurrentItem(0); // none
  m_spinCopies->setValue(1);
}

#include "k3bcdcopydialog.moc"
