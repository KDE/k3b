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

#include <QSplitter>
#include <QLayout>

K3bStandardView::K3bStandardView(K3bDataDoc* doc, QWidget *parent )
: K3bView(doc, parent)
{
    // --- setup GUI ---------------------------------------------------
    m_splitter = new QSplitter( this );
    m_dirView = new QTreeView(m_splitter);
    m_fileView = new QTreeView(m_splitter);
    m_splitter->setStretchFactor( 0, 1 );
    m_splitter->setStretchFactor( 1, 3 );
    setMainWidget( m_splitter );
}


K3bStandardView::~K3bStandardView()
{
}

void K3bStandardView::setModel(QAbstractItemModel *model)
{
    //TODO: create a proxy model for the dir panel to show only directories
    m_dirView->setModel(model);
    m_fileView->setModel(model);
}

void K3bStandardView::setShowDirPanel(bool show)
{
    m_dirView->setVisible(show);
}

#include "k3bstandardview.moc"
