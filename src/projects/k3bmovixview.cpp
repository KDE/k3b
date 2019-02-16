/*
 *
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
 *           (C) 2009      Arthur Renato Mello <arthur@mandriva.com>
 *           (C) 2009      Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
 *           (C) 2009-2010 Michal Malek <michalm@jabster.pl>
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


#include "k3bmovixview.h"
#include "k3bmovixprojectmodel.h"
#include "k3bmovixdoc.h"
#include "k3bmovixburndialog.h"
#include "k3bmovixfileitem.h"

#include "k3bdatapropertiesdialog.h"
#include "k3baction.h"
#include "k3bvolumenamewidget.h"

#include <KLocalizedString>
#include <KMessageBox>
#include <KToolBar>

#include <QDebug>
#include <QItemSelectionModel>
#include <QList>
#include <QUrl>
#include <QAction>
#include <QFileDialog>
#include <QHeaderView>
#include <QTreeView>

K3b::MovixView::MovixView( K3b::MovixDoc* doc, QWidget* parent )
:
    View( doc, parent ),
    m_doc( doc ),
    m_model( new MovixProjectModel( m_doc, this ) ),
    m_view( new QTreeView( this ) )
{
    m_view->setModel( m_model );
    m_view->setAcceptDrops( true );
    m_view->setDragEnabled( true );
    m_view->setDragDropMode( QTreeView::DragDrop );
    m_view->setItemsExpandable( false );
    m_view->setRootIsDecorated( false );
    m_view->setSelectionMode( QTreeView::ExtendedSelection );
    m_view->setVerticalScrollMode( QAbstractItemView::ScrollPerPixel );
    m_view->setContextMenuPolicy( Qt::ActionsContextMenu );
    // FIXME: make QHeaderView::Interactive the default but connect to model changes and call header()->resizeSections( QHeaderView::ResizeToContents );
    m_view->header()->setSectionResizeMode( QHeaderView::ResizeToContents );
    m_view->setEditTriggers( QAbstractItemView::NoEditTriggers );
    setMainWidget( m_view );

    // setup actions
    m_actionProperties = K3b::createAction( this, i18n("Properties"), "document-properties",
                                            0, this, SLOT(showPropertiesDialog()),
                                            actionCollection(), "movix_show_props" );
    m_actionRemove = K3b::createAction( this, i18n( "Remove" ), "edit-delete",
                                        Qt::Key_Delete, this, SLOT(slotRemove()),
                                        actionCollection(), "movix_remove_item" );
    m_actionRemoveSubTitle = K3b::createAction( this, i18n( "Remove Subtitle File" ), "edit-delete",
                                                0, this, SLOT(slotRemoveSubTitleItems()),
                                                actionCollection(), "movix_remove_subtitle_item" );
    m_actionAddSubTitle = K3b::createAction( this, i18n("Add Subtitle File..."), 0,
                                             0, this, SLOT(slotAddSubTitleFile()),
                                             actionCollection(), "movix_add_subtitle" );

    connect( m_view->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
             this, SLOT(slotSelectionChanged()) );

    QAction* separator = new QAction( this );
    separator->setSeparator( true );
    m_view->addAction( m_actionRemove );
    m_view->addAction( m_actionRemoveSubTitle );
    m_view->addAction( m_actionAddSubTitle );
    m_view->addAction( separator );
    m_view->addAction( m_actionProperties );
    m_view->addAction( separator );
    m_view->addAction( actionCollection()->action("project_burn") );

    // Setup toolbar
    toolBox()->addActions( createPluginsActions( m_doc->type() ) );
    toolBox()->addWidget( new VolumeNameWidget( doc, toolBox() ) );
}


K3b::MovixView::~MovixView()
{
}


void K3b::MovixView::showPropertiesDialog()
{
    const QModelIndexList selection = m_view->selectionModel()->selectedRows();

    if( selection.isEmpty() ) {
        // show project properties
        slotProperties();
    }
    else {
        QList<K3b::DataItem*> items;

        foreach(const QModelIndex &index, selection)
            items.append(m_model->itemForIndex(index));

        K3b::DataPropertiesDialog dlg( items, this );
        dlg.exec();
    }
}


void K3b::MovixView::slotRemoveSubTitleItems()
{
    const QModelIndexList selection = m_view->selectionModel()->selectedRows();
    if ( !selection.count() )
        return;

    K3b::MovixFileItem *item = 0;
    Q_FOREACH( const QModelIndex& index, selection ) {
        item = m_model->itemForIndex(index);
        if (item)
            m_doc->removeSubTitleItem( item );
    }
}


void K3b::MovixView::slotAddSubTitleFile()
{
    const QModelIndexList selection = m_view->selectionModel()->selectedRows();
    if( !selection.count() )
        return;

    MovixFileItem *item = 0;
    foreach( const QModelIndex& index, selection )
    {
        item = m_model->itemForIndex(index);
        if( item )
            break;
    }

    if( item ) {
        QUrl url = QFileDialog::getOpenFileUrl( this );
        if( url.isValid() ) {
            if( url.isLocalFile() )
                m_doc->addSubTitleItem( item, url );
            else
                KMessageBox::error( 0, i18n("K3b currently only supports local files.") );
        }
    }
}


void K3b::MovixView::slotSelectionChanged()
{
    const QModelIndexList selection = m_view->selectionModel()->selectedRows();
    if( selection.count() >= 1 ) {
        m_actionRemove->setEnabled(true);

        bool subtitle = false;
        // check if any of the items have a subtitle
        Q_FOREACH( const QModelIndex& index, selection ) {
            K3b::MovixFileItem *item = m_model->itemForIndex(index);
            if (item && item->subTitleItem()) {
                subtitle = true;
                break;
            }
        }
        m_actionRemoveSubTitle->setEnabled( subtitle );
        // only enable the subtitle adding if there is just one item selected
        m_actionAddSubTitle->setEnabled( selection.count() == 1 );
    }
    else {
        m_actionRemove->setEnabled(false);
        m_actionRemoveSubTitle->setEnabled( false );
        m_actionAddSubTitle->setEnabled( false );
    }
}


void K3b::MovixView::slotRemove()
{
    const QModelIndexList selected = m_view->selectionModel()->selectedRows();

    // create a list of persistent model indexes to be able to remove all of them
    QList<QPersistentModelIndex> indexes;
    Q_FOREACH( const QModelIndex& index, selected ) {
        indexes.append( QPersistentModelIndex( index ) );
    }

    // and now ask the indexes to be removed
    Q_FOREACH( const QPersistentModelIndex& index, indexes ) {
        m_model->removeRow( index.row(), index.parent() );
    }
}


K3b::ProjectBurnDialog* K3b::MovixView::newBurnDialog( QWidget* parent )
{
    return new K3b::MovixBurnDialog( m_doc, parent );
}


