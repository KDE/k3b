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


#include "k3bdatafileview.h"
#include "k3bdataview.h"
#include <k3bdatadoc.h>
#include <k3bdataitem.h>
#include <k3bdiritem.h>
#include <k3bfileitem.h>
#include <k3bspecialdataitem.h>
#include <k3bsessionimportitem.h>
#include <k3bvalidators.h>
#include <k3bview.h>
#include "k3bdataprojectmodel.h"

#include <QList>
#include <QtGui/QHeaderView>

#include <klocale.h>
#include <kdebug.h>


K3bDataFileView::K3bDataFileView( K3bView* view, K3bDataDoc* doc, QWidget* parent )
    : QTreeView( parent ),
      m_view(view)
{
    m_model = new K3b::DataProjectModel( doc, this );
    setModel( m_model );
    setCurrentDir( doc->root() );

    setRootIsDecorated( false );
    setDragDropMode( QAbstractItemView::DragDrop );
    setDropIndicatorShown( true );
    setDragEnabled(true);
    setAcceptDrops( true );
    header()->setVisible( true );
//     setDropVisualizer( false );
//     setDropHighlighter( true );
//     setDragEnabled( true );
//     setItemsMovable( false );
//     setShowSortIndicator( true );

//     setNoItemText( i18n("Use drag'n'drop to add files and directories to the project.\n"
//                         "To remove or rename files use the context menu.\n"
//                         "After that press the burn button to write the CD.") );


//    setSelectionModeExt( K3ListView::Extended );

    m_doc = doc;
}


K3bDataFileView::~K3bDataFileView()
{
}


K3bDataItem* K3bDataFileView::itemAt( const QPoint& pos )
{
    return m_model->itemForIndex( indexAt( pos ) );
}


QList<K3bDataItem*> K3bDataFileView::selectedItems() const
{
    QList<K3bDataItem*> items;
    foreach( const QModelIndex& index, selectionModel()->selectedRows() ) {
        items.append( m_model->itemForIndex( index ) );
    }
    return items;
}


K3bDirItem* K3bDataFileView::currentDir() const
{
    return m_model->itemForIndex( rootIndex() )->getDirItem();
}


void K3bDataFileView::setCurrentDir( K3bDirItem* dir )
{
    setRootIndex( m_model->indexForItem( dir ? dir : m_doc->root() ) );
}

#include "k3bdatafileview.moc"
