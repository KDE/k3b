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
#include "../k3bstdguiitems.h"
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
#include <qtabwidget.h>
#include <qwhatsthis.h>
#include <qhbox.h>


K3bCdCopyDialog::K3bCdCopyDialog( QWidget *parent, const char *name, bool modal )
  : KDialogBase( KDialogBase::Plain, i18n("Copy CD"), User1|User2, User1, parent, name, modal, false, 
		 KGuiItem( i18n("&Copy"), "copy", i18n("Start CD Copy") ), KStdGuiItem::close() )
{
  setButtonBoxOrientation( Qt::Vertical );

  QFrame* main = plainPage();

  QGridLayout* mainGrid = new QGridLayout( main );
  mainGrid->setSpacing( spacingHint() );
  mainGrid->setMargin( 0 );

  m_writerSelectionWidget = new K3bWriterSelectionWidget( main );

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
  p->setStretchFactor(new QLabel( i18n("Paranoia Mode:"), p ), 1 );
  m_comboParanoiaMode = K3bStdGuiItems::paranoiaModeComboBox( p );

  QGroupBox* groupRaw   = new QGroupBox( 2, Qt::Vertical, i18n("Read Raw"), advancedTab ); 
  groupRaw->setInsideSpacing( spacingHint() );
  groupRaw->setInsideMargin( marginHint() );
  m_checkRawCopy = new QCheckBox( i18n("Raw Copy"), groupRaw );
  QHBox *s = new QHBox( groupRaw );
  s->setStretchFactor(new QLabel( i18n("Read Subchan Mode:"), s ), 1 );
  m_comboSubchanMode = new QComboBox( s );
  m_comboSubchanMode->insertItem( "none" );
  m_comboSubchanMode->insertItem( "rw" );
  m_comboSubchanMode->insertItem( "rw_raw" );
 
  QGroupBox* groupTao   = new QGroupBox( 2, Qt::Vertical, i18n("Track at Once Source"), advancedTab ); 
  groupTao->setInsideSpacing( spacingHint() );
  groupTao->setInsideMargin( marginHint() );
  m_checkTaoSource = new QCheckBox( i18n("TAO Source"), groupTao );
  QHBox *t = new QHBox( groupTao );
  QLabel* taoSourceAdjustLabel = new QLabel( i18n("Tao Source Adjust:"), t);
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
  m_checkForce = new QCheckBox( i18n("Force Write"), groupOther );

  advancedTabGrid->addWidget( groupAudio, 0, 0 );  
  advancedTabGrid->addWidget( groupRaw,   0, 1 );
  advancedTabGrid->addWidget( groupTao,   1, 0 );
  advancedTabGrid->addWidget( groupOther, 1, 1 );


  tabWidget->addTab( advancedTab, i18n("&Advanced") );

  mainGrid->addWidget( groupSource, 0, 0  );
  mainGrid->addWidget( m_writerSelectionWidget, 1, 0  );
  mainGrid->addWidget( tabWidget, 2, 0 );
  mainGrid->setRowStretch( 2, 1 );


  // -- read cd-devices ----------------------------------------------
  QPtrList<K3bDevice> devices = k3bMain()->deviceManager()->readingDevices();
  K3bDevice* dev = devices.first();
  while( dev ) {
    // cdrdao only supports SCSI devices
    if( dev->interfaceType() == K3bDevice::SCSI )
      m_comboSourceDevice->insertItem( dev->vendor() + " " + dev->description() + " (" + dev->blockDeviceName() + ")" );
    dev = devices.next();
  }
  devices = k3bMain()->deviceManager()->burningDevices();
  dev = devices.first();
  while( dev ) {
    // cdrdao only supports SCSI devices
    if( dev->interfaceType() == K3bDevice::SCSI )
      m_comboSourceDevice->insertItem( dev->vendor() + " " + dev->description() + " (" + dev->blockDeviceName() + ")" );
    dev = devices.next();
  }

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
  slotSourceSelected();

  m_checkDeleteImages->setChecked( true );


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
 
  K3bDevice* dev =  k3bMain()->deviceManager()->deviceByName( strDev );
  if( !dev )
    kdDebug() << "(K3bCdCopyDialog) could not find device " << s << endl;
		
  return dev;
}


void K3bCdCopyDialog::slotUser1()
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
  K3bBurnProgressDialog d( k3bMain(), "burnProgress", 
			   true /*!m_checkOnTheFly->isChecked() && !m_checkOnlyCreateImage->isChecked()*/,
			   !m_checkOnlyCreateImage->isChecked() );

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


void K3bCdCopyDialog::slotOnlyCreateImageChecked( bool c )
{
  if( c )
    m_checkDeleteImages->setChecked( false );

  // check if we can enable on-the-fly
  slotSourceSelected();
}


#include "k3bcdcopydialog.moc"
