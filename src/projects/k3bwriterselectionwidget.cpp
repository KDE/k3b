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

#include <k3bdevicecombobox.h>
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
  bool dvd;
};


K3bWriterSelectionWidget::K3bWriterSelectionWidget( bool dvd, QWidget *parent, const char *name )
  : QWidget( parent, name )
{
  d = new Private;
  d->dvd = dvd;

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


  connect( m_comboWriter, SIGNAL(activated(int)), this, SIGNAL(writerChanged()) );
  connect( m_comboWritingApp, SIGNAL(activated(int)), this, SLOT(slotWritingAppSelected(int)) );
  connect( this, SIGNAL(writerChanged()), SLOT(slotWriterChanged()) );

  connect( m_comboSpeed, SIGNAL(activated(int)), this, SLOT(slotSpeedChanged(int)) );

  init();

  slotWriterChanged();
}


K3bWriterSelectionWidget::~K3bWriterSelectionWidget()
{
  delete d;
}


void K3bWriterSelectionWidget::setDvd( bool b )
{
  if( b != d->dvd ) {
    d->dvd = true;
    init();
  }
}

void K3bWriterSelectionWidget::init()
{
  m_comboWriter->clear();

  // -- read cd-writers ----------------------------------------------
  QPtrList<K3bDevice>& devices = ( d->dvd 
				   ? k3bcore->deviceManager()->dvdWriter() 
				   : k3bcore->deviceManager()->cdWriter() );

  K3bDevice* dev = devices.first();
  while( dev ) {
    m_comboWriter->addDevice( dev );
    dev = devices.next();
  }

  k3bcore->config()->setGroup( "General Options" );
  K3bDevice *current = k3bcore->deviceManager()->deviceByName( k3bcore->config()->readEntry( "current_writer" ) );

  if ( current == 0 )
    current = devices.first();
  setWriterDevice( current );
  
  slotRefreshWriterSpeeds();
  
  slotConfigChanged(k3bcore->config());

  if( d->dvd )
    setSupportedWritingApps( K3b::GROWISOFS );
  else
    setSupportedWritingApps( K3b::CDRDAO|K3b::CDRECORD );


  // ToolTips
  // --------------------------------------------------------------------------------
  QToolTip::remove( m_comboWriter );
  QToolTip::remove( m_comboSpeed );
  QToolTip::remove( m_comboWritingApp );

  if( d->dvd ) {
    QToolTip::add( m_comboWriter, i18n("The DVD writer that will write the DVD") );
    QToolTip::add( m_comboSpeed, i18n("The speed at which to write the DVD") );
    QToolTip::add( m_comboWritingApp, i18n("The external application to acually write the DVD") );
  }
  else {
    QToolTip::add( m_comboWriter, i18n("The CD writer that will write the CD") );
    QToolTip::add( m_comboSpeed, i18n("The speed at which to write the CD") );
    QToolTip::add( m_comboWritingApp, i18n("The external application to acually write the CD") );
  }

  // What's This info
  // --------------------------------------------------------------------------------
  QWhatsThis::remove( m_comboWriter );
  QWhatsThis::remove( m_comboSpeed );
  QWhatsThis::remove( m_comboWritingApp );

  if( d->dvd ) {
    QWhatsThis::add( m_comboWriter, i18n("<p>Select the DVD writer that you want to use."
					 "<p>In most cases there will only be one writer available which "
					 "does not leave much choice.") );
    QWhatsThis::add( m_comboSpeed, i18n("<p>Select the speed with which you want the writer to burn."
					"<p><b>Auto</b><br>"
					"Let the DVD writer decide which speed to use for the media. "
					"This is the recommended selection for most media.</p>"
					"<p><b>1x</b><br>"
					"Some DVD writers reportedly fail to determine optimal speed "
					"for no-name media and pick one higher than the media can stand "
					"which results in corrupted recordings. This option forces the "
					"writer to switch to 1x speed to be on the save side. When using "
					"no-name media it is recommended to use this option.</p>"
					"<p>1x refers to 1385 KB/s.</p>"
					"<p><b>Caution:</b> Be aware that the speed selection only makes "
					"sense for DVD-R(W) writers since DVD+R(W) writers always choose "
					"the speed automagically.") );
  }
  else {
    QWhatsThis::add( m_comboWriter, i18n("<p>Select the CD writer that you want to use."
					 "<p>In most cases there will only be one writer available which "
					 "does not leave much choice.") );
    QWhatsThis::add( m_comboSpeed, i18n("<p>Select the speed with which you want the writer to burn."
					"<p>1x refers to 150 KB/s."
					"<p><b>Caution:</b> Make sure your system is able to send the data "
					"fast enough to prevent buffer underruns.") );
  }
  QWhatsThis::add( m_comboWritingApp, i18n("<p>K3b uses the command line tools cdrecord, growisofs, and cdrdao "
					   "to actually write a CD/DVD. Normally K3b chooses the best "
					   "suited application for every task but in some cases it "
					   "may be possible that one of the applications does not "
					   "support a writer. In this case one may select the "
					   "application manually.") );
}


void K3bWriterSelectionWidget::slotConfigChanged( KConfig* c )
{
  QString oldGroup = c->group();
  c->setGroup("General Options");
  bool manualAppSelect = c->readBoolEntry( "Manual writing app selection", false );
  c->setGroup( oldGroup );
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
  if( d->dvd ) {
    m_comboSpeed->clear();
    m_comboSpeed->insertItem( i18n("Auto") );
    m_comboSpeed->insertItem( "1x" );
  }
  else if( K3bDevice* dev = writerDevice() ) {
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
  if( d->dvd && s == 0 )
    m_comboSpeed->setCurrentItem( 0 ); // Auto
  else if( d->dvd )
    m_comboSpeed->setCurrentItem( 1 ); // 1x
  else
    m_comboSpeed->setCurrentItem( QString("%1x").arg(s), false );
}


void K3bWriterSelectionWidget::setWritingApp( int app )
{
  switch( app ) {
  case K3b::CDRECORD:
    m_comboWritingApp->setCurrentItem( "cdrecord" );
    break;
  case K3b::CDRDAO:
    m_comboWritingApp->setCurrentItem( "cdrdao" );
    break;
  case K3b::DVDRECORD:
    m_comboWritingApp->setCurrentItem( "dvdrecord" );
    break;
  case K3b::GROWISOFS:
    m_comboWritingApp->setCurrentItem( "growisofs" );
    break;
  case K3b::DVD_RW_FORMAT:
    m_comboWritingApp->setCurrentItem( "dvd+rw-format" );
    break;
  default:
    m_comboWritingApp->setCurrentItem( 0 );  // Auto
    break;
  }
}


int K3bWriterSelectionWidget::writerSpeed() const
{
  if( d->dvd ) {
    // 0 for Auto
    // 1 for 1x
    return m_comboSpeed->currentItem();
  }
  else {
    QString strSpeed = m_comboSpeed->currentText();
    strSpeed.truncate( strSpeed.find('x') );
    
    return strSpeed.toInt();
  }
}


int K3bWriterSelectionWidget::writingApp() const
{
  KConfig* c = k3bcore->config();
  QString oldGroup = c->group();
  c->setGroup("General Options");
  bool b = c->readBoolEntry( "Manual writing app selection", false );
  c->setGroup( oldGroup );
  if( b ) {
    return selectedWritingApp();
  }
  else
    return K3b::DEFAULT;
}


int K3bWriterSelectionWidget::selectedWritingApp() const
{
  return writingAppFromString( m_comboWritingApp->currentText() );
}

// static
int K3bWriterSelectionWidget::writingAppFromString( const QString& s )
{
  if( s == "cdrdao" )
    return K3b::CDRDAO;
  else if( s == "cdrecord" )
    return K3b::CDRECORD;
  else if( s == "dvdrecord" )
    return K3b::DVDRECORD;
  else if( s == "growisofs" )
    return K3b::GROWISOFS;
  else if( s == "dvd+rw-format" )
    return K3b::DVD_RW_FORMAT;
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
  slotRefreshWriterSpeeds();

  // save last selected writer
  if( K3bDevice* dev = writerDevice() ) {
    QString oldGroup = k3bcore->config()->group();
    k3bcore->config()->setGroup( "General Options" );
    k3bcore->config()->writeEntry( "current_writer", dev->devicename() );
    k3bcore->config()->setGroup( oldGroup );
    setSpeed( dev->currentWriteSpeed() );
  }
}


void K3bWriterSelectionWidget::setSupportedWritingApps( int i )
{
  int oldApp = writingApp();

  m_comboWritingApp->clear();

  m_comboWritingApp->insertItem( i18n("Auto") );

  if( i & K3b::CDRDAO )
    m_comboWritingApp->insertItem( "cdrdao" );
  if( i & K3b::CDRECORD )
    m_comboWritingApp->insertItem( "cdrecord" );
  if( i & K3b::DVDRECORD )
    m_comboWritingApp->insertItem( "dvdrecord" );
  if( i & K3b::GROWISOFS )
    m_comboWritingApp->insertItem( "growisofs" );
  if( i & K3b::DVD_RW_FORMAT )
    m_comboWritingApp->insertItem( "dvd+rw-format" );

  setWritingApp( oldApp );
}


void K3bWriterSelectionWidget::loadConfig( KConfig* c )
{
  setWriterDevice( k3bcore->deviceManager()->findDevice( c->readEntry( "writer_device" ) ) );
  setSpeed( c->readNumEntry( "writing_speed",  
			     d->dvd 
			     ? 0 
			     : ( writerDevice() 
				 ? writerDevice()->currentWriteSpeed() 
				 : 1 ) ) );
  setWritingApp( writingAppFromString( c->readEntry( "writing_app" ) ) );
}


void K3bWriterSelectionWidget::saveConfig( KConfig* c )
{
  c->writeEntry( "writing_speed", writerSpeed() );
  c->writeEntry( "writer_device", writerDevice() ? writerDevice()->devicename() : QString::null );
  c->writeEntry( "writing_app", m_comboWritingApp->currentText() );
}

void K3bWriterSelectionWidget::loadDefaults()
{
  // ignore the writer and the writer speed in CD mode
  if( d->dvd )
    m_comboSpeed->setCurrentItem( 0 ); // Auto
  setWritingApp( K3b::DEFAULT );
}


void K3bWriterSelectionWidget::setForceAutoSpeed( bool b )
{
  m_comboSpeed->setDisabled(b);
}

#include "k3bwriterselectionwidget.moc"
