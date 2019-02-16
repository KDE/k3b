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


#include "k3bdatadirtreeview.h"
#include "k3bdataview.h"
#include "k3bdatadoc.h"
#include "k3bdataitem.h"
#include "k3bdiritem.h"
#include "k3bdataprojectmodel.h"
#include "k3bdataurladdingdialog.h"
#include "k3bview.h"
#include "k3bvalidators.h"

#include <KLocalizedString>
#include <KIconLoader>

#include <QDebug>
#include <QList>
#include <QHeaderView>


class K3b::DataDirTreeView::Private
{
public:
    Private() {
    }

    int animationCounter;
    QPixmap beforeAniPixmap;

#ifdef __GNUC__
#warning Re-enable the iso and ascii validators
#endif
    QValidator* iso9660Validator;
    QValidator* asciiValidator;

    K3b::DataProjectModel* model;
};


K3b::DataDirTreeView::DataDirTreeView( K3b::View* view, K3b::DataDoc* doc, QWidget* parent )
    : QTreeView( parent ), m_view(view)
{
    d = new Private();

    setSelectionMode(QAbstractItemView::SingleSelection);

//    setRootIsDecorated( false );
    setDragDropMode( QAbstractItemView::DragDrop );
    setDropIndicatorShown( true );
    setDragEnabled(true);
    setAcceptDrops( true );

    header()->hide();

    m_doc = doc;

    d->model = new K3b::DataProjectModel( doc, this );
    //d->model->setListOnlyDirs( true );
    setModel( d->model );

    for ( int i = K3b::DataProjectModel::TypeColumn; i < K3b::DataProjectModel::NumColumns; ++i ) {
        hideColumn( i );
    }

    connect( d->model, SIGNAL(addUrlsRequested(QList<QUrl>,K3b::DirItem*)),
                       SLOT(slotAddUrlsRequested(QList<QUrl>,K3b::DirItem*)) );
    connect( d->model, SIGNAL(moveItemsRequested(QList<K3b::DataItem*>,K3b::DirItem*)),
                       SLOT(slotMoveItemsRequested(QList<K3b::DataItem*>,K3b::DirItem*)) );
    connect( selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
                               SLOT(slotSelectionChanged(QItemSelection,QItemSelection)) );
}


K3b::DataDirTreeView::~DataDirTreeView()
{
    delete d;
}


K3b::DataItem* K3b::DataDirTreeView::itemAt( const QPoint& pos )
{
    return d->model->itemForIndex( indexAt( pos ) );
}


K3b::DirItem* K3b::DataDirTreeView::selectedDir() const
{
    if ( K3b::DataItem* item = d->model->itemForIndex( selectionModel()->currentIndex() ) ) {
        return item->getDirItem();
    }
//     QModelIndexList indices = selectionModel()->selectedRows();
//     if ( !indices.isEmpty() ) {
//         return d->model->itemForIndex( indices.first() )->getDirItem();
//     }
    else {
        return 0;
    }
}


void K3b::DataDirTreeView::slotSelectionChanged( const QItemSelection& selected, const QItemSelection& )
{
    QModelIndexList indexes = selected.indexes();
    if ( !indexes.isEmpty() ) {
        K3b::DirItem* dir = d->model->itemForIndex( indexes.first() )->getDirItem();
        emit dirSelected( dir );
    }
}


void K3b::DataDirTreeView::slotAddUrlsRequested( QList<QUrl> urls, K3b::DirItem* targetDir )
{
    DataUrlAddingDialog::addUrls( urls, targetDir, m_view );
}


void K3b::DataDirTreeView::slotMoveItemsRequested( QList<K3b::DataItem*> items, K3b::DirItem* targetDir )
{
    DataUrlAddingDialog::moveItems( items, targetDir, m_view );
}


void K3b::DataDirTreeView::setCurrentDir( K3b::DirItem* dirItem )
{
    selectionModel()->select( d->model->indexForItem( dirItem ), QItemSelectionModel::SelectCurrent );
}



#if 0
void K3b::DataDirTreeView::startDropAnimation( K3b::DirItem* dir )
{
    stopDropAnimation();

    K3b::DataDirViewItem* vI = m_itemMap[dir];
    if( vI ) {
        d->animationCounter = 0;
        d->animatedDirItem = vI;
        d->beforeAniPixmap = QPixmap( *vI->pixmap(0) );
        QTimer::singleShot( 0, this, SLOT(slotDropAnimate()) );
    }
}


void K3b::DataDirTreeView::slotDropAnimate()
{
    if( d->animatedDirItem ) {
        if( d->animationCounter > 5 )
            stopDropAnimation();
        else {
            switch(d->animationCounter) {
            case 0:
                d->animatedDirItem->setPixmap( 0, SmallIcon( "folder-cyan" ) );
                break;
            case 1:
                d->animatedDirItem->setPixmap( 0, SmallIcon( "folder-green" ) );
                break;
            case 2:
                d->animatedDirItem->setPixmap( 0, SmallIcon( "folder-yellow" ) );
                break;
            case 3:
                d->animatedDirItem->setPixmap( 0, SmallIcon( "folder-orange" ) );
                break;
            case 4:
                d->animatedDirItem->setPixmap( 0, SmallIcon( "folder-red" ) );
                break;
            case 5:
                d->animatedDirItem->setPixmap( 0, SmallIcon( "folder-violet" ) );
                break;
            }

            d->animationCounter++;
            QTimer::singleShot( 300, this, SLOT(slotDropAnimate()) );
        }
    }
}


void K3b::DataDirTreeView::stopDropAnimation()
{
    if( d->animatedDirItem ) {
        d->animatedDirItem->setPixmap( 0, d->beforeAniPixmap );
        d->animatedDirItem = 0;
    }
}
#endif


