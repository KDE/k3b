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

#include "k3bwriterspeedverificationdialog.h"

#include <k3bcore.h>
#include <k3bversion.h>
#include <k3bdevicemanager.h>

#include <qlabel.h>
#include <qspinbox.h>
#include <qlayout.h>

#include <klocale.h>


K3bWriterSpeedVerificationDialog::K3bWriterSpeedVerificationDialog( QPtrList<K3bCdDevice::CdDevice>& wlist, 
								    QWidget* parent, const char* name )
  : KDialogBase( KDialogBase::Plain, i18n("Writer speed verification"), KDialogBase::Ok, KDialogBase::Ok, parent, name, true )
{
  QGridLayout* grid = new QGridLayout( plainPage() );
  grid->setSpacing( spacingHint() );
  grid->setMargin( marginHint() );

  QLabel* infoLabel = new QLabel( i18n("<p>It seems this is the first time you start K3b %1 "
				       "with this device configuration. Please verify if the writing "
				       "speeds have been detected properly and correct them if necessary.").arg(k3bcore->version()),
				  plainPage() );

  grid->addMultiCellWidget( infoLabel, 0, 0, 0, 1 );

  int row = 1;
  for( QPtrListIterator<K3bCdDevice::CdDevice> it( wlist ); it.current(); ++it ) {
    K3bCdDevice::CdDevice* dev = *it;

    QSpinBox* spin = new QSpinBox( plainPage() );
    spin->setSuffix( "x" );
    spin->setValue( dev->maxWriteSpeed()/175 );

    grid->addWidget( new QLabel( i18n("%1 %2 - CD writing speed:").arg(dev->vendor()).arg(dev->description()), plainPage() ), row, 0 );
    grid->addWidget( spin, row, 1 );

    m_spinMap.insert( spin, dev );

    connect( spin, SIGNAL(valueChanged(int)), this, SLOT(slotSpeedChanged(int)) );
    row++;
  }

  grid->setRowStretch( row, 1 );
}


K3bWriterSpeedVerificationDialog::~K3bWriterSpeedVerificationDialog()
{
}


void K3bWriterSpeedVerificationDialog::slotSpeedChanged( int speed )
{
  m_spinMap[dynamic_cast<const QSpinBox*>(sender())]->setMaxWriteSpeed( speed*175 );
}


void K3bWriterSpeedVerificationDialog::verify( QPtrList<K3bCdDevice::CdDevice>& wlist, QWidget* parent, const char* name )
{
  K3bWriterSpeedVerificationDialog d( wlist, parent, name );
  d.exec();
}

#include "k3bwriterspeedverificationdialog.moc"
