/***************************************************************************
                          k3bcdinfodialog.cpp  -  description
                             -------------------
    begin                : Mon Oct 29 2001
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

#include "k3bcdinfodialog.h"
#include "k3bcdinfo.h"
#include "../k3b.h"
#include "../device/k3bdevice.h"
#include "../device/k3bdevicemanager.h"

#include <qlayout.h>
#include <qcombobox.h>
#include <qlist.h>
#include <qlabel.h>
#include <klocale.h>
#include <qpushbutton.h>



K3bCdInfoDialog::K3bCdInfoDialog( QWidget* parent, const char* name, bool modal )
  : KDialogBase( parent, name, modal, i18n("CD Info"), 
		 KDialogBase::Close, KDialogBase::Close, true )
{
  QFrame* mainWidget = makeMainWidget();
  QGridLayout* mainGrid = new QGridLayout( mainWidget );

  mainGrid->setSpacing( spacingHint() );
  mainGrid->setMargin( 0 );

  m_comboDevice = new QComboBox( FALSE, mainWidget, "m_comboDevice" );
  m_cdInfo = new K3bCdInfo( mainWidget );
  m_buttonRefresh = new QPushButton( m_cdInfo->refreshAction()->iconSet(), 
				     m_cdInfo->refreshAction()->text(), mainWidget );

  mainGrid->addWidget( new QLabel( i18n( "Device" ), mainWidget, "TextLabel1_2" ), 0, 0 );
  mainGrid->addWidget( m_comboDevice, 0, 1 );
  mainGrid->addWidget( m_buttonRefresh, 0, 3 );
  mainGrid->addMultiCellWidget( m_cdInfo, 1, 1, 0, 3 );
  mainGrid->setColStretch( 2, 1 );

  connect( m_comboDevice, SIGNAL(activated(int)), this, SLOT(slotDeviceChanged()) );
  connect( m_buttonRefresh, SIGNAL(clicked()), m_cdInfo->refreshAction(), SLOT(activate()) );



  // -- read devices ----------------------------------------------
  QList<K3bDevice> devices = k3bMain()->deviceManager()->burningDevices();
  K3bDevice* dev = devices.first();
  while( dev ) {
    m_comboDevice->insertItem( dev->vendor() + " " + dev->description() + " (" + dev->devicename() + ")" );
    dev = devices.next();
  }
  
  devices = k3bMain()->deviceManager()->readingDevices();
  dev = devices.first();
  while( dev ) {
    m_comboDevice->insertItem( dev->vendor() + " " + dev->description() + " (" + dev->devicename() + ")" );
    dev = devices.next();
  }
  // ------------------------------------------------------------------




  slotDeviceChanged();
}


K3bCdInfoDialog::~K3bCdInfoDialog()
{
}


void K3bCdInfoDialog::slotDeviceChanged()
{
  const QString s = m_comboDevice->currentText();
  
  QString strDev = s.mid( s.find('(') + 1, s.find(')') - s.find('(') - 1 );
 
  K3bDevice* dev =  k3bMain()->deviceManager()->deviceByName( strDev );
  if( !dev )
    qDebug( "(K3bCdInfoDialog) could not find device " + s );
  else
    m_cdInfo->setDevice( dev );
}


#include "k3bcdinfodialog.moc"
