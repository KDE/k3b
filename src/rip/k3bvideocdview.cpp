/*
*
* Copyright (C) 2003 Christian Kvasny <chris@k3b.org>
* Copyright (C) 2008 Sebastian Trueg <trueg@k3b.org>
* Copyright (C) 2010-2011 Michal Malek <michalm@jabster.pl>
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

#include "k3bvideocdview.h"
#include "k3bvideocdrippingdialog.h"
#include "k3bvideocdinfo.h"
#include "k3bappdevicemanager.h"
#include "k3bapplication.h"
#include "k3bdevice.h"
#include "k3bmsf.h"
#include "k3btoc.h"
#include "k3bcore.h"
#include "k3bmedium.h"
#include "k3bstdguiitems.h"

#include <KStandardAction>
#include <KLocalizedString>
#include <KActionMenu>
#include <KMessageBox>
#include <KToolBarSpacerAction>
#include <KToolBar>
#include <KActionCollection>

#include <QDebug>
#include <QList>
#include <QCursor>
#include <QFont>
#include <QApplication>
#include <QAction>
#include <QHeaderView>
#include <QMenu>
#include <QLabel>
#include <QStyle>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QDomElement>

namespace {

    class VideoTrackViewItem : public QTreeWidgetItem
    {
    public:
        VideoTrackViewItem( QTreeWidgetItem* parent, QTreeWidgetItem* preceding )
            : QTreeWidgetItem( parent, preceding )
        {
            setFlags( flags() & ~Qt::ItemIsUserCheckable );
        }

        VideoTrackViewItem( QTreeWidget* parent, QTreeWidgetItem* preceding )
            : QTreeWidgetItem( parent, preceding )
        {
            setFlags( flags() & ~Qt::ItemIsUserCheckable );
        }

        VideoTrackViewItem( QTreeWidgetItem* parent,
                            const QString& name,
                            const QString& id,
                            int _trackNumber,
                            const K3b::Msf& length )
            : QTreeWidgetItem( parent )
        {
            setFlags( flags() & ~Qt::ItemIsUserCheckable );

            setText( 0, QString( "%1. %2" ).arg( _trackNumber ).arg( id ) );
            setText( 1, name );
            if ( length > 0 ) {
                setText( 2, length.toString() );
                setText( 3, KIO::convertSize( length.mode2Form2Bytes() ) );
            }

            trackNumber = _trackNumber;
        }

        int trackNumber;

        void updateData( const K3b::VideoCdInfoResultEntry& resultEntry )
        {
            setText( 0, QString( "%1. %2" ).arg( trackNumber ).arg( resultEntry.id ) );
            setText( 1, resultEntry.name );
        }

    };

    class VideoTrackViewCheckItem : public QTreeWidgetItem
    {
    public:
        VideoTrackViewCheckItem( QTreeWidgetItem* parent,
                                const QString& desc )
            : QTreeWidgetItem( parent )
        {
            setText( 0, desc );
            setFlags( flags() | Qt::ItemIsUserCheckable );
            setCheckState( 0, Qt::Checked );
        }

        VideoTrackViewCheckItem( QTreeWidget* parent,
                                  const QString& desc )
            : QTreeWidgetItem( parent )
        {
            setText( 0, desc );
            setFlags( flags() | Qt::ItemIsUserCheckable );
            setCheckState( 0, Qt::Checked );
        }

        VideoTrackViewCheckItem( VideoTrackViewCheckItem* parent,
                                const QString& desc )
            : QTreeWidgetItem( parent )
        {
            setText( 0, desc );
            setFlags( flags() | Qt::ItemIsUserCheckable );
            setCheckState( 0, Qt::Checked );
        }

        void updateData( const K3b::Msf& length, bool form2 = false )
        {
            setText( 2, length.toString() );
            if ( form2 )
                setText( 3, KIO::convertSize( length.mode2Form2Bytes() ) );
            else
                setText( 3, KIO::convertSize( length.mode2Form1Bytes() ) );
        }

    };

    void setCheckState( QTreeWidgetItem* item, Qt::CheckState checkState )
    {
        if( item->flags() & Qt::ItemIsUserCheckable ) {
            item->setCheckState( 0, checkState );
        }

        for( int i = 0; i < item->childCount(); ++i ) {
            setCheckState( item->child( i ), checkState );
        }
    }

} // namespace

class K3b::VideoCdView::Private
{
public:
    Device::Toc toc;

    KActionCollection* actionCollection;
    KActionMenu* popupMenu;

    VideoCdInfoResult videocdinfoResult;
    VideoCdInfo* videocdinfo;
    VideoCdRippingOptions* videooptions;

    QTreeWidget* trackView;
    KToolBar* toolBox;
    QLabel* labelLength;

    QDomDocument domTree;

    QList<VideoTrackViewCheckItem *> contentList;

    unsigned long videocddatasize;
    unsigned long videocdmpegsize;
};


K3b::VideoCdView::VideoCdView( QWidget* parent )
    : K3b::MediaContentsView( true,
                            K3b::Medium::ContentVideoCD,
                            K3b::Device::MEDIA_CD_ALL,
                            K3b::Device::STATE_INCOMPLETE|K3b::Device::STATE_COMPLETE,
                            parent ),
      d( new Private )
{
    // toolbox
    // ----------------------------------------------------------------------------------
    d->toolBox = new KToolBar( mainWidget() );
    d->labelLength = new QLabel( mainWidget() );
    d->labelLength->setAlignment( Qt::AlignVCenter | Qt::AlignRight );
    d->labelLength->setContentsMargins( 0, 0, style()->pixelMetric( QStyle::PM_LayoutRightMargin ), 0 );

    // the track view
    // ----------------------------------------------------------------------------------
    d->trackView = new QTreeWidget( mainWidget() );
    d->trackView->setAllColumnsShowFocus( true );
    d->trackView->setContextMenuPolicy( Qt::CustomContextMenu );
    d->trackView->header()->setSectionResizeMode( 0, QHeaderView::ResizeToContents );

    QTreeWidgetItem* header = d->trackView->headerItem();
    header->setText( 0, i18n( "Item Name" ) );
    header->setText( 1, i18n( "Extracted Name" ) );
    header->setText( 2, i18n( "Length" ) );
    header->setText( 3, i18n( "Size" ) );

    connect( d->trackView, SIGNAL(customContextMenuRequested(QPoint)),
             this, SLOT(slotContextMenu(QPoint)) );
    connect( d->trackView, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
             this, SLOT(slotTrackSelectionChanged(QTreeWidgetItem*,QTreeWidgetItem*)) );
    connect( d->trackView, SIGNAL(itemChanged(QTreeWidgetItem*,int)),
             this, SLOT(slotStateChanged(QTreeWidgetItem*,int)) );

    QVBoxLayout * mainGrid = new QVBoxLayout( mainWidget() );
    mainGrid->addWidget( d->toolBox );
    mainGrid->addWidget( d->trackView );
    mainGrid->setSpacing( 0 );
    mainGrid->setContentsMargins( 0, 0, 0, 0 );

    initActions();

    // setup the toolbox
    d->toolBox->addAction( d->actionCollection->action( "start_rip" ) );
    d->toolBox->addSeparator();
    d->toolBox->addAction( d->actionCollection->action( "view_files" ) );
    d->toolBox->addAction( new KToolBarSpacerAction( d->toolBox ) );
    d->toolBox->addWidget( d->labelLength );

    slotTrackSelectionChanged( 0, 0 );

    d->videocdinfo = 0L;
    d->videooptions = new K3b::VideoCdRippingOptions();

    d->contentList.clear();
}


K3b::VideoCdView::~VideoCdView()
{
    delete d->videocdinfo;
    delete d->videooptions;
    delete d;
}


KActionCollection* K3b::VideoCdView::actionCollection() const
{
    return d->actionCollection;
}


void K3b::VideoCdView::reloadMedium()
{
    d->toc = medium().toc();

    d->trackView->clear();

    d->trackView->setEnabled( false );
    d->toolBox->setEnabled( false );
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );

    d->contentList.clear();
    d->contentList.append( new VideoTrackViewCheckItem( d->trackView, i18n("Video CD MPEG tracks") ) );
    d->contentList.append( new VideoTrackViewCheckItem( d->trackView, i18n("Video CD DATA track" ) ) );

    // create a listviewItem for every video track
    int index = 0;
    d->videocddatasize = 0;
    d->videocdmpegsize = 0;

    K3b::Msf sequenceSize;

    for ( K3b::Device::Toc::const_iterator it = d->toc.constBegin();
          it != d->toc.constEnd(); ++it ) {

        if ( index > 0 ) {
            K3b::Msf length( ( *it ).length() );
            sequenceSize += length;
            d->videocdmpegsize += length.mode2Form2Bytes();
            ( void ) new VideoTrackViewItem( d->contentList[ 0 ], i18n( "Sequence-%1" , index ), "", index, length );
        } else {
            K3b::Msf length( ( *it ).length() );
            d->videocddatasize += length.mode2Form1Bytes();
            ( ( VideoTrackViewCheckItem* ) d->contentList[ 1 ] ) ->updateData( length );
            ( void ) new VideoTrackViewCheckItem( d->contentList[ 1 ], i18n( "Files" ) );
            ( void ) new VideoTrackViewCheckItem( d->contentList[ 1 ], i18n( "Segments" ) );
        }

        index++;
    }

    ( ( VideoTrackViewCheckItem* ) d->contentList[ 0 ] ) ->updateData( sequenceSize, true );

    d->videooptions ->setVideoCdSource( device()->blockDeviceName() );

    d->videocdinfo = new K3b::VideoCdInfo( this );
    d->videocdinfo->info( device()->blockDeviceName() );

    connect( d->videocdinfo, SIGNAL(infoFinished(bool)),
             this, SLOT(slotVideoCdInfoFinished(bool)) );
}

void K3b::VideoCdView::slotVideoCdInfoFinished( bool success )
{
    if ( success ) {
        d->videocdinfoResult = d->videocdinfo->result();
        updateDisplay();
    }

    d->trackView->expandAll();
    d->trackView->setEnabled( true );
    d->toolBox->setEnabled( true );
    QApplication::restoreOverrideCursor();

}

void K3b::VideoCdView::updateDisplay()
{
    // update the listview

    for( int i = 0; i < d->contentList[ 0 ]->childCount(); ++i ) {
        VideoTrackViewItem* child = dynamic_cast<VideoTrackViewItem*>( d->contentList[ 0 ]->child( i ) );
        child->updateData( d->videocdinfoResult.entry( i, K3b::VideoCdInfoResult::SEQUENCE ) );
    }

    for( int i = 0; i < d->contentList[ 1 ]->childCount(); ++i ) {
        QTreeWidgetItem* child = d->contentList[ 1 ]->child( i );
        if ( child->text( 0 ) == i18n( "Files" ) ) {
            if ( d->domTree.setContent( d->videocdinfoResult.xmlData ) ) {

                QDomElement root = d->domTree.documentElement();
                QDomNode node;
                node = root.firstChild();
                while ( !node.isNull() ) {
                    if ( node.isElement() && node.nodeName() == "filesystem" ) {
                        QDomElement body = node.toElement();
                        buildTree( child, body );
                        break;
                    }
                    node = node.nextSibling();
                }
            }
        } else {
            for ( int j = 0; j < d->videocdinfoResult.foundEntries( K3b::VideoCdInfoResult::SEGMENT ); j++ ) {
                ( void ) new VideoTrackViewItem( child,
                                                 d->videocdinfoResult.entry( j, K3b::VideoCdInfoResult::SEGMENT ).name,
                                                 d->videocdinfoResult.entry( j, K3b::VideoCdInfoResult::SEGMENT ).id , j + 1, 0 );
            }
        }
    }

    if ( !d->videocdinfoResult.volumeId.isEmpty() ) {
        QString description = d->videocdinfoResult.volumeId + " (" + d->videocdinfoResult.type + ' ' + d->videocdinfoResult.version + ')' ;
        setTitle( description );
        d->videooptions ->setVideoCdDescription( description );
    }
    else
        setTitle( i18n( "Video CD" ) );

    d->labelLength->setText( i18np( "1 track (%2)", "%1 tracks (%2)", d->toc.count(), K3b::Msf( d->toc.length() ).toString() ) );
}


void K3b::VideoCdView::initActions()
{
    d->actionCollection = new KActionCollection( this );

    QAction *actionCheckAll = new QAction(i18n("Check All"), this);
    d->actionCollection->addAction("check_all", actionCheckAll);
    connect(actionCheckAll, SIGNAL(triggered(bool)), this, SLOT(slotCheckAll()));

    QAction *actionUncheckAll = new QAction(i18n("Uncheck All"), this);
    d->actionCollection->addAction("decheck_all", actionUncheckAll);
    connect(actionUncheckAll, SIGNAL(triggered(bool)), this, SLOT(slotUncheckAll()));

    QAction *actionCheckTrack = new QAction(i18n("Check Track"), this);
    d->actionCollection->addAction("check_track", actionCheckTrack);
    connect(actionCheckTrack, SIGNAL(triggered(bool)), this, SLOT(slotCheck()));

    QAction *actionUncheckTrack = new QAction(i18n("Uncheck Track"), this);
    d->actionCollection->addAction("decheck_track", actionUncheckTrack);
    connect(actionUncheckTrack, SIGNAL(triggered(bool)), this, SLOT(slotUncheck()));

    QAction *actionStartRipping = new QAction(QIcon::fromTheme("tools-rip-video-cd"), i18n("Start Ripping"), this);
    d->actionCollection->addAction("start_rip", actionStartRipping);
    connect(actionStartRipping, SIGNAL(triggered(bool)), this, SLOT(startRip()));

    QAction* actionShowDataPart = new QAction(QIcon::fromTheme("media-optical-data"), i18n("View Files"), this);
    actionShowDataPart->setToolTip(i18n("View plain data files"));
    actionShowDataPart->setStatusTip(actionShowDataPart->toolTip());
    d->actionCollection->addAction("view_files", actionShowDataPart);
    connect(actionShowDataPart, SIGNAL(triggered(bool)), this, SLOT(slotViewFiles()));

    // TODO: set the actions tooltips and whatsthis infos

    // setup the popup menu
    d->popupMenu = new KActionMenu( actionCollection() );
    d->popupMenu->addAction( actionCheckTrack );
    d->popupMenu->addAction( actionUncheckTrack );
    d->popupMenu->addAction( actionCheckAll );
    d->popupMenu->addAction( actionUncheckTrack );
    d->popupMenu->addSeparator();
    d->popupMenu->addAction( actionStartRipping );
}


void K3b::VideoCdView::slotContextMenu( const QPoint& pos )
{
    d->popupMenu->menu()->popup( d->trackView->viewport()->mapToGlobal( pos ) );
}


void K3b::VideoCdView::slotTrackSelectionChanged( QTreeWidgetItem* current, QTreeWidgetItem* /*previous*/ )
{
    actionCollection() ->action( "check_track" ) ->setEnabled( current != 0 );
    actionCollection() ->action( "decheck_track" ) ->setEnabled( current != 0 );
}


void K3b::VideoCdView::slotStateChanged( QTreeWidgetItem* item, int column )
{
    if( item && column == 0 ) {
        for( int i = 0; i < item->childCount(); ++i )
            setCheckState( item->child( i ), item->checkState( 0 ) );
    }
}


void K3b::VideoCdView::startRip()
{
    int selectedItems  = 0;
    QList<QTreeWidgetItem*> children;

    children = d->trackView->findItems( i18n("Video CD MPEG tracks" ), Qt::MatchExactly, 0 );
    if( !children.isEmpty() && children.first()->checkState( 0 ) == Qt::Checked ) {
        d->videooptions ->setVideoCdRipSequences( true );
        ++selectedItems;
    }

    children = d->trackView->findItems( i18n("Files" ), Qt::MatchExactly, 0 );
    if( !children.isEmpty() && children.first()->checkState( 0 ) == Qt::Checked ) {
        d->videooptions ->setVideoCdRipFiles( true );
        ++selectedItems;
    }

    children = d->trackView->findItems( i18n("Segments" ), Qt::MatchExactly, 0 );
    if( !children.isEmpty() && children.first()->checkState( 0 ) == Qt::Checked ) {
        d->videooptions ->setVideoCdRipSegments( true );
        ++selectedItems;
    }

    if( selectedItems == 0 ) {
        KMessageBox::error( this, i18n("Please select the tracks to rip."), i18n("No Tracks Selected") );
    }
    else {
        unsigned long videocdsize = 0;
        // TODO: split SegmentSize and FileSize. Have no infos now
        if ( d->videooptions ->getVideoCdRipSegments() || d->videooptions ->getVideoCdRipFiles())
            videocdsize += d->videocddatasize;
        if ( d->videooptions ->getVideoCdRipSequences() )
            videocdsize += d->videocdmpegsize;

        qDebug() << QString("(K3b::VideoCdView::startRip())  d->videooptions ->setVideoCdSize( %1)").arg( videocdsize );
        d->videooptions ->setVideoCdSize( videocdsize );
        K3b::VideoCdRippingDialog rip( d->videooptions, this );
        rip.exec();
    }
}


void K3b::VideoCdView::slotCheckAll()
{
    for( int i = 0; i < d->trackView->topLevelItemCount(); ++i ) {
        setCheckState( d->trackView->topLevelItem( i ), Qt::Checked );
    }
}


void K3b::VideoCdView::slotUncheckAll()
{
    for( int i = 0; i < d->trackView->topLevelItemCount(); ++i ) {
        setCheckState( d->trackView->topLevelItem( i ), Qt::Unchecked );
    }
}


void K3b::VideoCdView::slotViewFiles()
{
    k3bappcore->appDeviceManager()->mountDisk( device() );
}


void K3b::VideoCdView::slotCheck()
{
    if( QTreeWidgetItem* current = d->trackView->currentItem() ) {
        setCheckState( current, Qt::Checked );
    }
}


void K3b::VideoCdView::slotUncheck()
{
    if( QTreeWidgetItem* current = d->trackView->currentItem() ) {
        setCheckState( current, Qt::Unchecked );
    }
}


void K3b::VideoCdView::enableInteraction( bool b )
{
    actionCollection()->action( "start_rip" )->setEnabled( b );
}


void K3b::VideoCdView::buildTree( QTreeWidgetItem* parentItem, const QDomElement& parentElement, const QString& pname )
{
    VideoTrackViewItem * thisItem = 0;
    QDomNode node = parentElement.firstChild();

    while ( !node.isNull() ) {
        if ( (node.isElement() && node.nodeName() == "folder") || node.nodeName() == "file" ) {
            if ( parentItem == 0 )
                thisItem = new VideoTrackViewItem( d->trackView, thisItem );
            else
                thisItem = new VideoTrackViewItem( parentItem, thisItem );

            QString txt = node.firstChild().toElement().text();
            thisItem->setText( 0, txt);
            if ( node.nodeName() == "folder" ) {
                buildTree( thisItem, node.toElement(), pname + '_' + txt.toLower() );
            }
            else {
                thisItem->setText( 1, pname + '_' + txt.toLower() );
                buildTree( thisItem, node.toElement(), pname );
            }
        } else if ( (node.isElement() && node.nodeName() == "segment-item") || node.nodeName() == "sequence-item" ) {
            if ( parentItem == 0 )
                thisItem = new VideoTrackViewItem( d->trackView, thisItem );
            else
                thisItem = new VideoTrackViewItem( parentItem, thisItem );

            thisItem->setText( 0, node.toElement().attribute( "src" ) );

            buildTree( thisItem, node.toElement() );
        }

        node = node.nextSibling();
    }
}


