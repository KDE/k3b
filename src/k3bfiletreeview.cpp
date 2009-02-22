/*
 *
 * Copyright (C) 2003-2007 Sebastian Trueg <trueg@k3b.org>
 *           (C) 2009      Gustavo Pichorim Boiko <gustavo.boiko@kdemail.net>
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


#include "k3bfiletreeview.h"
#include "k3bappdevicemanager.h"
#include "k3bapplication.h"
#include "k3bmediacache.h"
#include "k3btooltip.h"
#include "k3bthememanager.h"
#include "k3bplacesmodel.h"
#include "k3bdevicedelegate.h"
#include "k3bdevicemenu.h"
#include "k3baction.h"
#include "k3b.h"

#include <k3bdevice.h>
#include <k3bdiskinfo.h>
#include <k3bglobals.h>

#include <kiconloader.h>
#include <klocale.h>
#include <kurl.h>
#include <kstandarddirs.h>
#include <kaction.h>
#include <kio/global.h>
#include <kfileitem.h>
#include <kapplication.h>
#include <kglobalsettings.h>
#include <KActionMenu>

#include <qdir.h>
#include <qevent.h>
#include <q3dragobject.h>
#include <qcursor.h>
#include <qnamespace.h>
#include <qmap.h>
#include <q3ptrdict.h>
#include <qpainter.h>
#include <qfont.h>
#include <qstyle.h>
#include <qlabel.h>
#include <qlayout.h>
#include <QPixmap>
#include <QFrame>
#include <QStyleOption>
#include <QHeaderView>


// void K3b::DeviceBranch::updateLabel()
// {
//     if( m_showBlockDeviceName )
//         setName( QString("%1 %2 (%3)")
//                  .arg(m_device->vendor())
//                  .arg(m_device->description())
//                  .arg(m_device->blockDeviceName()) );
//     else
//         setName( QString("%1 %2")
//                  .arg(m_device->vendor())
//                  .arg(m_device->description()) );

//     if( k3bappcore->mediaCache() ) {
//         root()->setMultiLinesEnabled( true );
//         root()->setText( 0,QString::number(0) +name() + "\n" + k3bappcore->mediaCache()->mediumString( m_device ) );
//         static_cast<K3b::FileTreeView*>( root()->listView() )->updateMinimumWidth();
//     }
//     else {
//         root()->setMultiLinesEnabled( false );
//         root()->setText( 0,QString::number(0)+name() );
//     }
// }

// void K3b::DeviceBranchViewItem::paintCell( QPainter* p, const QColorGroup& cg, int /* col */, int width, int align )
// {
//     p->save();

//     int xpos = 1;
//     int ypos = 1;
//     QFontMetrics fm( p->fontMetrics() );

//     if( isSelected() ) {
//         p->fillRect( 0, 0, width, height(),
//                      cg.brush( QColorGroup::Highlight ) );
//         p->setPen( cg.highlightedText() );
//     }
//     else {
//         p->fillRect( 0, 0, width, height(), cg.base() );
//         p->setPen( cg.text() );
//     }

//     if( pixmap(0) ) {
//         p->drawPixmap( xpos, ypos, *pixmap(0) );
//         xpos += pixmap(0)->width() + 5;
//     }

//     if( m_bCurrent ) {
//         QFont f( listView()->font() );
//         f.setBold( true );
//         p->setFont( f );
//     }

//     ypos += fm.ascent();
//     QString line1 = text(0).left( text(0).indexOf('\n') );
//     p->drawText( xpos, ypos, line1 );

//     QFont f( listView()->font() );
//     f.setItalic( true );
//     f.setBold( false );
//     f.setPointSize( f.pointSize() - 2 );
//     p->setFont( f );

//     ypos += p->fontMetrics().height() + 1;
//     QString line2 = text(0).mid( text(0).indexOf('\n')+1 );
//     p->drawText( xpos - p->fontMetrics().leftBearing( line2[0] ), ypos, line2 );


//     // from QListViewItem
//     if( isOpen() && childCount() ) {
//         int textheight = fm.size( align, text(0) ).height() + 2 * listView()->itemMargin();
//         textheight = qMax( textheight, QApplication::globalStrut().height() );
//         if ( textheight % 2 > 0 )
//             textheight++;
//         if ( textheight < height() ) {
//             //FIXME kde4
// #if 0
//             int w = listView()->treeStepSize() / 2;
//             listView()->style().drawComplexControl( QStyle::CC_ListView, p, listView(),
//                                                     QRect( 0, textheight, w + 1, height() - textheight + 1 ), cg,
//                                                     QStyle::Style_Enabled,
//                                                     QStyle::SC_ListViewExpand,
//                                                     (uint)QStyle::SC_All, QStyleOption( this ) );
// #endif
//         }
//     }

//     p->restore();
// }


// int K3b::DeviceBranchViewItem::widthHint() const
// {
//     QFont f( listView()->font() );
//     if ( m_bCurrent ) {
//         f.setBold( true );
//     }
//     int w = QFontMetrics(f).width( text(0).left( text(0).indexOf('\n') ) );

//     f.setItalic( true );
//     f.setBold( false );
//     f.setPointSize( f.pointSize() - 2 );
//     w = qMax( w, QFontMetrics(f).width( text(0).mid( text(0).indexOf('\n')+1 ) ) );

//     w++; // see paintCell

//     if( pixmap(0) )
//         w += pixmap(0)->width() + 5;

//     return w;
// }


// class K3b::DeviceTreeToolTip : public K3b::ToolTip
// {
// public:
//     K3b::DeviceTreeToolTip( QWidget* parent, K3b::FileTreeView* lv );

//     void maybeTip( const QPoint &pos );

// private:
//     K3b::FileTreeView* m_view;
// };


// K3b::DeviceTreeToolTip::DeviceTreeToolTip( QWidget* parent, K3b::FileTreeView* lv )
//     : K3b::ToolTip( parent ),
//       m_view( lv )
// {
//     setTipTimeout( 500 );
// }


// void K3b::DeviceTreeToolTip::maybeTip( const QPoint& pos )
// {
//     if( !parentWidget() || !m_view )
//         return;

//     K3b::DeviceBranchViewItem* item = dynamic_cast<K3b::DeviceBranchViewItem*>( m_view->itemAt( pos ) );
//     if( !item )
//         return;

//     K3b::Device::Device* dev = static_cast<K3b::DeviceBranch*>( item->branch() )->device();

//     QFrame* tooltip = new QFrame( parentWidget() );
//     tooltip->setFrameStyle( QFrame::Panel | QFrame::Raised );
//     tooltip->setFrameShape( QFrame::StyledPanel );
//     QGridLayout* lay = new QGridLayout( tooltip, 2, 2, tooltip->frameWidth()*2 /*margin*/, 6 /*spacing*/ );

//     QString text = k3bappcore->mediaCache()->medium( dev ).longString();
//     int detailsStart = text.indexOf( "<p>", 3 );
//     QString details = text.mid( detailsStart );
//     text.truncate( detailsStart );

//     QLabel* label = new QLabel( text, tooltip );
//     label->setMargin( 9 );
//     lay->addMultiCellWidget( label, 0, 0, 0, 1 );
//     label = new QLabel( details, tooltip );
//     label->setMargin( 9 );
//     label->setAlignment( Qt::Vertical );
//     lay->addMultiCellWidget( label, 1, 2, 0, 0 );
//     label = new QLabel( tooltip );
//     lay->addWidget( label, 2, 1 );
//     lay->setColumnStretch( 0, 1 );

//     if( K3b::Theme* theme = k3bappcore->themeManager()->currentTheme() ) {
//         tooltip->setPaletteBackgroundColor( theme->backgroundColor() );
//         tooltip->setPaletteForegroundColor( theme->foregroundColor() );
//         K3b::Theme::PixmapType pm;
//         int c = k3bappcore->mediaCache()->medium( dev ).content();
//         if( c & (K3b::Medium::CONTENT_VIDEO_CD|K3b::Medium::CONTENT_VIDEO_DVD) )
//             pm = K3b::Theme::MEDIA_VIDEO;
//         else if( c & K3b::Medium::CONTENT_AUDIO &&
//                  c & K3b::Medium::CONTENT_DATA )
//             pm = K3b::Theme::MEDIA_MIXED;
//         else if( c & K3b::Medium::CONTENT_AUDIO )
//             pm = K3b::Theme::MEDIA_AUDIO;
//         else if( c & K3b::Medium::CONTENT_DATA )
//             pm = K3b::Theme::MEDIA_DATA;
//         else {
//             K3b::Device::DiskInfo di = k3bappcore->mediaCache()->diskInfo( dev );
//             if( di.diskState() == K3b::Device::STATE_EMPTY )
//                 pm = K3b::Theme::MEDIA_EMPTY;
//             else
//                 pm = K3b::Theme::MEDIA_NONE;
//         }
//         label->setPixmap( theme->pixmap( pm ) );
//     }

//     // the tooltip will take care of deleting the widget
//     tip( m_view->itemRect( item ), tooltip );
// }



class K3b::FileTreeView::Private
{
public:
    Private()
        : deviceManager(0) {
    }

    K3b::Device::DeviceManager* deviceManager;

    K3b::PlacesModel* model;

    KActionCollection* actionCollection;
    K3b::DeviceMenu* devicePopupMenu;
    KActionMenu* urlPopupMenu;
    bool menuEnabled;
};


K3b::FileTreeView::FileTreeView( QWidget *parent )
    : QTreeView( parent ),
      d( new Private() )
{
    header()->hide();

    setContextMenuPolicy( Qt::CustomContextMenu );
    setSelectionMode(QAbstractItemView::SingleSelection);
//    setSortingEnabled( true );
//    setRootIsDecorated( false );
    setDragEnabled( true );

    K3b::DeviceDelegate* delegate = new K3b::DeviceDelegate(this);
    setItemDelegate(delegate);

    d->model = new K3b::PlacesModel( this );
    setModel( d->model );

    d->actionCollection = new KActionCollection( this );
    d->devicePopupMenu = new K3b::DeviceMenu( this );
    d->urlPopupMenu = new KActionMenu(this);
    initActions();

    // react on K3b::PlacesModel::expandToUrl calls
    connect( d->model, SIGNAL( expand( const QModelIndex& ) ),
             this, SLOT( slotExpandUrl( const QModelIndex& ) ) );

    // add the default places
    d->model->addPlace( i18n( "Home" ), KIcon("user-home"), QDir::homePath() );
    d->model->addPlace( i18n( "Root" ), KIcon("folder-red"), KUrl( "/" ) );

    connect( this, SIGNAL(clicked(const QModelIndex&)), SLOT(slotClicked(const QModelIndex&)) );
    connect( this, SIGNAL(customContextMenuRequested( const QPoint& )), SLOT( slotContextMenu( const QPoint& ) ) );
}


K3b::FileTreeView::~FileTreeView()
{
    delete d;
}

void K3b::FileTreeView::initActions()
{
    // those actions are supposed to be used with url items
    d->urlPopupMenu->addAction( K3b::createAction(this,i18n("&Add to Project"), 0, Qt::SHIFT+Qt::Key_Return,
                                                  this, SLOT(slotAddFilesToProject()),
                                                  d->actionCollection, "add_files_to_project") );
}

K3b::Device::Device* K3b::FileTreeView::selectedDevice() const
{
    return d->model->deviceForIndex( currentIndex() );
}


KUrl K3b::FileTreeView::selectedUrl() const
{
    KFileItem fileItem = d->model->itemForIndex( currentIndex() );
    if( fileItem.isNull() )
        return KUrl();
    else
        return fileItem.url();
}


void K3b::FileTreeView::slotExpandUrl( const QModelIndex& index )
{
    kDebug();
    expand( index );
    setCurrentIndex( index );
    scrollTo( index );
}

void K3b::FileTreeView::slotAddFilesToProject()
{
    QModelIndexList indexes = selectedIndexes();
    KUrl::List files;
    foreach(QModelIndex index, indexes)
    {
        KFileItem item = d->model->itemForIndex(index);
        if (item.isNull())
            continue;

        files.append(item.url());
    }

    if (!files.isEmpty())
        k3bappcore->k3bMainWindow()->addUrls(files);
}


void K3b::FileTreeView::setSelectedUrl( const KUrl& url )
{
    kDebug();
    d->model->expandToUrl( url );
}


void K3b::FileTreeView::setSelectedDevice( K3b::Device::Device* dev )
{
    setCurrentIndex( d->model->indexForDevice( dev ) );
}


// void K3b::FileTreeView::addCdDeviceBranches( K3b::Device::DeviceManager* dm )
// {
//     kDebug() << "(K3b::FileTreeView::addCdDeviceBranches)";

//     // remove all previous added device branches
//     for( QMap<KFileTreeBranch*, K3b::Device::Device*>::Iterator it = d->branchDeviceMap.begin();
//          it != d->branchDeviceMap.end(); ++it ) {
//         removeBranch( it.key() );
//     }

//     // clear the maps
//     d->branchDeviceMap.clear();
//     d->deviceBranchDict.clear();
//     QList<K3b::Device::Device *> items(dm->allDevices());
//     for( QList<K3b::Device::Device *>::const_iterator it = items.begin();
//          it != items.end(); ++it )
//         addDeviceBranch( *it );

//     if( dm != d->deviceManager ) {
//         if( d->deviceManager )
//             d->deviceManager->disconnect( this );
//         d->deviceManager = dm;

//         // make sure we get changes to the config
//         connect( dm, SIGNAL(changed(K3b::Device::DeviceManager*)),
//                  this, SLOT(addCdDeviceBranches(K3b::Device::DeviceManager*)) );

//         if( K3b::AppDeviceManager* appDevM = dynamic_cast<K3b::AppDeviceManager*>( dm ) )
//             connect( appDevM, SIGNAL(currentDeviceChanged(K3b::Device::Device*)),
//                      this, SLOT(setCurrentDevice(K3b::Device::Device*)) );
//     }

//     K3b::Device::Device* currentDevice = k3bappcore->appDeviceManager()->currentDevice();
//     if ( !currentDevice && !k3bappcore->appDeviceManager()->allDevices().isEmpty() ) {
//         k3bappcore->appDeviceManager()->setCurrentDevice( k3bappcore->appDeviceManager()->allDevices().first() );
//     }

//     d->currentDeviceBranch = d->deviceBranchDict[currentDevice];
//     if( d->currentDeviceBranch ) {
//         d->currentDeviceBranch->setCurrent( true );
//     }

//     kDebug() << "(K3b::FileTreeView::addCdDeviceBranches) done";
// }


// void K3b::FileTreeView::addDeviceBranch( K3b::Device::Device* dev )
// {
//     K3b::DeviceBranch* newBranch = new K3b::DeviceBranch( this, dev );
//     addBranch( newBranch );

//     // search for an equal device
//     int equalCnt = 0;
//     K3b::DeviceBranch* equalBranch = 0;
//     for( QMap<KFileTreeBranch*, K3b::Device::Device*>::Iterator it = d->branchDeviceMap.begin();
//          it != d->branchDeviceMap.end(); ++it ) {
//         K3b::Device::Device* itDev = it.data();
//         K3b::DeviceBranch* itBranch = (K3b::DeviceBranch*)it.key();
//         if( itDev->vendor() == dev->vendor() &&
//             itDev->description() == dev->description() ) {
//             ++equalCnt;
//             equalBranch = itBranch;
//         }
//     }

//     // if there is at least one equal device add the block device name
//     // if there is more than one equal device they have been updated after
//     // adding the last one so there is no need to update more than two
//     if( equalCnt > 0 ) {
//         kDebug() << "(K3b::FileTreeView) equal branch";
//         newBranch->showBlockDeviceName(true);
//         equalBranch->showBlockDeviceName(true);
//     }

//     // add to maps
//     d->branchDeviceMap.insert( newBranch, dev );
//     d->deviceBranchDict.insert( (void*)dev, newBranch );

//     updateMinimumWidth();
// }

void K3b::FileTreeView::slotClicked( const QModelIndex& index )
{
    if ( K3b::Device::Device* dev = d->model->deviceForIndex( index ) ) {
        k3bappcore->appDeviceManager()->setCurrentDevice( dev );
        emit activated( dev );
    }
    else if ( index.isValid() ) {
        emit activated( d->model->itemForIndex( index ).url() );
    }
}


void K3b::FileTreeView::slotContextMenu( const QPoint& pos )
{
    // check if the context menu is for a device item
    QModelIndex index = indexAt( pos );
    if ( K3b::Device::Device* dev = d->model->deviceForIndex( index ) ) {
        k3bappcore->appDeviceManager()->setCurrentDevice( dev );
        d->devicePopupMenu->exec( mapToGlobal( pos ) );
    }

    // ... or if it is for an url item
    KFileItem item = d->model->itemForIndex( index );
    if ( !item.isNull() )
    {
        // enable/disable the "add to project" action
        d->actionCollection->action("add_files_to_project")->setEnabled(k3bappcore->k3bMainWindow()->activeView() != 0);

        // and shows the menu
        d->urlPopupMenu->menu()->exec( mapToGlobal( pos ) );
    }
}



// void K3b::FileTreeView::slotMouseButtonClickedK3b( int btn, Q3ListViewItem *item, const QPoint &pos, int c )
// {
//     if( (btn == Qt::LeftButton) && item )
//         emitExecute(item, pos, c);
// }


// void K3b::FileTreeView::updateMinimumWidth()
// {
//     //
//     // only handle the device branches, we don't care about the folders.
//     //
//     int w = 0;
//     for( QMap<KFileTreeBranch*, K3b::Device::Device*>::Iterator it = d->branchDeviceMap.begin();
//          it != d->branchDeviceMap.end(); ++it ) {
//         w = qMax( w, static_cast<K3b::DeviceBranchViewItem*>( it.key()->root() )->widthHint() );
//     }

//     // width of the items + scrollbar width + the frame + a little eyecandy spacing
//     setMinimumWidth( w + verticalScrollBar()->sizeHint().width() + 2*frameWidth() + 2 );
// }

#include "k3bfiletreeview.moc"
