/***************************************************************************
                          k3bwriterselectionwidget.cpp  -  description
                             -------------------
    begin                : Mon Dec 3 2001
    copyright            : (C) 2001 by Sebastian Trueg
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

#include "k3bwriterselectionwidget.h"

#include "device/k3bdevice.h"
#include "device/k3bdevicemanager.h"
#include "k3b.h"
#include "tools/k3bglobals.h"

#include <klocale.h>
#include <kdialog.h>
#include <kconfig.h>

#include <qcombobox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qradiobutton.h>
#include <qhbuttongroup.h>


K3bWriterSelectionWidget::K3bWriterSelectionWidget(QWidget *parent, const char *name )
  : QWidget( parent, name )
{
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
  labelSpeed->setText( i18n( "Burning speed" ) );
    
  m_comboSpeed = new QComboBox( FALSE, groupWriter, "m_comboSpeed" );
  m_comboSpeed->setAutoMask( FALSE );
  m_comboSpeed->setDuplicatesEnabled( FALSE );
    
  m_comboWriter = new QComboBox( FALSE, groupWriter, "m_comboWriter" );

  QLabel* labelDevice = new QLabel( groupWriter, "TextLabel1_2" );
  labelDevice->setText( i18n( "Device" ) );

  groupWriterLayout->addWidget( labelDevice, 0, 0 );
  groupWriterLayout->addWidget( labelSpeed, 0, 1 );
  groupWriterLayout->addWidget( m_comboWriter, 1, 0 );  
  groupWriterLayout->addWidget( m_comboSpeed, 1, 1 );
  groupWriterLayout->setColStretch( 0, 1 );


  m_groupCdWritingApp = new QHButtonGroup( i18n("Writing Application"), this );
  m_groupCdWritingApp->setExclusive( true );
  m_selectDefault  = new QRadioButton( i18n("Default"), m_groupCdWritingApp );
  m_selectCdrecord = new QRadioButton( i18n("Cdrecord"), m_groupCdWritingApp );
  m_selectCdrdao   = new QRadioButton( i18n("Cdrdao"), m_groupCdWritingApp );


  QGridLayout* mainLayout = new QGridLayout( this );
  mainLayout->setAlignment( Qt::AlignTop );
  mainLayout->setSpacing( KDialog::spacingHint() );
  mainLayout->setMargin( 0 );

  mainLayout->addWidget( groupWriter, 0, 0 );
  mainLayout->addWidget( m_groupCdWritingApp, 1, 0 );
  

  connect( m_comboWriter, SIGNAL(activated(int)), this, SLOT(slotRefreshWriterSpeeds()) );
  connect( m_comboWriter, SIGNAL(activated(int)), this, SIGNAL(writerChanged()) );
  connect( m_groupCdWritingApp, SIGNAL(clicked(int)), this, SLOT(slotWritingAppSelected(int)) );


  // -- read cd-writers ----------------------------------------------
  QPtrList<K3bDevice> _devices = k3bMain()->deviceManager()->burningDevices();
  K3bDevice* _dev = _devices.first();
  while( _dev ) {
    m_comboWriter->insertItem( _dev->vendor() + " " + _dev->description() + " (" + _dev->genericDevice() + ")" );
    _dev = _devices.next();
  }
  
  slotRefreshWriterSpeeds(); 
  slotConfigChanged(k3bMain()->config());
  m_groupCdWritingApp->setButton( 0 );

  connect( k3bMain(), SIGNAL(configChanged(KConfig*)), this, SLOT(slotConfigChanged(KConfig*)) );
  connect( m_comboSpeed, SIGNAL(activated(int)), this, SLOT(slotSpeedChanged(int)) );
}


K3bWriterSelectionWidget::~K3bWriterSelectionWidget()
{
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
  const QString s = m_comboWriter->currentText();

  QString strDev = s.mid( s.find('(') + 1, s.find(')') - s.find('(') - 1 );
 
  K3bDevice* dev =  k3bMain()->deviceManager()->deviceByName( strDev );
  if( !dev )
    qDebug( "(K3bWriterSelectionWidget) could not find device " + s );
		
  return dev;
}


int K3bWriterSelectionWidget::writerSpeed() const
{
  QString strSpeed = m_comboSpeed->currentText();
  strSpeed.truncate( strSpeed.find('x') );
	
  return strSpeed.toInt();
}


int K3bWriterSelectionWidget::writingApp() const
{
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


void K3bWriterSelectionWidget::slotSpeedChanged( int index )
{
  writerDevice()->setCurrentWriteSpeed( writerSpeed() );
}


#include "k3bwriterselectionwidget.moc"
