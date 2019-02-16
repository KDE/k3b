/*
 *
 * Copyright (C) 2003-2004 Christian Kvasny <chris@k3b.org>
 * Copyright (C) 2009      Arthur Renato Mello <arthur@mandriva.com>
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

#include "k3bvcdview.h"
#include "k3bvcdprojectmodel.h"
#include "k3bvcddoc.h"
#include "k3bvcdburndialog.h"
#include "k3bvcdtrackdialog.h"
#include "k3bfillstatusdisplay.h"
#include "k3bexternalbinmanager.h"
#include "k3bcore.h"
#include "k3baction.h"

#include <KLocalizedString>
#include <KMessageBox>

#include <QDebug>
#include <QItemSelectionModel>
#include <QString>
#include <QAction>
#include <QHeaderView>
#include <QLayout>
#include <QTreeView>

K3b::VcdView::VcdView( K3b::VcdDoc* doc, QWidget* parent )
:
    View( doc, parent ),
    m_doc( doc ),
    m_model( new K3b::VcdProjectModel( m_doc, this ) ),
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
                                            0, this, SLOT(slotProperties()),
                                            actionCollection(), "vcd_show_props" );

    m_actionRemove = K3b::createAction( this, i18n( "Remove" ), "edit-delete",
                                        Qt::Key_Delete, this, SLOT(slotRemove()),
                                        actionCollection(), "vcd_remove_track" );

    connect( m_view, SIGNAL(doubleClicked(QModelIndex)),
             this, SLOT(slotItemActivated(QModelIndex)) );
    connect( m_view->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
             this, SLOT(slotSelectionChanged()) );

    QAction* separator = new QAction( this );
    separator->setSeparator( true );
    m_view->addAction( m_actionRemove );
    m_view->addAction( separator );
    m_view->addAction( m_actionProperties );
    m_view->addAction( separator );
    m_view->addAction( actionCollection()->action("project_burn") );
}


K3b::VcdView::~VcdView()
{
}


K3b::ProjectBurnDialog* K3b::VcdView::newBurnDialog( QWidget * parent)
{
    return new K3b::VcdBurnDialog( m_doc, parent );
}


void K3b::VcdView::init()
{
    if( !k3bcore->externalBinManager()->foundBin( "vcdxbuild" ) ) {
        qDebug() << "(K3b::VcdView) could not find vcdxbuild executable";
        KMessageBox::information( this,
                        i18n( "Could not find VcdImager executable. "
                        "To create Video CDs you have to install VcdImager >= 0.7.12. "
                        "You can find this on your distribution’s software repository or download "
                        "it from http://www.vcdimager.org" ) );
    }
}


void K3b::VcdView::slotSelectionChanged()
{
    const QModelIndexList selected = m_view->selectionModel()->selectedRows();
    if( !selected.isEmpty() ) {
        m_actionRemove->setEnabled( true );
    }
    else {
        m_actionRemove->setEnabled( false );
    }
}


void K3b::VcdView::slotProperties()
{
    const QModelIndexList selection = m_view->selectionModel()->selectedRows();

    if( selection.isEmpty() ) {
        // show project properties
	    View::slotProperties();
    }
    else {
        QList<K3b::VcdTrack*> selected;

        Q_FOREACH( const QModelIndex& index, selection ) {
            selected.append( m_model->trackForIndex(index) );
        }

        QList<K3b::VcdTrack*> tracks = *m_doc->tracks();

        K3b::VcdTrackDialog dlg( m_doc, tracks, selected, this );
        dlg.exec();
    }
}


void K3b::VcdView::slotRemove()
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


void K3b::VcdView::slotItemActivated( const QModelIndex& index )
{
    if( VcdTrack* track = m_model->trackForIndex( index ) ) {
        QList<VcdTrack*> tracks = *m_doc->tracks();
        QList<VcdTrack*> selected;
        selected.append( track );
        K3b::VcdTrackDialog dlg( m_doc, tracks, selected, this );
        dlg.exec();
    }
}


