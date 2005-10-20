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
#include "k3bmediaselectioncombobox.h"

#include <k3bdatadoc.h>
#include <k3bcore.h>

#include <qpushbutton.h>
#include <qcursor.h>
#include <qapplication.h>

#include <klocale.h>


K3bDataSessionImportDialog::K3bDataSessionImportDialog( QWidget* parent )
  : KDialogBase( parent,
		 "session_import_dialog",
		 true, 
		 i18n("Session Import"), 
		 KDialogBase::Ok|KDialogBase::Cancel,
		 KDialogBase::Ok,
		 false )
{
  m_comboMedia = new K3bMediaSelectionComboBox( this );
  setMainWidget( m_comboMedia );

  connect( m_comboMedia, SIGNAL(selectionChanged(K3bDevice::Device*)),
	   this, SLOT(slotSelectionChanged(K3bDevice::Device*)) );
}


K3bDataSessionImportDialog::~K3bDataSessionImportDialog()
{
}


void K3bDataSessionImportDialog::importSession( K3bDataDoc* doc )
{
  m_doc = doc;

  m_comboMedia->setWantedMediumType( m_doc->type() == K3bDoc::DVD 
				     ? K3bDevice::MEDIA_WRITABLE_DVD
				     : K3bDevice::MEDIA_WRITABLE_CD );
  m_comboMedia->setWantedMediumState( K3bDevice::STATE_INCOMPLETE );

  slotSelectionChanged( m_comboMedia->selectedDevice() );
}


void K3bDataSessionImportDialog::slotOk()
{
  // find the selected device, show a busy mouse cursor and call K3bDataDoc::importSession
  if( K3bDevice::Device* dev = m_comboMedia->selectedDevice() ) {
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


void K3bDataSessionImportDialog::slotSelectionChanged( K3bDevice::Device* dev )
{      
  actionButton( KDialogBase::Ok )->setEnabled( dev != 0 );
}


bool K3bDataSessionImportDialog::importSession( K3bDataDoc* doc, QWidget* parent )
{
  K3bDataSessionImportDialog dlg( parent );
  dlg.importSession( doc );
  dlg.exec();
  return ( dlg.result() == 0 );
}


#include "k3bdatasessionimportdialog.moc"
