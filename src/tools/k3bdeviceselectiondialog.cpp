/***************************************************************************
                          k3bdeviceselectiondialog.cpp
                             -------------------
    begin                : Tue Nov 12 2002
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


#include "k3bdeviceselectiondialog.h"
#include <device/k3bdevice.h>
#include <device/k3bdevicemanager.h>

#include <qcombobox.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qstring.h>
#include <qframe.h>

#include <klocale.h>


K3bDeviceSelectionDialog::K3bDeviceSelectionDialog( bool reader, 
						    bool writer, 
						    QWidget* parent, 
						    const char* name, 
						    const QString& text,
						    bool modal )
  : KDialogBase( KDialogBase::Plain, 
		 i18n("Device Selection"), 
		 KDialogBase::Ok, 
		 KDialogBase::Ok,
		 parent,
		 name,
		 modal )
{
  if( !reader && !writer )
    reader = true;

  QGridLayout* lay = new QGridLayout( plainPage() );

  QLabel* label = new QLabel( text.isEmpty() ? i18n("Please select a device") : text, plainPage() );
  m_comboDevices = new QComboBox( plainPage() );

  lay->addWidget( label, 0, 0 );
  lay->addWidget( m_comboDevices, 1, 0 );
  lay->setRowStretch( 2, 1 );

  if( writer ) {
    // -- read cd-writers ----------------------------------------------
    QPtrList<K3bDevice> devices = K3bDeviceManager::self()->burningDevices();
    K3bDevice* dev = devices.first();
    while( dev ) {
      m_comboDevices->insertItem( dev->vendor() + " " + dev->description() + " (" + dev->ioctlDevice() + ")" );
      dev = devices.next();
    }
  }
  if( reader ) {
    // -- read cd-writers ----------------------------------------------
    QPtrList<K3bDevice> devices = K3bDeviceManager::self()->readingDevices();
    K3bDevice* dev = devices.first();
    while( dev ) {
      m_comboDevices->insertItem( dev->vendor() + " " + dev->description() + " (" + dev->ioctlDevice() + ")" );
      dev = devices.next();
    }
  }
}


K3bDeviceSelectionDialog::~K3bDeviceSelectionDialog()
{
}


K3bDevice* K3bDeviceSelectionDialog::selectedDevice() const
{
  const QString& s = m_comboDevices->currentText();

  QString strDev = s.mid( s.find('(') + 1, s.find(')') - s.find('(') - 1 );
 
  K3bDevice* dev =  K3bDeviceManager::self()->deviceByName( strDev );
  if( !dev )
    kdDebug() << "(K3bDeviceSelectionDialog) could not find device " << s << endl;
		
  return dev;
}


K3bDevice* K3bDeviceSelectionDialog::selectWriter( QWidget* parent, const QString& text )
{
  K3bDeviceSelectionDialog d( false, true, parent, 0, text );
  d.exec();
  return d.selectedDevice();
}


#include "k3bdeviceselectiondialog.moc"
