/* 
 *
 * $Id: sourceheader 380067 2005-01-19 13:03:46Z trueg $
 * Copyright (C) 2005 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
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
#include <k3bapplication.h>
#include <k3biso9660.h>
#include <k3bmediacache.h>
#include <k3b.h>

#include <qpushbutton.h>
#include <qcursor.h>
#include <qapplication.h>

#include <klocale.h>
#include <kmessagebox.h>


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

  if( doc ) {
    m_comboMedia->setWantedMediumType( m_doc->type() == K3bDoc::DVD 
				       ? K3bDevice::MEDIA_WRITABLE_DVD
				       : K3bDevice::MEDIA_WRITABLE_CD );
  }
  else
    m_comboMedia->setWantedMediumType( K3bDevice::MEDIA_WRITABLE );

  m_comboMedia->setWantedMediumState( K3bDevice::STATE_INCOMPLETE );

  slotSelectionChanged( m_comboMedia->selectedDevice() );
}


void K3bDataSessionImportDialog::slotOk()
{
  // find the selected device, show a busy mouse cursor and call K3bDataDoc::importSession
  if( K3bDevice::Device* dev = m_comboMedia->selectedDevice() ) {
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    //
    // Mkisofs does not properly import joliet filenames from an old session
    //
    // See bug 79215 for details
    //
    K3bIso9660 iso( dev );
    if( iso.open() ) {
      if( iso.firstRRDirEntry() == 0 && iso.jolietLevel() > 0 )
	KMessageBox::sorry( this, 
			    i18n("<p>K3b found session containing Joliet information for long filenames "
				 "but no Rock Ridge extensions."
				 "<p>The filenames in the imported session will be converted to a restricted "
				 "character set in the new session. This character set is based on the ISO9660 "
				 "settings in the K3b project. K3b is not able to display these converted filenames yet."), 
			    i18n("Session Import Warning") );
      iso.close();
    }

    if( !m_doc ) {
      if( k3bappcore->mediaCache()->diskInfo( dev ).isDvdMedia() )
	m_doc = static_cast<K3bDataDoc*>( k3bappcore->k3bMainWindow()->slotNewDvdDoc() );
      else
	m_doc = static_cast<K3bDataDoc*>( k3bappcore->k3bMainWindow()->slotNewDataDoc() );
    }

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


K3bDataDoc* K3bDataSessionImportDialog::importSession( K3bDataDoc* doc, QWidget* parent )
{
  K3bDataSessionImportDialog dlg( parent );
  dlg.importSession( doc );
  dlg.exec();
  return dlg.m_doc;
}


#include "k3bdatasessionimportdialog.moc"
