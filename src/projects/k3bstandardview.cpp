/*
 *
 * Copyright (C) 200       Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
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

    // File panel
    m_fileView->setAcceptDrops(true);
    m_fileView->setDragEnabled(true);
    m_fileView->setDragDropMode(QTreeView::DragDrop);
    m_fileView->setRootIsDecorated(false);

}


K3bStandardView::~K3bStandardView()
{
}

void K3bStandardView::setModel(QAbstractItemModel *model)
{
    //TODO: create a proxy model for the dir panel to show only directories
    m_dirView->setModel(model);

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
    if (model->rowCount() > 0)
        m_dirView->setCurrentIndex(model->index(0,0));
}

void K3bStandardView::setShowDirPanel(bool show)
{
    m_dirView->setVisible(show);
    if (!show)
        m_fileView->setRootIndex(QModelIndex());
}

void K3bStandardView::slotCurrentDirChanged()
{
    QModelIndexList indexes = m_dirView->selectionModel()->selectedRows();

    QModelIndex currentDir;
    if (indexes.count())
        currentDir = indexes.first();

    // make the file view show only the child nodes of the currently selected
    // directory from dir view
    m_fileView->setRootIndex(currentDir);
}

#include "k3bstandardview.moc"
