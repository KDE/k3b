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

#include <klocale.h>
#include <kdialog.h>

#include <qcombobox.h>
#include <qlabel.h>
#include <qlayout.h>



K3bWriterSelectionWidget::K3bWriterSelectionWidget(QWidget *parent, const char *name )
  : QGroupBox( parent, name )
{
  setTitle( i18n( "Burning Device" ) );
  setColumnLayout(0, Qt::Vertical );
  layout()->setSpacing( 0 );
  layout()->setMargin( 0 );

  QGridLayout* mainLayout = new QGridLayout( layout() );
  mainLayout->setAlignment( Qt::AlignTop );
  mainLayout->setSpacing( KDialog::spacingHint() );
  mainLayout->setMargin( KDialog::marginHint() );

  QLabel* labelSpeed = new QLabel( this, "TextLabel1" );
  labelSpeed->setText( i18n( "Burning Speed" ) );
    
  m_comboSpeed = new QComboBox( FALSE, this, "m_comboSpeed" );
  m_comboSpeed->setAutoMask( FALSE );
  m_comboSpeed->setDuplicatesEnabled( FALSE );
    
  m_comboWriter = new QComboBox( FALSE, this, "m_comboWriter" );

  QLabel* labelDevice = new QLabel( this, "TextLabel1_2" );
  labelDevice->setText( i18n( "Device" ) );

  mainLayout->addWidget( labelDevice, 0, 0 );
  mainLayout->addWidget( labelSpeed, 0, 1 );
  mainLayout->addWidget( m_comboWriter, 1, 0 );  
  mainLayout->addWidget( m_comboSpeed, 1, 1 );
  
  mainLayout->setColStretch( 0, 1 );
  
  connect( m_comboWriter, SIGNAL(activated(int)), this, SLOT(slotRefreshWriterSpeeds()) );
  connect( m_comboWriter, SIGNAL(activated(int)), this, SIGNAL(writerChanged()) );



  // -- read cd-writers ----------------------------------------------
  QList<K3bDevice> _devices = k3bMain()->deviceManager()->burningDevices();
  K3bDevice* _dev = _devices.first();
  while( _dev ) {
    m_comboWriter->insertItem( _dev->vendor() + " " + _dev->description() + " (" + _dev->genericDevice() + ")" );
    _dev = _devices.next();
  }
  
  slotRefreshWriterSpeeds(); 
}


K3bWriterSelectionWidget::~K3bWriterSelectionWidget()
{
}


void K3bWriterSelectionWidget::slotRefreshWriterSpeeds()
{
  if( K3bDevice* _dev = writerDevice() ) {
    // add speeds to combobox
    m_comboSpeed->clear();
    m_comboSpeed->insertItem( "1x" );
    int _speed = 2;
    while( _speed <= _dev->maxWriteSpeed() ) {
      m_comboSpeed->insertItem( QString( "%1x" ).arg(_speed) );
      _speed+=2;
    }
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
  QString _strSpeed = m_comboSpeed->currentText();
  _strSpeed.truncate( _strSpeed.find('x') );
	
  return _strSpeed.toInt();
}


#include "k3bwriterselectionwidget.moc"
