/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 *           (C) 2009      Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2009 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3bdataview.h"
#include "k3bdatadoc.h"
#include "k3bdataprojectmodel.h"
#include "k3bmodeltypes.h"
#include "k3bdataburndialog.h"
#include "k3bbootimageview.h"
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
    : K3bStandardView(doc, parent)
{
    m_doc = doc;

    m_model = new K3b::DataProjectModel(doc, this);
    // set the model for the K3bStandardView's views
    setModel(m_model);

    connect( m_doc, SIGNAL(changed()), this, SLOT(slotDocChanged()) );

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


K3bDataView::~K3bDataView()
{
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
#if 0
    K3bDataUrlAddingDialog::addUrls( urls, m_dataFileView->currentDir() );
#endif
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

    m_actionRemove = K3b::createAction( this, i18n("Remove"), "edit-delete", Qt::Key_Delete, this, SLOT(slotRemoveSelectedIndexes()),
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

void K3bDataView::contextMenuForSelection(const QModelIndexList &selectedIndexes, const QPoint &pos)
{
    bool open = true, rename = true, remove = true, parent = true;

    // we can only rename one item at a time
    // also, we can only create a new dir over a single directory
    if (selectedIndexes.count() > 1)
    {
        rename = false;
        open = false;
    } 
    else if (selectedIndexes.count() == 1)
    {
        QModelIndex index = selectedIndexes.first();
        rename = (index.flags() & Qt::ItemIsEditable);
        open = (index.data(K3b::ItemTypeRole).toInt() == (int) K3b::FileItem);
    }
    else // selectedIndex.count() == 0
    {
        remove = false;
        rename = false;
        open = false;
    }

    // check if all selected items can be removed
    foreach(QModelIndex index, selectedIndexes)
    {
        if (!index.data(K3b::CustomFlagsRole).toInt() & K3b::ItemIsRemovable)
        {
            remove = false;
            break;
        }
    }

    if (m_model->indexForItem(m_doc->root()) == currentRoot())
        parent = false;

    m_actionRename->setEnabled( rename );
    m_actionRemove->setEnabled( remove );
    m_actionOpen->setEnabled( open );
    m_actionParentDir->setEnabled( parent );

    m_popupMenu->exec(pos);
}

void K3bDataView::slotNewDir()
{
    K3bDirItem *parent = 0;
    QModelIndex index = currentRoot();

    if (index.isValid())
        parent = dynamic_cast<K3bDirItem*>(m_model->itemForIndex(index));

    if (!parent)
        parent = m_doc->root();

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
#if 0
    if ( m_contextMenuOnTreeView ) {
        m_dataDirTree->edit( m_dataDirTree->selectionModel()->selectedRows().first() );
    }
    else {
        m_dataFileView->edit( m_dataFileView->selectionModel()->selectedRows().first() );
    }
#endif
}

void K3bDataView::slotItemProperties()
{
    QModelIndexList selection = currentSelection();

    if ( selection.isEmpty() ) 
    {
        // show project properties
        slotProperties();
    }
    else 
    {
        QList<K3bDataItem*> items;

        foreach(QModelIndex index, selection)
            items.append(m_model->itemForIndex(index));

        K3bDataPropertiesDialog dlg( items, this );
        dlg.exec();
    }
}


void K3bDataView::slotOpen()
{
#if 0
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
#endif
}


void K3bDataView::slotDoubleClicked( const QModelIndex& )
{
#if 0
    m_contextMenuOnTreeView = ( sender() == m_dataDirTree );
    slotItemProperties();
#endif
}

#include "k3bdataview.moc"
