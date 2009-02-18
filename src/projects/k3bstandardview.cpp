/*
 *
 * Copyright (C) 2009       Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
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

#include "k3bstandardview.h"
#include "k3bdirproxymodel.h"

#include <QAbstractItemModel>
#include <QTreeView>
#include <QSplitter>
#include <QLayout>
#include <QDebug>
#include <QHeaderView>

K3bStandardView::K3bStandardView(K3bDoc* doc, QWidget *parent )
: K3bView(doc, parent)
{
    m_dirProxy = new K3bDirProxyModel(this);

    // --- setup GUI ---------------------------------------------------
    m_splitter = new QSplitter( this );
    m_dirView = new QTreeView(m_splitter);
    m_fileView = new QTreeView(m_splitter);
    m_splitter->setStretchFactor( 0, 1 );
    m_splitter->setStretchFactor( 1, 3 );
    setMainWidget( m_splitter );

    // --- setup Views ---------------------------------------------------
    // Dir panel
    m_dirView->setHeaderHidden(true);
    m_dirView->setAcceptDrops(true);
    m_dirView->setDragEnabled(true);
    m_dirView->setDragDropMode(QTreeView::DragDrop);
    m_dirView->setSelectionMode(QTreeView::SingleSelection);
    m_dirView->setModel(m_dirProxy);
    m_dirView->setContextMenuPolicy(Qt::CustomContextMenu);

    // File panel
    m_fileView->setAcceptDrops(true);
    m_fileView->setDragEnabled(true);
    m_fileView->setDragDropMode(QTreeView::DragDrop);
    m_fileView->setRootIsDecorated(false);
    m_fileView->setSelectionMode(QTreeView::ExtendedSelection);
    m_fileView->setContextMenuPolicy(Qt::CustomContextMenu);

    // connect signals/slots
    connect(m_dirView, SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(slotCustomContextMenu(const QPoint&)));
    connect(m_fileView, SIGNAL(customContextMenuRequested(const QPoint&)),
            this, SLOT(slotCustomContextMenu(const QPoint&)));
}

K3bStandardView::~K3bStandardView()
{
}

void K3bStandardView::setModel(QAbstractItemModel *model)
{
    m_dirProxy->setSourceModel(model);

    // hide the columns in the dir view
    for (int i = 1; i < model->columnCount(); ++i)
        m_dirView->setColumnHidden(i, true);

    m_fileView->setModel(model);

    // connect signals/slots
    // this signal is better to get connected before the setCurrentIndex is called,
    // so that it updates the file view
    connect(m_dirView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection&)),
            this, SLOT(slotCurrentDirChanged()));

    // select the first item from the model
    if (m_dirProxy->rowCount() > 0)
        m_dirView->setCurrentIndex(m_dirProxy->index(0,0));
}

void K3bStandardView::setShowDirPanel(bool show)
{
    m_dirView->setVisible(show);
    if (!show)
        m_fileView->setRootIndex(QModelIndex());
}

void K3bStandardView::contextMenuForSelection(const QModelIndexList &selectedIndexes, const QPoint &pos)
{
    // do nothing in the default implementation (at least for now)
    qDebug() << "Gotta show a menu for " << selectedIndexes.count() << " items at " << pos;
}

QModelIndexList K3bStandardView::currentSelection() const
{
    return m_currentSelection;
}

QModelIndex K3bStandardView::currentRoot() const
{
    return m_fileView->rootIndex();
}

void K3bStandardView::slotCurrentDirChanged()
{
    QModelIndexList indexes = m_dirView->selectionModel()->selectedRows();

    QModelIndex currentDir;
    if (indexes.count())
        currentDir = m_dirProxy->mapToSource(indexes.first());

    // make the file view show only the child nodes of the currently selected
    // directory from dir view
    m_fileView->setRootIndex(currentDir);
    m_fileView->header()->resizeSections( QHeaderView::Stretch );

    emit currentRootChanged( currentDir );
}

void K3bStandardView::slotCustomContextMenu(const QPoint &pos)
{
    QModelIndexList selection;
    // detect which view emitted the signal
    QTreeView *view = dynamic_cast<QTreeView*>(sender());

    // this should not happen, but just in case...
    if (!view)
        return;

    // if the signal was emitted by the dirview, we need to map the indexes to the
    // source model
    if (view == m_dirView)
    {
        foreach(QModelIndex index, view->selectionModel()->selectedRows())
            selection.append( m_dirProxy->mapToSource(index) );
    }
    else
    {
        selection = view->selectionModel()->selectedRows();
    }

    m_currentSelection = selection;

    // call the context menu method so that derived classes can place customized
    // context menus
    contextMenuForSelection(selection, view->viewport()->mapToGlobal(pos));
}

void K3bStandardView::slotParentDir()
{
    m_dirView->setCurrentIndex(m_dirProxy->mapFromSource(m_fileView->rootIndex().parent()));
}

void K3bStandardView::slotRemoveSelectedIndexes()
{
    QAbstractItemModel *model = m_fileView->model();
    if (!model)
        return;

    // create a list of persistent model indexes to be able to remove all of them
    QList<QPersistentModelIndex> indexes;
    foreach(QModelIndex index, m_currentSelection)
        indexes.append( QPersistentModelIndex(index) );


    // and now ask the indexes to be removed
    foreach(QPersistentModelIndex index, indexes)
        model->removeRow(index.row(), index.parent());

    // clear the selection, just to make sure
    m_currentSelection.clear();
}

void K3bStandardView::slotRenameItem()
{
    if (m_currentSelection.isEmpty())
        return;

    if (m_dirView->hasFocus())
        m_dirView->edit( m_dirProxy->mapFromSource(m_currentSelection.first()) );
    else
        m_fileView->edit( m_currentSelection.first() );
}
#include "k3bstandardview.moc"
