/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2008 Sebastian Trueg <trueg@k3b.org>
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
#include "k3bdatamultisessionimportdialog.h"
#include "k3bdatapropertiesdialog.h"
#include <k3bdiritem.h>
#include <k3bdevice.h>
#include <k3bdeviceselectiondialog.h>
#include <k3bfillstatusdisplay.h>
#include <k3bcore.h>
#include <k3bprojectplugin.h>
#include <k3bvalidators.h>
#include <k3baction.h>

#include <klocale.h>
#include <kurl.h>
#include <kapplication.h>
#include <kmenu.h>
#include <kaction.h>
#include <kmessagebox.h>
#include <kio/global.h>
#include <kio/job.h>
#include <kdialog.h>
#include <ktoolbar.h>
#include <kinputdialog.h>

#include <qpixmap.h>
#include <qsplitter.h>
#include <qlayout.h>
#include <q3dragobject.h>
#include <q3header.h>
#include <qlist.h>
#include <qlineedit.h>
#include <QtCore/QModelIndex>

#include <assert.h>
#include <kdebug.h>
#include <KRun>


K3bDataView::K3bDataView(K3bDataDoc* doc, QWidget *parent )
    : K3bView(doc, parent)
{
    m_doc = doc;

    // --- setup GUI ---------------------------------------------------
    QSplitter* mainSplitter = new QSplitter( this );
    m_dataDirTree = new K3bDataDirTreeView( this, doc, mainSplitter );
    m_dataFileView = new K3bDataFileView( this, doc, mainSplitter );
    setMainWidget( mainSplitter );


    connect( m_dataFileView, SIGNAL(dirSelected(K3bDirItem*)),
             this, SLOT(setCurrentDir(K3bDirItem*)) );
    connect( m_dataDirTree, SIGNAL(dirSelected(K3bDirItem*)),
             this, SLOT(setCurrentDir(K3bDirItem*)) );
    connect( m_doc, SIGNAL(changed()), this, SLOT(slotDocChanged()) );

    m_dataDirTree->setContextMenuPolicy( Qt::CustomContextMenu );
    m_dataFileView->setContextMenuPolicy( Qt::CustomContextMenu );

    connect( m_dataDirTree, SIGNAL(customContextMenuRequested(const QPoint&)),
             this, SLOT(showPopupMenu(const QPoint&)) );
    connect( m_dataFileView, SIGNAL(customContextMenuRequested(const QPoint&)),
             this, SLOT(showPopupMenu(const QPoint&)) );

    setupContextMenu();

    // the data actions
    KAction* actionImportSession = K3b::createAction( this, i18n("&Import Session..."), "document-import", 0, this, SLOT(importSession()),
                                                      actionCollection(), "project_data_import_session" );
    KAction* actionClearSession = K3b::createAction( this, i18n("&Clear Imported Session"), "edit-clear", 0, this,
                                                     SLOT(clearImportedSession()), actionCollection(),
                                                     "project_data_clear_imported_session" );
    KAction* actionEditBootImages = K3b::createAction( this, i18n("&Edit Boot Images..."), "document-properties", 0, this,
                                                       SLOT(editBootImages()), actionCollection(),
                                                       "project_data_edit_boot_images" );

    actionImportSession->setToolTip( i18n("Import a previously burned session into the current project") );
    actionClearSession->setToolTip( i18n("Remove the imported items from a previous session") );
    actionEditBootImages->setToolTip( i18n("Modify the bootable settings of the current project") );

    toolBox()->addAction( actionImportSession );
    toolBox()->addAction( actionClearSession );
    toolBox()->addAction( actionEditBootImages );
    toolBox()->addSeparator();
    toolBox()->addAction( actionCollection()->action("parent_dir") );
    toolBox()->addSeparator();

    addPluginButtons( K3bProjectPlugin::DATA_CD );

    m_volumeIDEdit = new QLineEdit( doc->isoOptions().volumeID(), toolBox() );
    m_volumeIDEdit->setValidator( new K3bLatin1Validator( m_volumeIDEdit ) );
    toolBox()->addWidget( new QLabel( i18n("Volume Name:"), toolBox() ) );
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


void K3bDataView::setCurrentDir( K3bDirItem* dir )
{
    m_dataFileView->setCurrentDir( dir );
    m_dataDirTree->setCurrentDir( dir );
    m_actionParentDir->setEnabled( currentDir() != m_doc->root() );
}


void K3bDataView::importSession()
{
    K3bDataMultisessionImportDialog::importSession( m_doc, this );
}


void K3bDataView::clearImportedSession()
{
    m_doc->clearImportedSession();
}


void K3bDataView::editBootImages()
{
    KDialog dlg( this );
    dlg.setCaption( i18n("Edit Boot Images") );
    dlg.setButtons( KDialog::Ok );
    dlg.setDefaultButton( KDialog::Ok );
    dlg.setMainWidget( new K3bBootImageView( m_doc, &dlg ) );
    dlg.exec();
}


K3bProjectBurnDialog* K3bDataView::newBurnDialog( QWidget* parent )
{
    return new K3bDataBurnDialog( m_doc, parent );
}


void K3bDataView::slotBurn()
{
    if( m_doc->burningSize() == 0 ) {
        KMessageBox::information( this, i18n("Please add files to your project first."),
                                  i18n("No Data to Burn"), QString(), false );
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


void K3bDataView::addUrls( const KUrl::List& urls )
{
    K3bDataUrlAddingDialog::addUrls( urls, m_dataFileView->currentDir() );
}


void K3bDataView::setupContextMenu()
{
    m_actionProperties = new KAction( this );
    m_actionProperties->setText( i18n("Properties") );
    m_actionProperties->setIcon( KIcon( "document-properties" ) );
    connect( m_actionProperties, SIGNAL( triggered() ), this, SLOT(slotItemProperties()) );
    actionCollection()->addAction( "properties", m_actionProperties );

    m_actionNewDir = new KAction( this );
    m_actionNewDir->setText( i18n("New Directory...") );
    m_actionNewDir->setIcon( KIcon( "folder-new" ) );
    m_actionNewDir->setShortcut( Qt::CTRL+Qt::Key_N );
    connect( m_actionNewDir, SIGNAL( triggered() ), this, SLOT(slotNewDir()) );
    actionCollection()->addAction( "new_dir", m_actionNewDir );

    m_actionRemove = K3b::createAction( this, i18n("Remove"), "edit-delete", Qt::Key_Delete, this, SLOT(slotRemoveItem()),
                                        actionCollection(), "remove" );
    m_actionRename = K3b::createAction( this, i18n("Rename"), "edit-rename", Qt::Key_F2, this, SLOT(slotRenameItem()),
                                        actionCollection(), "rename" );
    m_actionParentDir = K3b::createAction( this, i18n("Parent Directory"), "go-up", 0, this, SLOT(slotParentDir()),
                                           actionCollection(), "parent_dir" );
    m_actionOpen = K3b::createAction( this, i18n("Open"), "document-open", 0, this, SLOT(slotOpen()),
                                      actionCollection(), "open" );

    m_popupMenu = new KMenu( this );
    m_popupMenu->addAction( m_actionParentDir );
    m_popupMenu->addSeparator();
    m_popupMenu->addAction( m_actionRename );
    m_popupMenu->addAction( m_actionRemove );
    m_popupMenu->addAction( m_actionNewDir );
    m_popupMenu->addSeparator();
    m_popupMenu->addAction( m_actionOpen );
    m_popupMenu->addSeparator();
    m_popupMenu->addAction( m_actionProperties );
    m_popupMenu->addSeparator();
    m_popupMenu->addAction( actionCollection()->action("project_burn") );
}


void K3bDataView::showPopupMenu( const QPoint& point )
{
    m_contextMenuOnTreeView = ( sender() == m_dataDirTree );

    QPoint globalPoint;
    K3bDataItem* item = 0;
    if ( m_contextMenuOnTreeView ) {
        globalPoint = m_dataDirTree->mapToGlobal( point );
        item = m_dataDirTree->selectedDir();
    }
    else if ( m_dataFileView->selectedItems().count() ) {
        globalPoint = m_dataFileView->mapToGlobal( point );
        item = m_dataFileView->selectedItems().first();
    }

    if( item ) {
        m_actionRemove->setEnabled( item->isRemoveable() );
        m_actionRename->setEnabled( item->isRenameable() );
        m_actionOpen->setEnabled( item->isFile() );
    }
    else {
        m_actionRemove->setEnabled( false );
        m_actionRename->setEnabled( false );
        m_actionOpen->setEnabled( false );
    }

    m_popupMenu->popup( globalPoint );
}


void K3bDataView::slotNewDir()
{
    K3bDirItem* parent = currentDir();

    QString name;
    bool ok;

    name = KInputDialog::getText( i18n("New Directory"),
                                  i18n("Please insert the name for the new directory:"),
                                  i18n("New Directory"), &ok, this );

    while( ok && K3bDataDoc::nameAlreadyInDir( name, parent ) ) {
        name = KInputDialog::getText( i18n("New Directory"),
                                      i18n("A file with that name already exists. "
                                           "Please insert the name for the new directory:"),
                                      i18n("New Directory"), &ok, this );
    }

    if( !ok )
        return;


    m_doc->addEmptyDir( name, parent );
}


void K3bDataView::slotRenameItem()
{
    if ( m_contextMenuOnTreeView ) {
        m_dataDirTree->edit( m_dataDirTree->selectionModel()->selectedRows().first() );
    }
    else {
        m_dataFileView->edit( m_dataFileView->selectionModel()->selectedRows().first() );
    }
}


void K3bDataView::slotRemoveItem()
{
    if ( m_contextMenuOnTreeView ) {
        if ( m_dataDirTree->selectedDir() )
            m_doc->removeItem( m_dataDirTree->selectedDir() );
    }
    else {
        foreach( K3bDataItem* item, m_dataFileView->selectedItems() ) {
            m_doc->removeItem( item );
        }
    }
}


void K3bDataView::slotParentDir()
{
    if( m_dataFileView->currentDir() != m_doc->root() ) {
        setCurrentDir( currentDir()->parent() );
    }
}


void K3bDataView::slotItemProperties()
{
    QList<K3bDataItem*> items = selectedItems();
    if ( items.isEmpty() ) {
        // show project properties
        slotProperties();
    }
    else {
        K3bDataPropertiesDialog dlg( items, this );
        dlg.exec();
    }
}


void K3bDataView::slotOpen()
{
    QList<K3bDataItem*> items = selectedItems();
    if( !items.isEmpty() && items.first()->isFile() ) {
        K3bDataItem* item = items.first();
        KUrl url = item->localPath();
        if( !KRun::isExecutableFile( url,
                                     item->mimeType()->name() ) ) {
            KRun::runUrl( url,
                          item->mimeType()->name(),
                          this );
        }
        else {
            KRun::displayOpenWithDialog( KUrl::List() << url, false, this );
        }
    }
}


void K3bDataView::slotDoubleClicked( const QModelIndex& )
{
    m_contextMenuOnTreeView = ( sender() == m_dataDirTree );
    slotItemProperties();
}


QList<K3bDataItem*> K3bDataView::selectedItems() const
{
    QList<K3bDataItem*> items;
    if ( m_contextMenuOnTreeView ) {
        if( m_dataDirTree->selectedDir() ) {
           items.append( m_dataDirTree->selectedDir() );
       }
    }
    else {
        items = m_dataFileView->selectedItems();
    }
    return items;
}

#include "k3bdataview.moc"
