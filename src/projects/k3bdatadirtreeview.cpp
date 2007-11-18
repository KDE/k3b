/*
 *
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This file is part of the K3b project.
 * Copyright (C) 1998-2007 Sebastian Trueg <trueg@k3b.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * See the file "COPYING" for the exact licensing terms.
 */


#include "k3bdatadirtreeview.h"
#include "k3bdatafileview.h"
#include "k3bdataview.h"
#include "k3bdatadoc.h"
#include "k3bdataitem.h"
#include "k3bdiritem.h"
#include "k3bdatapropertiesdialog.h"
#include "k3bdataviewitem.h"
#include "k3bdataurladdingdialog.h"
#include <k3bview.h>
#include <k3bvalidators.h>

#include <q3dragobject.h>
#include <q3header.h>
#include <qtimer.h>
//Added by qt3to4:
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QDragLeaveEvent>
#include <Q3ValueList>
#include <QPixmap>
#include <Q3PtrList>

#include <klocale.h>
#include <kaction.h>
#include <k3urldrag.h>
#include <kinputdialog.h>
#include <kiconloader.h>
#include <kshortcut.h>
#include <kactioncollection.h>
#include <kdebug.h>
#include <KMenu>



class K3bDataDirTreeView::Private
{
public:
    Private()
        : animatedDirItem(0),
          dropDirItem(0) {
    }

    K3bDataDirViewItem* animatedDirItem;
    K3bDataDirViewItem* dropDirItem;
    int animationCounter;
    QPixmap beforeAniPixmap;

    // used for the urladdingdialog hack
    KUrl::List addUrls;
    K3bDirItem* addParentDir;

    QString lastUpdateVolumeId;

    QValidator* iso9660Validator;
    QValidator* asciiValidator;
};


K3bDataDirTreeView::K3bDataDirTreeView( K3bView* view, K3bDataDoc* doc, QWidget* parent )
    : K3bListView( parent ), m_view(view)
{
    d = new Private();

    m_fileView = 0;

    setAcceptDrops( true );
    setDropVisualizer( false );
    setDropHighlighter( true );
    setRootIsDecorated( false );
    setFullWidth( true );
    setDragEnabled( true );
    setItemsMovable( false );
    setAlternateBackground( QColor() );
    //  setSorting(-1);

    addColumn( i18n("Directory") );
    header()->hide();

    m_doc = doc;

    m_root = new K3bDataRootViewItem( doc, this );
    m_itemMap.insert( doc->root(), m_root );

    connect( m_doc, SIGNAL(changed()), this, SLOT(slotDocChanged()) );
    connect( this, SIGNAL(clicked(Q3ListViewItem*)), this, SLOT(slotExecuted(Q3ListViewItem*)) );
    connect( this, SIGNAL(selectionChanged(Q3ListViewItem*)), this, SLOT(slotExecuted(Q3ListViewItem*)) );
    connect( m_doc, SIGNAL(itemRemoved(K3bDataItem*)), this, SLOT(slotDataItemRemoved(K3bDataItem*)) );
    connect( m_doc, SIGNAL(itemAdded(K3bDataItem*)), this, SLOT(slotItemAdded(K3bDataItem*)) );
    connect( this, SIGNAL(contextMenu(K3ListView*,Q3ListViewItem*, const QPoint&)),
             this, SLOT(showPopupMenu(K3ListView*,Q3ListViewItem*, const QPoint&)) );
    connect( this, SIGNAL(dropped(QDropEvent*, Q3ListViewItem*, Q3ListViewItem*)),
             this, SLOT(slotDropped(QDropEvent*, Q3ListViewItem*, Q3ListViewItem*)) );

    setupActions();
}


K3bDataDirTreeView::~K3bDataDirTreeView()
{
    delete d;
}


void K3bDataDirTreeView::slotExecuted( Q3ListViewItem* item )
{
    if( K3bDataDirViewItem* viewItem = dynamic_cast<K3bDataDirViewItem*>(item) )
        emit dirSelected( viewItem->dirItem() );
}


bool K3bDataDirTreeView::acceptDrag(QDropEvent* e) const{
    return ( e->source() == viewport() || K3URLDrag::canDecode(e) ||
             ( m_fileView && e->source() == m_fileView->viewport() ) );
}


void K3bDataDirTreeView::contentsDragMoveEvent( QDragMoveEvent* e )
{
    K3bListView::contentsDragMoveEvent( e );

    // highlight the folder the items would be added to
    if( d->dropDirItem )
        d->dropDirItem->highlightIcon( false );

    d->dropDirItem = dynamic_cast<K3bDataDirViewItem*>( itemAt(contentsToViewport(e->pos())) );
    if( !d->dropDirItem )
        d->dropDirItem = m_root;

    d->dropDirItem->highlightIcon( true );
}


void K3bDataDirTreeView::contentsDragLeaveEvent( QDragLeaveEvent* e )
{
    K3bListView::contentsDragLeaveEvent( e );

    // remove any highlighting
    if( d->dropDirItem ) {
        d->dropDirItem->highlightIcon( false );
        d->dropDirItem = 0;
    }
}


void K3bDataDirTreeView::slotDropped( QDropEvent* e, Q3ListViewItem*, Q3ListViewItem* )
{
    // remove any highlighting
    if( d->dropDirItem ) {
        d->dropDirItem->highlightIcon( false );
        d->dropDirItem = 0;
    }

    if( !e->isAccepted() )
        return;

    // determine K3bDirItem to add the items to
    if( K3bDataDirViewItem* dirViewItem = dynamic_cast<K3bDataDirViewItem*>( itemAt(contentsToViewport(e->pos())) ) ) {
        d->addParentDir = dirViewItem->dirItem();
    }
    else {
        d->addParentDir = m_doc->root();
    }

    if( d->addParentDir ) {

        //    startDropAnimation( parent );

        // check if items have been moved
        if( m_fileView &&
            e->source() == m_fileView->viewport() ) {
            // move all selected items
            QList<Q3ListViewItem*> selectedViewItems = m_fileView->selectedItems();
            QList<K3bDataItem*> selectedDataItems;
            Q_FOREACH( Q3ListViewItem* item, selectedViewItems ) {
                K3bDataViewItem* dataViewItem = dynamic_cast<K3bDataViewItem*>( item );
                if( dataViewItem )
                    selectedDataItems.append( dataViewItem->dataItem() );
                else
                    kDebug() << "no dataviewitem";
            }

            K3bDataUrlAddingDialog::copyMoveItems( selectedDataItems, d->addParentDir, this, e->action() == QDropEvent::Copy );
        }
        else if( e->source() == viewport() ) {
            // move the selected dir
            if( K3bDataDirViewItem* dirItem = dynamic_cast<K3bDataDirViewItem*>( selectedItem() ) ) {
                QList<K3bDataItem*> selectedDataItems;
                selectedDataItems.append( dirItem->dirItem() );
                K3bDataUrlAddingDialog::copyMoveItems( selectedDataItems, d->addParentDir, this, e->action() == QDropEvent::Copy );
            }
        }
        else {
            // seems that new items have been dropped
            d->addUrls.clear();
            if( K3URLDrag::decode( e, d->addUrls ) ) {
                //
                // This is a small (not to ugly) hack to circumvent problems with the
                // event queues: the url adding dialog will be non-modal regardless of
                // the settings in case we open it directly.
                //
                QTimer::singleShot( 0, this, SLOT(slotAddUrls()) );
            }
        }
    }

    // now grab that focus
    setFocus();
}


void K3bDataDirTreeView::slotAddUrls()
{
    K3bDataUrlAddingDialog::addUrls( d->addUrls, d->addParentDir, this );
}


void K3bDataDirTreeView::slotItemAdded( K3bDataItem* item )
{
    if( item->isDir() ) {
        //
        // We assume that we do not already have an item for the dir since the itemAdded signal
        // should only be emitted once for every item
        //
        K3bDirItem* dirItem = static_cast<K3bDirItem*>( item );
        K3bDataDirViewItem* parentViewItem = m_itemMap[dirItem->parent()];
        K3bDataDirViewItem* newDirItem = new K3bDataDirViewItem( dirItem, parentViewItem );
        m_itemMap.insert( dirItem, newDirItem );
    }
}


void K3bDataDirTreeView::slotDataItemRemoved( K3bDataItem* item )
{
    if( item->isDir() ) {
        K3bDirItem* dirItem = static_cast<K3bDirItem*>( item );
        QMap<K3bDirItem*, K3bDataDirViewItem*>::iterator it = m_itemMap.find( dirItem );
        if( it != m_itemMap.end() ) {
            K3bDataDirViewItem* viewItem = *it;
            m_itemMap.remove( it );

            // we don't get removedInfo for the child items
            // so we need to remove them here
            Q_FOREACH( K3bDataItem* item, dirItem->children() ) {
                if( item->isDir() )
                    slotDataItemRemoved( item );
            }

            delete viewItem;
        }
    }
}


void K3bDataDirTreeView::setCurrentDir( K3bDirItem* dirItem )
{
    QMap<K3bDirItem*, K3bDataDirViewItem*>::iterator it = m_itemMap.find( dirItem );
    if( it != m_itemMap.end() ) {
        setCurrentItem( *it );
        it.value()->setOpen(true);
        if( it.value() != root() )
            it.value()->parent()->setOpen(true);
    }
    else {
        kDebug() << "Tried to set unknown dirItem to current";
    }
}


void K3bDataDirTreeView::setupActions()
{
    m_actionCollection = new KActionCollection( this );

    m_actionProperties = new KAction( this );
    m_actionProperties->setText( i18n("Properties") );
    m_actionProperties->setIcon( KIcon( "misc" ) );
    connect( m_actionProperties, SIGNAL( triggered() ), this, SLOT( slotProperties() ) );
    m_actionCollection->addAction( "properties", m_actionProperties );

    m_actionNewDir = new KAction( this );
    m_actionNewDir->setText( i18n("New Directory...") );
    m_actionNewDir->setShortcut( Qt::CTRL+Qt::Key_N );
    connect( m_actionNewDir, SIGNAL( triggered() ), this, SLOT( slotNewDir() ) );
    m_actionCollection->addAction( "new_dir", m_actionNewDir );

    m_actionRemove = new KAction( this );
    m_actionRemove->setText( i18n("Remove") );
    m_actionRemove->setIcon( KIcon( "editdelete" ) );
    m_actionRemove->setShortcut( Qt::Key_Delete );
    connect( m_actionRemove, SIGNAL( triggered() ), this, SLOT( slotRemoveItem() ) );
    m_actionCollection->addAction( "remove", m_actionRemove );

    m_actionRename = new KAction( this );
    m_actionRename->setText( i18n("Rename") );
    m_actionRename->setShortcut( Qt::Key_F2 );
    m_actionRename->setIcon( KIcon( "edit" ) );
    connect( m_actionRemove, SIGNAL( triggered() ), this, SLOT( slotRenameItem() ) );
    m_actionCollection->addAction( "rename", m_actionRename );

    m_popupMenu = new KMenu( this );
    m_popupMenu->addAction( m_actionRename );
    m_popupMenu->addAction( m_actionRemove );
    m_popupMenu->addAction( m_actionNewDir );
    m_popupMenu->addSeparator();
    m_popupMenu->addAction( m_actionProperties );
    m_popupMenu->addSeparator();
    m_popupMenu->addAction( m_view->actionCollection()->action("project_burn") );
}


void K3bDataDirTreeView::showPopupMenu( K3ListView*, Q3ListViewItem* item, const QPoint& point )
{
    if( item ) {
        if( K3bDataViewItem* di = dynamic_cast<K3bDataViewItem*>(item) ) {
            m_actionRemove->setEnabled( di->dataItem()->isRemoveable() );
            m_actionRename->setEnabled( di->dataItem()->isRenameable() );
        }
        else {
            m_actionRemove->setEnabled( false );
            m_actionRename->setEnabled( false );
        }
        m_actionProperties->setEnabled( true );
    }
    else {
        m_actionRemove->setEnabled( false );
        m_actionRename->setEnabled( false );
        m_actionProperties->setEnabled( false );
    }

    m_popupMenu->popup( point );
}


void K3bDataDirTreeView::slotNewDir()
{
    if( K3bDataDirViewItem* vI = dynamic_cast<K3bDataDirViewItem*>(currentItem()) ) {
        K3bDirItem* parent = vI->dirItem();

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
}


void K3bDataDirTreeView::slotRenameItem()
{
    showEditor( (K3bListViewItem*)currentItem(), 0 );
}


void K3bDataDirTreeView::slotRemoveItem()
{
    if( currentItem() ) {
        if( K3bDataDirViewItem* dirViewItem = dynamic_cast<K3bDataDirViewItem*>( currentItem() ) )
            m_doc->removeItem( dirViewItem->dirItem() );
    }
}


void K3bDataDirTreeView::slotProperties()
{
    K3bDataViewItem* viewItem = dynamic_cast<K3bDataViewItem*>( currentItem() );
    if( viewItem && currentItem() != root() ) {
        Q3ValueList<K3bDataItem*> dataItems;
        dataItems.append( viewItem->dataItem() );
        K3bDataPropertiesDialog d( dataItems, this );
        if( d.exec() ) {
            repaint();
            if( m_fileView )
                m_fileView->repaint();
        }
    }
    else
        m_view->slotProperties();
}


void K3bDataDirTreeView::startDropAnimation( K3bDirItem* dir )
{
    stopDropAnimation();

    K3bDataDirViewItem* vI = m_itemMap[dir];
    if( vI ) {
        d->animationCounter = 0;
        d->animatedDirItem = vI;
        d->beforeAniPixmap = QPixmap( *vI->pixmap(0) );
        QTimer::singleShot( 0, this, SLOT(slotDropAnimate()) );
    }
}


void K3bDataDirTreeView::slotDropAnimate()
{
    if( d->animatedDirItem ) {
        if( d->animationCounter > 5 )
            stopDropAnimation();
        else {
            switch(d->animationCounter) {
            case 0:
                d->animatedDirItem->setPixmap( 0, SmallIcon( "folder_cyan" ) );
                break;
            case 1:
                d->animatedDirItem->setPixmap( 0, SmallIcon( "folder_green" ) );
                break;
            case 2:
                d->animatedDirItem->setPixmap( 0, SmallIcon( "folder_yellow" ) );
                break;
            case 3:
                d->animatedDirItem->setPixmap( 0, SmallIcon( "folder_orange" ) );
                break;
            case 4:
                d->animatedDirItem->setPixmap( 0, SmallIcon( "folder_red" ) );
                break;
            case 5:
                d->animatedDirItem->setPixmap( 0, SmallIcon( "folder_violet" ) );
                break;
            }

            d->animationCounter++;
            QTimer::singleShot( 300, this, SLOT(slotDropAnimate()) );
        }
    }
}


void K3bDataDirTreeView::stopDropAnimation()
{
    if( d->animatedDirItem ) {
        d->animatedDirItem->setPixmap( 0, d->beforeAniPixmap );
        d->animatedDirItem = 0;
    }
}


// FIXME: remove this
void K3bDataDirTreeView::checkForNewItems()
{
    K3bDataItem* item = m_root->dirItem()->nextSibling();
    while( item != 0 )
    {
        // check if we have an entry and if not, create one
        // we can assume that a listViewItem for the parent exists
        // since we go top to bottom
        if( item->isDir() )
	{
            K3bDirItem* dirItem = dynamic_cast<K3bDirItem*>( item );

            QMap<K3bDirItem*, K3bDataDirViewItem*>::iterator itDirItem = m_itemMap.find( dirItem );
            if( itDirItem == m_itemMap.end() ) {
                K3bDataDirViewItem* parentViewItem = m_itemMap[dirItem->parent()];
                K3bDataDirViewItem* newDirItem = new K3bDataDirViewItem( dirItem, parentViewItem );
                m_itemMap.insert( dirItem, newDirItem );
            }
            else {
                // check if parent still correct (to get moved items)
                K3bDataDirViewItem* dirViewItem = itDirItem.value();
                K3bDataDirViewItem* parentViewItem = (K3bDataDirViewItem*)dirViewItem->parent();
                K3bDataDirViewItem* dirParentViewItem = m_itemMap[dirItem->parent()];
                if( dirParentViewItem != parentViewItem ) {
                    // reparent it
                    parentViewItem->takeItem( dirViewItem );
                    dirParentViewItem->insertItem( dirViewItem );
                }
            }
	}

        item = item->nextSibling();
    }


    // check the directory depth
    Q3ListViewItemIterator it(root());
    while( it.current() != 0 ) {
        if( K3bDataDirViewItem* dirViewItem = dynamic_cast<K3bDataDirViewItem*>(it.current()) )
            if( it.current() != m_root ) {
                K3bDirItem* dirItem = dirViewItem->dirItem();
                dirViewItem->setPixmap( 0, dirItem->depth() > 7 ? SmallIcon( "folder_red" ) : SmallIcon( "folder" ) );
            }

        ++it;
    }

    // always show the first level
    m_root->setOpen( true );
}


void K3bDataDirTreeView::slotDocChanged()
{
    // avoid flicker
    if( d->lastUpdateVolumeId != m_doc->isoOptions().volumeID() ) {
        d->lastUpdateVolumeId = m_doc->isoOptions().volumeID();
        root()->repaint();
    }
}

#include "k3bdatadirtreeview.moc"
