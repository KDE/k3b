/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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


#include "k3bwriterselectionwidget.h"

#include "k3bdevicecombobox.h"
#include <device/k3bdevice.h>
#include <device/k3bdevicemanager.h>
#include <k3bglobals.h>
#include <k3bcore.h>

#include <klocale.h>
#include <kdialog.h>
#include <kconfig.h>
#include <kcombobox.h>

#include <qlabel.h>
#include <qlayout.h>
#include <qgroupbox.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qmap.h>
#include <qptrvector.h>


class K3bWriterSelectionWidget::Private
{
public:
  int maxSpeed;
};


K3bWriterSelectionWidget::K3bWriterSelectionWidget(QWidget *parent, const char *name )
  : QWidget( parent, name )
{
  d = new Private;

  QGroupBox* groupWriter = new QGroupBox( this );
  groupWriter->setTitle( i18n( "Burning Device" ) );
  groupWriter->setColumnLayout(0, Qt::Vertical );
  groupWriter->layout()->setSpacing( 0 );
  groupWriter->layout()->setMargin( 0 );

  QGridLayout* groupWriterLayout = new QGridLayout( groupWriter->layout() );
  groupWriterLayout->setAlignment( Qt::AlignTop );
  groupWriterLayout->setSpacing( KDialog::spacingHint() );
  groupWriterLayout->setMargin( KDialog::marginHint() );

  QLabel* labelSpeed = new QLabel( groupWriter, "TextLabel1" );
  labelSpeed->setText( i18n( "Speed:" ) );

  m_comboSpeed = new KComboBox( FALSE, groupWriter, "m_comboSpeed" );
  m_comboSpeed->setAutoMask( FALSE );
  m_comboSpeed->setDuplicatesEnabled( FALSE );

  m_comboWriter = new K3bDeviceComboBox( groupWriter, "m_comboWriter" );

  QLabel* labelDevice = new QLabel( groupWriter, "TextLabel1_2" );
  labelDevice->setText( i18n( "Device:" ) );

  m_writingAppLabel = new QLabel( i18n("Writing app:"), groupWriter );
  m_comboWritingApp = new KComboBox( groupWriter );

  groupWriterLayout->addWidget( labelDevice, 0, 0 );
  groupWriterLayout->addWidget( labelSpeed, 0, 1 );
  groupWriterLayout->addWidget( m_writingAppLabel, 0, 2 );
  groupWriterLayout->addWidget( m_comboWriter, 1, 0 );
  groupWriterLayout->addWidget( m_comboSpeed, 1, 1 );
  groupWriterLayout->addWidget( m_comboWritingApp, 1, 2 );
  groupWriterLayout->setColStretch( 0, 1 );


  QGridLayout* mainLayout = new QGridLayout( this );
  mainLayout->setAlignment( Qt::AlignTop );
  mainLayout->setSpacing( KDialog::spacingHint() );
  mainLayout->setMargin( 0 );

  mainLayout->addWidget( groupWriter, 0, 0 );


  connect( m_comboWriter, SIGNAL(activated(int)), this, SLOT(slotRefreshWriterSpeeds()) );
  connect( m_comboWriter, SIGNAL(activated(int)), this, SIGNAL(writerChanged()) );
  connect( m_comboWritingApp, SIGNAL(activated(int)), this, SLOT(slotWritingAppSelected(int)) );
  connect( this, SIGNAL(writerChanged()), SLOT(slotWriterChanged()) );

  connect( m_comboSpeed, SIGNAL(activated(int)), this, SLOT(slotSpeedChanged(int)) );



  // ToolTips
  // --------------------------------------------------------------------------------
  QToolTip::add( m_comboWriter, i18n("The CD writer that will write the CD") );
  QToolTip::add( m_comboSpeed, i18n("The speed at which to write the CD") );
  QToolTip::add( m_comboWritingApp, i18n("The external application to acually write the CD") );

  // What's This info
  // --------------------------------------------------------------------------------
  QWhatsThis::add( m_comboWriter, i18n("<p>Select the CD writer that you want to write the CD."
				       "<p>In most cases there will only be one writer available which "
				       "does not leave much choice.") );
  QWhatsThis::add( m_comboSpeed, i18n("<p>Select the speed with which you want the writer to burn."
				      "<p>1x speed means 150 KB/s."
				      "<p><b>Caution:</b> Make sure your system is able to send the data "
				      "fast enough to prevent buffer underruns.") );
  QWhatsThis::add( m_comboWritingApp, i18n("<p>K3b uses the command line tools cdrecord and cdrdao "
					   "to actually write the CDs. Normally K3b chooses the best "
					   "suited application for every task but in some cases it "
					   "may be possible that one of the applications does not "
					   "support a writer. In this case one may select the "
					   "application manually.") );

  init();
}


K3bWriterSelectionWidget::~K3bWriterSelectionWidget()
{
  delete d;
}


void K3bWriterSelectionWidget::init()
{
  m_comboWriter->clear();

  // -- read cd-writers ----------------------------------------------
  QPtrList<K3bDevice> devices = k3bcore->deviceManager()->burningDevices();
  K3bDevice* dev = devices.first();
  while( dev ) {
    m_comboWriter->addDevice( dev );
    dev = devices.next();
  }

  k3bcore->config()->setGroup( "General Settings" );
  K3bDevice *current = k3bcore->deviceManager()->deviceByName( k3bcore->config()->readEntry( "current_writer" ) );

  if ( current == 0 )
    current = devices.first();
  setWriterDevice( current );
  
  slotRefreshWriterSpeeds();
  
  // set to saved speed
  if( writerDevice() )
    setSpeed( writerDevice()->currentWriteSpeed() );

  slotConfigChanged(k3bcore->config());
  setSupportedWritingApps( K3b::CDRDAO|K3b::CDRECORD );
}


void K3bWriterSelectionWidget::slotConfigChanged( KConfig* c )
{
  c->setGroup("General Options");
  bool manualAppSelect = c->readBoolEntry( "Manual writing app selection", false );
  if( manualAppSelect ) {
    m_comboWritingApp->show();
    m_writingAppLabel->show();
  }
  else {
    m_comboWritingApp->hide();
    m_writingAppLabel->hide();
  }
}


void K3bWriterSelectionWidget::slotRefreshWriterSpeeds()
{
  if( K3bDevice* dev = writerDevice() ) {
    // add speeds to combobox
    m_comboSpeed->clear();
    m_comboSpeed->insertItem( "1x" );
    int speed = 2;
    while( speed <= dev->maxWriteSpeed() ) {
      m_comboSpeed->insertItem( QString( "%1x" ).arg(speed) );
      speed += 2;
    }

    d->maxSpeed = speed;
  }
}


void K3bWriterSelectionWidget::slotWritingAppSelected( int )
{
  emit writingAppChanged( selectedWritingApp() );
}


K3bDevice* K3bWriterSelectionWidget::writerDevice() const
{
  return m_comboWriter->selectedDevice();
}


void K3bWriterSelectionWidget::setWriterDevice( K3bDevice* dev )
{
  m_comboWriter->setSelectedDevice( dev );
}


void K3bWriterSelectionWidget::setSpeed( int s )
{
  m_comboSpeed->setCurrentItem( QString("%1x").arg(s), false );
}


int K3bWriterSelectionWidget::writerSpeed() const
{
  QString strSpeed = m_comboSpeed->currentText();
  strSpeed.truncate( strSpeed.find('x') );

  return strSpeed.toInt();
}


int K3bWriterSelectionWidget::writingApp() const
{
  KConfig* c = k3bcore->config();
  c->setGroup("General Options");
  if( c->readBoolEntry( "Manual writing app selection", false ) ) {
    return selectedWritingApp();
  }
  else
    return K3b::DEFAULT;
}


int K3bWriterSelectionWidget::selectedWritingApp() const
{
  QString s = m_comboWritingApp->currentText();
  if( s == "cdrdao" )
    return K3b::CDRDAO;
  else if( s == "cdrecord" )
    return K3b::CDRECORD;
  else if( s == "cdrecord-prodvd" )
    return K3b::CDRECORD_PRODVD;
  else if( s == "dvdrecord" )
    return K3b::DVDRECORD;
  else
    return K3b::DEFAULT;
}


void K3bWriterSelectionWidget::slotSpeedChanged( int )
{
  if( K3bCdDevice::CdDevice* dev = writerDevice() )
    dev->setCurrentWriteSpeed( writerSpeed() );
}


void K3bWriterSelectionWidget::slotWriterChanged()
{
  // save last selected writer
  if( writerDevice() ) {
    k3bcore->config()->setGroup( "General Settings" );
    k3bcore->config()->writeEntry( "current_writer", writerDevice()->devicename() );
  }
}


void K3bWriterSelectionWidget::setSupportedWritingApps( int i )
{
  m_comboWritingApp->clear();

  m_comboWritingApp->insertItem( i18n("Auto") );

  if( i & K3b::CDRDAO )
    m_comboWritingApp->insertItem( "cdrdao" );
  if( i & K3b::CDRECORD )
    m_comboWritingApp->insertItem( "cdrecord" );
  if( i & K3b::CDRECORD_PRODVD )
    m_comboWritingApp->insertItem( "cdrecord-prodvd" );
  if( i & K3b::DVDRECORD )
    m_comboWritingApp->insertItem( "dvdrecord" );
}


void K3bWriterSelectionWidget::loadConfig( KConfig* c )
{
  setWriterDevice( k3bcore->deviceManager()->findDevice( c->readEntry( "writer_device" ) ) );
  setSpeed( writerDevice() ? c->readNumEntry( "writing_speed",  writerDevice()->currentWriteSpeed() ) : 1 );
}


void K3bWriterSelectionWidget::saveConfig( KConfig* c )
{
  c->writeEntry( "writing_speed", writerSpeed() );
  c->writeEntry( "writer_device", writerDevice() ? writerDevice()->devicename() : QString::null );
}

#include "k3bwriterselectionwidget.moc"
