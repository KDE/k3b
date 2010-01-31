/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 * Copyright (C) 2009      Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
 * Copyright (C) 2009-2010 Michal Malek <michalm@jabster.pl>
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

#include "k3bdataviewimpl.h"
#include "k3baction.h"
#include "k3bbootimageview.h"
#include "k3bdatadoc.h"
#include "k3bdatamultisessionimportdialog.h"
#include "k3bdataprojectmodel.h"
#include "k3bdatapropertiesdialog.h"
#include "k3bdataurladdingdialog.h"
#include "k3bdiritem.h"
#include "k3bmodeltypes.h"
#include "k3bview.h"

#include <KAction>
#include <KActionCollection>
#include <KInputDialog>
#include <KLocale>
#include <KMenu>
#include <KRun>


K3b::DataViewImpl::DataViewImpl( View* view, DataDoc* doc, DataProjectModel* model, KActionCollection* actionCollection )
    : QObject( view ), m_view( view ), m_doc( doc ), m_model( model )
{
    m_actionNewDir = createAction( m_view, i18n("New Folder..."), "folder-new", Qt::CTRL+Qt::Key_N, 0, 0,
                                   actionCollection, "new_dir" );
    m_actionRemove = createAction( m_view, i18n("Remove"), "edit-delete", Qt::Key_Delete, 0, 0,
                                   actionCollection, "remove" );
    m_actionRename = createAction( m_view, i18n("Rename"), "edit-rename", Qt::Key_F2, 0, 0,
                                   actionCollection, "rename" );
    m_actionParentDir = createAction( m_view, i18n("Parent Folder"), "go-up", 0, 0, 0,
                                      actionCollection, "parent_dir" );
    m_actionProperties = createAction( m_view, i18n("Properties"), "document-properties", 0, 0, 0,
                                       actionCollection, "properties" );
    m_actionOpen = createAction( m_view, i18n("Open"), "document-open", 0, 0, 0,
                                 actionCollection, "open" );
    m_actionImportSession = createAction( m_view, i18n("&Import Session..."), "document-import", 0, this, SLOT(slotImportSession()),
                                          actionCollection, "project_data_import_session" );
    m_actionClearSession = createAction( m_view, i18n("&Clear Imported Session"), "edit-clear", 0, this, SLOT(slotClearImportedSession()),
                                         actionCollection, "project_data_clear_imported_session" );
    m_actionEditBootImages = createAction( m_view, i18n("&Edit Boot Images..."), "document-properties", 0, this, SLOT(slotEditBootImages()),
                                           actionCollection, "project_data_edit_boot_images" );
    
    m_actionImportSession->setToolTip( i18n("Import a previously burned session into the current project") );
    m_actionClearSession->setToolTip( i18n("Remove the imported items from a previous session") );
    m_actionEditBootImages->setToolTip( i18n("Modify the bootable settings of the current project") );
    
    m_actionClearSession->setEnabled( m_doc->importedSession() > -1 );
    connect( m_doc, SIGNAL(importedSessionChanged(int)), this, SLOT(slotImportedSessionChanged(int)) );
    
    // Create data context menu
    m_popupMenu = new KMenu( m_view );
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
    m_popupMenu->addAction( actionCollection->action("project_burn") );
}


void K3b::DataViewImpl::addUrls( const QModelIndex& parent, const KUrl::List& urls )
{
    DirItem *item = dynamic_cast<DirItem*>(m_model->itemForIndex(parent));
    if (!item)
        item = m_doc->root();

    DataUrlAddingDialog::addUrls( urls, item, m_view );
}


void K3b::DataViewImpl::newDir( const QModelIndex& parent )
{
    DirItem* parentDir = 0;

    if (parent.isValid())
        parentDir = dynamic_cast<DirItem*>(m_model->itemForIndex(parent));

    if (!parentDir)
        parentDir = m_doc->root();

    QString name;
    bool ok;

    name = KInputDialog::getText( i18n("New Folder"),
                                i18n("Please insert the name for the new folder:"),
                                i18n("New Folder"), &ok, m_view );

    while( ok && DataDoc::nameAlreadyInDir( name, parentDir ) ) {
        name = KInputDialog::getText( i18n("New Folder"),
                                    i18n("A file with that name already exists. "
                                        "Please insert the name for the new folder:"),
                                    i18n("New Folder"), &ok, m_view );
    }

    if( !ok )
        return;

    m_doc->addEmptyDir( name, parentDir );
}


void K3b::DataViewImpl::properties( const QModelIndexList& indexes )
{
    if ( indexes.isEmpty() )
    {
        // show project properties
        m_view->slotProperties();
    }
    else
    {
        QList<DataItem*> items;

        foreach(const QModelIndex& index, indexes) {
            items.append( m_model->itemForIndex(index) );
        }

        DataPropertiesDialog dlg( items, m_view );
        dlg.exec();
    }
}


void K3b::DataViewImpl::open( const QModelIndexList& indexes )
{
    if (indexes.isEmpty())
        return;

    DataItem* item = m_model->itemForIndex( indexes.first() );

    if( !item->isFile() ) {
        KUrl url = item->localPath();
        if( !KRun::isExecutableFile( url,
                                    item->mimeType()->name() ) ) {
            KRun::runUrl( url,
                        item->mimeType()->name(),
                        m_view );
        }
        else {
            KRun::displayOpenWithDialog( KUrl::List() << url, false, m_view );
        }
    }
}


void K3b::DataViewImpl::slotCurrentRootChanged( const QModelIndex& newRoot )
{
    m_actionParentDir->setEnabled( newRoot.isValid() && m_model->indexForItem(m_doc->root()) != newRoot );
}


void K3b::DataViewImpl::slotSelectionChanged( const QModelIndexList& indexes )
{
    bool open = true, rename = true, remove = true;

    // we can only rename one item at a time
    // also, we can only create a new dir over a single directory
    if (indexes.count() > 1)
    {
        rename = false;
        open = false;
    }
    else if (indexes.count() == 1)
    {
        QModelIndex index = indexes.first();
        rename = (index.flags() & Qt::ItemIsEditable);
        open = (index.data(ItemTypeRole).toInt() == (int) FileItemType);
    }
    else // selectedIndex.count() == 0
    {
        remove = false;
        rename = false;
        open = false;
    }

    // check if all selected items can be removed
    foreach(const QModelIndex &index, indexes)
    {
        if (!index.data(CustomFlagsRole).toInt() & ItemIsRemovable)
        {
            remove = false;
            break;
        }
    }

    m_actionRename->setEnabled( rename );
    m_actionRemove->setEnabled( remove );
    m_actionOpen->setEnabled( open );
}

void K3b::DataViewImpl::slotItemActivated( const QModelIndex& index )
{
    if( index.isValid() ) {
        int type = index.data(ItemTypeRole).toInt();
        if( type == (int)DirItemType ) {
            emit setCurrentRoot( index );
        }
        else if( type == (int)FileItemType ) {
            properties( QModelIndexList() << index );
        }
    }
}


void K3b::DataViewImpl::slotImportSession()
{
    K3b::DataMultisessionImportDialog::importSession( m_doc, m_view );
}


void K3b::DataViewImpl::slotClearImportedSession()
{
    m_doc->clearImportedSession();
}


void K3b::DataViewImpl::slotEditBootImages()
{
    KDialog dlg( m_view );
    dlg.setCaption( i18n("Edit Boot Images") );
    dlg.setButtons( KDialog::Ok );
    dlg.setDefaultButton( KDialog::Ok );
    dlg.setMainWidget( new K3b::BootImageView( m_doc, &dlg ) );
    dlg.exec();
}


void K3b::DataViewImpl::slotImportedSessionChanged( int importedSession )
{
    m_actionClearSession->setEnabled( importedSession > -1 );
}

#include "k3bdataviewimpl.moc"
