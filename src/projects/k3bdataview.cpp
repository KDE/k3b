/*
 *
 * $Id$
 * Copyright (C) 2003 Sebastian Trueg <trueg@k3b.org>
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


#include "k3bdataview.h"
#include "k3bdatadoc.h"
#include "k3bdataburndialog.h"
#include "k3bbootimageview.h"
#include "k3bdatadirtreeview.h"
#include "k3bdatafileview.h"
#include "k3bdataurladdingdialog.h"
#include "k3bdatasessionimportdialog.h"
#include <k3bdevice.h>
#include <k3bdeviceselectiondialog.h>
#include <k3bfillstatusdisplay.h>
#include <k3bcore.h>
#include <k3bprojectplugin.h>
#include <k3btoolbox.h>
#include <k3bvalidators.h>

#include <klocale.h>
#include <kurl.h>
#include <kapplication.h>
#include <kpopupmenu.h>
#include <kaction.h>
#include <kmessagebox.h>
#include <kio/global.h>
#include <kio/job.h>
#include <kdialogbase.h>

#include <qpixmap.h>
#include <qsplitter.h>
#include <qlayout.h>
#include <qdragobject.h>
#include <qheader.h>
#include <qptrlist.h>
#include <qlineedit.h>

#include <assert.h>
#include <kdebug.h>


K3bDataView::K3bDataView(K3bDataDoc* doc, QWidget *parent, const char *name )
  : K3bView(doc, parent,name)
{
  m_doc = doc;

  // --- setup GUI ---------------------------------------------------
  QSplitter* mainSplitter = new QSplitter( this );
  m_dataDirTree = new K3bDataDirTreeView( this, doc, mainSplitter );
  m_dataFileView = new K3bDataFileView( this, m_dataDirTree, doc, mainSplitter );
  m_dataDirTree->setFileView( m_dataFileView );
  setMainWidget( mainSplitter );


  connect( m_dataFileView, SIGNAL(dirSelected(K3bDirItem*)), 
	   m_dataDirTree, SLOT(setCurrentDir(K3bDirItem*)) );
  connect( m_doc, SIGNAL(changed()), this, SLOT(slotDocChanged()) );

  m_dataDirTree->checkForNewItems();
  m_dataFileView->checkForNewItems();


  // the data actions
  KAction* actionImportSession = new KAction(i18n("&Import Session..."), "gear", 0, this, SLOT(importSession()),
					     actionCollection(), "project_data_import_session" );
  KAction* actionClearSession = new KAction(i18n("&Clear Imported Session"), "gear", 0, this,
					    SLOT(clearImportedSession()), actionCollection(),
					    "project_data_clear_imported_session" );
  KAction* actionEditBootImages = new KAction(i18n("&Edit Boot Images..."), "cdtrack", 0, this,
					      SLOT(editBootImages()), actionCollection(),
					      "project_data_edit_boot_images" );

  actionImportSession->setToolTip( i18n("Import a previously burned session into the current project") );
  actionClearSession->setToolTip( i18n("Remove the imported items from a previous session") );
  actionEditBootImages->setToolTip( i18n("Modify the bootable settings of the current project") );

  toolBox()->addButton( actionImportSession );
  toolBox()->addButton( actionClearSession );
  toolBox()->addButton( actionEditBootImages );
  toolBox()->addSeparator();

  addPluginButtons( K3bProjectPlugin::DATA_CD );

  toolBox()->addStretch();

  m_volumeIDEdit = new QLineEdit( doc->isoOptions().volumeID(), toolBox() );
  m_volumeIDEdit->setValidator( new K3bLatin1Validator( m_volumeIDEdit ) );
  toolBox()->addLabel( i18n("Volume Name:") );
  toolBox()->addSpacing();
  toolBox()->addWidget( m_volumeIDEdit );
  connect( m_volumeIDEdit, SIGNAL(textChanged(const QString&)), 
	   m_doc,
	   SLOT(setVolumeID(const QString&)) );

  // this is just for testing (or not?)
  // most likely every project type will have it's rc file in the future
  // we only add the additional actions since K3bView already added the default actions
  setXML( "<!DOCTYPE kpartgui SYSTEM \"kpartgui.dtd\">"
	  "<kpartgui name=\"k3bproject\" version=\"1\">"
	  "<MenuBar>"
	  " <Menu name=\"project\"><text>&amp;Project</text>"
	  "  <Action name=\"project_data_import_session\"/>"
	  "  <Action name=\"project_data_clear_imported_session\"/>"
	  "  <Action name=\"project_data_edit_boot_images\"/>"
	  " </Menu>"
	  "</MenuBar>"
	  "</kpartgui>", true );
}


K3bDataView::~K3bDataView(){
}


K3bDirItem* K3bDataView::currentDir() const
{
  return m_dataFileView->currentDir();
}


void K3bDataView::importSession()
{
  K3bDataSessionImportDialog::importSession( m_doc, this );
}


void K3bDataView::clearImportedSession()
{
  m_doc->clearImportedSession();
}


void K3bDataView::editBootImages()
{
  KDialogBase* d = new KDialogBase( this, "", true, i18n("Edit Boot Images"),
				    KDialogBase::Ok, KDialogBase::Ok, true );
  d->setMainWidget( new K3bBootImageView( m_doc, d ) );
  d->exec();
  delete d;
}


K3bProjectBurnDialog* K3bDataView::newBurnDialog( QWidget* parent, const char* name )
{
  return new K3bDataBurnDialog( m_doc, parent, name, true );
}


void K3bDataView::slotBurn()
{
  if( m_doc->burningSize() == 0 ) {
    KMessageBox::information( this, i18n("Please add files to your project first."),
			      i18n("No Data to Burn"), QString::null, false );
  }
  else {
    K3bProjectBurnDialog* dlg = newBurnDialog( this );
    dlg->execBurnDialog(true);
    delete dlg;
  }
}


void K3bDataView::slotDocChanged()
{
  // do not update the editor in case it changed the volume id itself
  if( m_doc->isoOptions().volumeID() != m_volumeIDEdit->text() )
    m_volumeIDEdit->setText( m_doc->isoOptions().volumeID() );
}


void K3bDataView::addUrls( const KURL::List& urls )
{
  K3bDataUrlAddingDialog::addUrls( urls, m_dataFileView->currentDir() );
}

#include "k3bdataview.moc"
