/*
 *
 * Copyright (C) 2003-2008 Sebastian Trueg <trueg@k3b.org>
 *           (C) 2009      Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
 *           (C) 2009      Michal Malek <michalm@jabster.pl>
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
#include "k3bdataburndialog.h"
#include "k3bdatadoc.h"
#include "k3bdataprojectmodel.h"
#include "k3bdataviewimpl.h"
#include "k3bvalidators.h"

#include <KAction>
#include <KActionCollection>
#include <KDebug>
#include <KMessageBox>
#include <KLocale>
#include <KMenu>
#include <KToolBar>
#include <KUrl>

#include <QLabel>
#include <QLayout>
#include <QLineEdit>


K3b::DataView::DataView(K3b::DataDoc* doc, QWidget *parent )
    : K3b::StandardView(doc, parent)
{
    // FIXME: always sort folders first in fileview
    // FIXME: allow sorting by clicking fileview headers

    m_doc = doc;
    m_model = new DataProjectModel(doc, this);
    m_dataViewImpl = new DataViewImpl( this, m_doc, m_model, actionCollection() );

    connect( m_doc, SIGNAL(changed()), this, SLOT(slotDocChanged()) );
    connect( this, SIGNAL(currentRootChanged(QModelIndex)),
             m_dataViewImpl, SLOT(slotCurrentRootChanged(QModelIndex)) );
    connect( this, SIGNAL(activated(QModelIndex)),
             m_dataViewImpl, SLOT(slotItemActivated(QModelIndex)) );
    connect( m_dataViewImpl, SIGNAL(setCurrentRoot(QModelIndex)),
             this, SLOT(setCurrentRoot(QModelIndex)) );
    
    // Connect data actions
    connect( actionCollection()->action( "new_dir" ), SIGNAL( triggered() ),
             this, SLOT(slotNewDir()) );
    connect( actionCollection()->action( "remove" ), SIGNAL( triggered() ),
             this, SLOT(slotRemoveSelectedIndexes()) );
    connect( actionCollection()->action( "rename" ), SIGNAL( triggered() ),
             this, SLOT(slotRenameItem()) );
    connect( actionCollection()->action( "parent_dir" ), SIGNAL( triggered() ),
             this, SLOT(slotParentDir()) );
    connect( actionCollection()->action( "properties" ), SIGNAL( triggered() ),
             this, SLOT(slotItemProperties()) );
    connect( actionCollection()->action( "open" ), SIGNAL( triggered() ),
             this, SLOT(slotOpen()) );

    m_volumeIDEdit = new QLineEdit( doc->isoOptions().volumeID(), toolBox() );
    m_volumeIDEdit->setValidator( new Latin1Validator( m_volumeIDEdit ) );
    
    // Setup toolbar
    toolBox()->addAction( actionCollection()->action( "project_data_import_session" ) );
    toolBox()->addAction( actionCollection()->action( "project_data_clear_imported_session" ) );
    toolBox()->addAction( actionCollection()->action( "project_data_edit_boot_images" ) );
    toolBox()->addSeparator();
    toolBox()->addAction( actionCollection()->action( "parent_dir" ) );
    toolBox()->addSeparator();
    addPluginButtons();
    toolBox()->addWidget( new QLabel( i18n("Volume Name:"), toolBox() ) );
    toolBox()->addWidget( m_volumeIDEdit );
    connect( m_volumeIDEdit, SIGNAL(textChanged(const QString&)),
             m_doc, SLOT(setVolumeID(const QString&)) );
    
    // set the model for the StandardView's views
    setModel(m_model);

    // this is just for testing (or not?)
    // most likely every project type will have it's rc file in the future
    // we only add the additional actions since View already added the default actions
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


K3b::DataView::~DataView()
{
}


void K3b::DataView::importSession()
{
    m_dataViewImpl->slotImportSession();
}


void K3b::DataView::clearImportedSession()
{
    m_dataViewImpl->slotClearImportedSession();
}


void K3b::DataView::editBootImages()
{
    m_dataViewImpl->slotEditBootImages();
}


K3b::ProjectBurnDialog* K3b::DataView::newBurnDialog( QWidget* parent )
{
    return new DataBurnDialog( m_doc, parent );
}


void K3b::DataView::slotBurn()
{
    if( m_doc->burningSize() == 0 ) {
        KMessageBox::information( this, i18n("Please add files to your project first."),
                                  i18n("No Data to Burn"), QString(), false );
    }
    else {
        ProjectBurnDialog* dlg = newBurnDialog( this );
        dlg->execBurnDialog(true);
        delete dlg;
    }
}


void K3b::DataView::slotDocChanged()
{
    // do not update the editor in case it changed the volume id itself
    if( m_doc->isoOptions().volumeID() != m_volumeIDEdit->text() )
        m_volumeIDEdit->setText( m_doc->isoOptions().volumeID() );
}


void K3b::DataView::addUrls( const KUrl::List& urls )
{
    m_dataViewImpl->addUrls( currentRoot(), urls );
}


void K3b::DataView::selectionChanged( const QModelIndexList& indexes )
{
    m_dataViewImpl->slotSelectionChanged( indexes );
}


void K3b::DataView::contextMenu( const QPoint& pos )
{
    m_dataViewImpl->popupMenu()->exec( pos );
}


void K3b::DataView::slotNewDir()
{
    m_dataViewImpl->newDir( currentRoot() );
}


void K3b::DataView::slotItemProperties()
{
    m_dataViewImpl->properties( currentSelection() );
}


void K3b::DataView::slotOpen()
{
    m_dataViewImpl->open( currentSelection() );
}

#include "k3bdataview.moc"
