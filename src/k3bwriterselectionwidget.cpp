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

#include "device/k3bdevice.h"
#include "device/k3bdevicemanager.h"
#include "k3b.h"
#include "tools/k3bglobals.h"
#include <k3bcore.h>

#include <klocale.h>
#include <kdialog.h>
#include <kconfig.h>

#include <qcombobox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qradiobutton.h>
#include <qhbuttongroup.h>
#include <qtooltip.h>
#include <qwhatsthis.h>
#include <qmap.h>
#include <qptrvector.h>


class K3bWriterSelectionWidget::Private
{
public:
  QMap<QString, int> indexMap;
  QPtrVector<K3bDevice> devices;
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

  m_comboSpeed = new QComboBox( FALSE, groupWriter, "m_comboSpeed" );
  m_comboSpeed->setAutoMask( FALSE );
  m_comboSpeed->setDuplicatesEnabled( FALSE );

  m_comboWriter = new QComboBox( FALSE, groupWriter, "m_comboWriter" );

  QLabel* labelDevice = new QLabel( groupWriter, "TextLabel1_2" );
  labelDevice->setText( i18n( "Device:" ) );

  groupWriterLayout->addWidget( labelDevice, 0, 0 );
  groupWriterLayout->addWidget( labelSpeed, 0, 1 );
  groupWriterLayout->addWidget( m_comboWriter, 1, 0 );
  groupWriterLayout->addWidget( m_comboSpeed, 1, 1 );
  groupWriterLayout->setColStretch( 0, 1 );


  m_groupCdWritingApp = new QHButtonGroup( i18n("Writing Application"), this );
  m_groupCdWritingApp->setExclusive( true );
  m_selectDefault  = new QRadioButton( i18n("Default"), m_groupCdWritingApp );
  m_selectCdrecord = new QRadioButton( "cdrecord", m_groupCdWritingApp );
  m_selectCdrdao   = new QRadioButton( "cdrdao", m_groupCdWritingApp );


  QGridLayout* mainLayout = new QGridLayout( this );
  mainLayout->setAlignment( Qt::AlignTop );
  mainLayout->setSpacing( KDialog::spacingHint() );
  mainLayout->setMargin( 0 );

  mainLayout->addWidget( groupWriter, 0, 0 );
  mainLayout->addWidget( m_groupCdWritingApp, 1, 0 );


  connect( m_comboWriter, SIGNAL(activated(int)), this, SLOT(slotRefreshWriterSpeeds()) );
  connect( m_comboWriter, SIGNAL(activated(int)), this, SIGNAL(writerChanged()) );
  connect( m_groupCdWritingApp, SIGNAL(clicked(int)), this, SLOT(slotWritingAppSelected(int)) );
  connect( this, SIGNAL(writerChanged()), SLOT(slotWriterChanged()) );

  m_groupCdWritingApp->setButton( 0 );

  // TODO: probably use KApplication::settingsChanged
  connect( k3bMain(), SIGNAL(configChanged(KConfig*)), this, SLOT(slotConfigChanged(KConfig*)) );
  connect( m_comboSpeed, SIGNAL(activated(int)), this, SLOT(slotSpeedChanged(int)) );



  // ToolTips
  // --------------------------------------------------------------------------------
  QToolTip::add( m_comboWriter, i18n("The CD writer that will write the CD") );
  QToolTip::add( m_comboSpeed, i18n("The speed at which to write the CD") );

  // What's This info
  // --------------------------------------------------------------------------------
  QWhatsThis::add( m_comboWriter, i18n("<p>Select the CD writer that you want to write the CD."
				       "<p>In most cases there will only be one writer available which "
				       "does not leave much choice.") );
  QWhatsThis::add( m_comboSpeed, i18n("<p>Select the speed with which you want the writer to burn."
				      "<p>1x speed means 150 KB/s."
				      "<p><b>Caution:</b> Make sure your system is able to send the data "
				      "fast enough to prevent buffer underruns.") );
  init();
}


K3bWriterSelectionWidget::~K3bWriterSelectionWidget()
{
  delete d;
}


void K3bWriterSelectionWidget::init()
{
  d->indexMap.clear();
  d->devices.clear();
  m_comboWriter->clear();

  // -- read cd-writers ----------------------------------------------
  QPtrList<K3bDevice> devices = k3bcore->deviceManager()->burningDevices();
  d->devices.resize( devices.count() );
  K3bDevice* dev = devices.first();
  int i = 0;
  while( dev ) {
    m_comboWriter->insertItem( dev->vendor() + " " + dev->description() + " (" + dev->blockDeviceName() + ")", i );
    d->indexMap[dev->devicename()] = i;
    d->devices.insert(i, dev);
    ++i;
    dev = devices.next();
  }

  slotRefreshWriterSpeeds();
  slotConfigChanged(kapp->config());
  kapp->config()->setGroup( "General Settings" );
  setWriterDevice( k3bcore->deviceManager()->deviceByName( kapp->config()->readEntry( "current_writer" ) ) );
}


void K3bWriterSelectionWidget::slotConfigChanged( KConfig* c )
{
  c->setGroup("General Options");
  bool manualAppSelect = c->readBoolEntry( "Manual writing app selection", false );
  if( manualAppSelect ) {
    m_groupCdWritingApp->show();
  }
  else {
    m_groupCdWritingApp->setButton( 0 );
    m_groupCdWritingApp->hide();
  }
}


void K3bWriterSelectionWidget::slotRefreshWriterSpeeds()
{
  if( K3bDevice* dev = writerDevice() ) {
    // add speeds to combobox
    m_comboSpeed->clear();
    m_comboSpeed->insertItem( "1x" );
    int speed = 2;
    int currentSpeedIndex = 0;
    while( speed <= dev->maxWriteSpeed() ) {
      m_comboSpeed->insertItem( QString( "%1x" ).arg(speed) );
      if( speed == dev->currentWriteSpeed() )
	currentSpeedIndex = m_comboSpeed->count() - 1;
      speed += 2;
    }

    // set to saved speed
    m_comboSpeed->setCurrentItem( currentSpeedIndex );
  }
}


void K3bWriterSelectionWidget::slotWritingAppSelected( int id )
{
  switch( id ) {
  case 1:
    emit writingAppChanged( K3b::CDRECORD );
    break;
  case 2:
    emit writingAppChanged( K3b::CDRDAO );
    break;
  case 0:
  default:
    emit writingAppChanged( K3b::DEFAULT );
    break;
  }
}


K3bDevice* K3bWriterSelectionWidget::writerDevice() const
{
  return d->devices[m_comboWriter->currentItem()];
}


void K3bWriterSelectionWidget::setWriterDevice( K3bDevice* dev )
{
  if( dev ) {
    if( d->indexMap.contains(dev->devicename()) )
      m_comboWriter->setCurrentItem( d->indexMap[dev->devicename()] );
  }
}


int K3bWriterSelectionWidget::writerSpeed() const
{
  QString strSpeed = m_comboSpeed->currentText();
  strSpeed.truncate( strSpeed.find('x') );

  return strSpeed.toInt();
}


int K3bWriterSelectionWidget::writingApp() const
{
  KConfig* c = kapp->config();
  c->setGroup("General Options");
  if( c->readBoolEntry( "Manual writing app selection", false ) ) {
    switch( m_groupCdWritingApp->id( m_groupCdWritingApp->selected() ) ) {
    case 1:
      return K3b::CDRECORD;
    case 2:
      return K3b::CDRDAO;
    case 0:
    default:
      return K3b::DEFAULT;
    }
  }
  else
    return K3b::DEFAULT;
}


void K3bWriterSelectionWidget::slotSpeedChanged( int )
{
  writerDevice()->setCurrentWriteSpeed( writerSpeed() );
}


void K3bWriterSelectionWidget::slotWriterChanged()
{
  // save last selected writer
  if( writerDevice() ) {
    kapp->config()->setGroup( "General Settings" );
    kapp->config()->writeEntry( "current_writer", writerDevice()->devicename() );
  }
}

#include "k3bwriterselectionwidget.moc"
