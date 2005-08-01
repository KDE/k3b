/* 
 *
 * $Id: sourceheader 380067 2005-01-19 13:03:46Z trueg $
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */

#include "k3bdatasessionimportdialog.h"

#include <cdinfo/k3bdiskinfodetector.h>
#include <k3bbusywidget.h>
#include <k3bdatadoc.h>
#include <k3btoc.h>
#include <k3bdiskinfo.h>
#include <k3biso9660.h>
#include <k3bcore.h>
#include <k3bdevicemanager.h>

#include <qlabel.h>
#include <qpushbutton.h>
#include <qcursor.h>
#include <qapplication.h>
#include <qlayout.h>
#include <qtimer.h>

#include <klistbox.h>
#include <klocale.h>


K3bDataSessionImportDialog::K3bDataSessionImportDialog( QWidget* parent )
  : KDialogBase( KDialogBase::Plain, 
		 i18n("Session Import"), 
		 KDialogBase::Ok|KDialogBase::Cancel,
		 KDialogBase::Ok,
		 parent, 
		 className(),
		 true,
		 true )
{
  QGridLayout* grid = new QGridLayout( plainPage() );
  grid->setMargin( 0 );
  grid->setSpacing( spacingHint() );

  QLabel* label = new QLabel( i18n("Sessions to import:"), plainPage() );
  m_selectionBox = new KListBox( plainPage() );
  m_processLabel = new QLabel( plainPage() );
  m_busyWidget = new K3bBusyWidget( plainPage() );

  grid->addMultiCellWidget( label, 0, 0, 0, 1 );
  grid->addMultiCellWidget( m_selectionBox, 1, 1, 0, 1 );
  grid->addWidget( m_processLabel, 2, 0 );
  grid->addWidget( m_busyWidget, 2, 1 );

  connect( m_selectionBox, SIGNAL(selectionChanged()),
	   this, SLOT(slotSelectionChanged()) );

  m_diskInfoDetector = new K3bDevice::DiskInfoDetector( this );
  connect( m_diskInfoDetector, SIGNAL(diskInfoReady(K3bDevice::DiskInfoDetector*)),
	   this, SLOT(slotDiskInfoReady(K3bDevice::DiskInfoDetector*)) );
}


K3bDataSessionImportDialog::~K3bDataSessionImportDialog()
{
}


void K3bDataSessionImportDialog::importSession( K3bDataDoc* doc )
{
  m_deviceMap.clear();
  m_selectionBox->clear();
  m_busyWidget->showBusy( true );
  m_processLabel->setText( i18n("Searching...") );

  m_alreadySelected = false;

  showButton( KDialogBase::Ok, false );
  showButton( KDialogBase::Cancel, true );

  actionButton( KDialogBase::Ok )->setEnabled( false );

  m_doc = doc;

  if( doc->type() == K3bDoc::DVD )
    m_devices = k3bcore->deviceManager()->dvdWriter();
  else
    m_devices = k3bcore->deviceManager()->cdWriter();

  QTimer::singleShot( 0, this, SLOT(checkNextDevice()) );
}


void K3bDataSessionImportDialog::slotOk()
{
  // stop searching other devices
  m_alreadySelected = true;

  // find the selected device, show a busy mouse cursor and call K3bDataDoc::importSession
  int index = m_selectionBox->index( m_selectionBox->selectedItem() );
  if( index != -1 ) {
    K3bDevice::Device* dev = m_deviceMap[index];

    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    m_doc->setBurner( dev );    
    m_doc->importSession( dev );

    QApplication::restoreOverrideCursor();

    done( 0 );
  }
  else
    done( 1 );
}


void K3bDataSessionImportDialog::slotCancel()
{
  KDialogBase::slotCancel();
}


void K3bDataSessionImportDialog::slotSelectionChanged()
{      
  actionButton( KDialogBase::Ok )->setEnabled( m_selectionBox->selectedItem() != 0 );
}


void K3bDataSessionImportDialog::checkNextDevice()
{
  K3bDevice::Device* dev = m_devices.take();
  if( dev && !m_alreadySelected ) {
    m_diskInfoDetector->detect( dev );
  }
  else {
    m_busyWidget->showBusy( false );
    actionButton( KDialogBase::Ok )->setEnabled( true );

    if( m_selectionBox->count() == 0 ) {
      showButton( KDialogBase::Ok, true );
      showButton( KDialogBase::Cancel, false );
      
      m_processLabel->setText( i18n("No session to import found.") );
    }
    else {
      showButton( KDialogBase::Ok, true );
      showButton( KDialogBase::Cancel, true );

      m_processLabel->setText( i18n("Found 1 session to import.",
				    "Found %n sessions to import.",
				    m_selectionBox->count() ) );
    }
  }
}


void K3bDataSessionImportDialog::slotDiskInfoReady( K3bDevice::DiskInfoDetector* did )
{
  // check for an appendable CD (DVD) with a non-empty toc
  // DVD+RW is not reported as appendable
  if( ( did->diskInfo().appendable() ||
	did->diskInfo().mediaType() & (K3bDevice::MEDIA_DVD_PLUS_RW|K3bDevice::MEDIA_DVD_RW_OVWR) ) &&
      !did->toc().isEmpty() &&
      did->toc().last().type() == K3bDevice::Track::DATA ) {

    bool dvd = ( m_doc->type() == K3bDoc::DVD );

    // check if it's the correct media type
    if( ( dvd && did->diskInfo().mediaType() & K3bDevice::MEDIA_WRITABLE_DVD ) ||
	( !dvd && did->diskInfo().mediaType() & K3bDevice::MEDIA_WRITABLE_CD ) ) {

      // add a new session entry
      m_selectionBox->insertItem( QString("%1 - %2 (%3)")
				  .arg(did->iso9660()->primaryDescriptor().volumeId )
				  .arg(KIO::convertSize(did->iso9660()->primaryDescriptor().logicalBlockSize*did->iso9660()->primaryDescriptor().volumeSpaceSize))
				  .arg(did->device()->vendor() + " " + did->device()->description()) );

      // remember the device in a map
      m_deviceMap[m_selectionBox->count()-1] = did->device();
    }
  }

  QTimer::singleShot( 0, this, SLOT(checkNextDevice()) );
}



bool K3bDataSessionImportDialog::importSession( K3bDataDoc* doc, QWidget* parent )
{
  K3bDataSessionImportDialog dlg( parent );
  dlg.importSession( doc );
  dlg.exec();
  return ( dlg.result() == 0 );
}


#include "k3bdatasessionimportdialog.moc"
