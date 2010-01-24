/*
*
* Copyright (C) 2003 Christian Kvasny <chris@k3b.org>
* Copyright (C) 2008 Sebastian Trueg <trueg@k3b.org>
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

// k3b includes
#include "k3bvideocdview.h"
#include "k3bvideocdrippingdialog.h"
#include "k3bvideocdinfo.h"

#include "k3bappdevicemanager.h"
#include "k3bapplication.h"
#include "k3bdevice.h"
#include "k3bmsf.h"
#include "k3btoc.h"
#include "k3bcore.h"
#include "k3blistview.h"
#include "k3bmedium.h"
#include "k3bstdguiitems.h"
#include "k3baction.h"


// kde includes
#include <KAction>
#include <KActionCollection>
#include <KActionMenu>
#include <KDebug>
#include <KDialog>
#include <KLocale>
#include <KMenu>
#include <KMessageBox>
#include <KStandardDirs>
#include <KStandardAction>
#include <KToolBar>
#include <KToolBarSpacerAction>

// qt includes
#include <Q3Header>
#include <QApplication>
#include <QCursor>
#include <QDomElement>
#include <QFont>
#include <QLabel>
#include <QList>
#include <QStyle>
#include <QVBoxLayout>

namespace {

    class VideoTrackViewItem : public Q3ListViewItem
    {
    public:
        VideoTrackViewItem( Q3ListViewItem* parent, Q3ListViewItem* after )
            : Q3ListViewItem( parent, after )
        {
            setSelectable( false );
        }

        VideoTrackViewItem( Q3ListView* parent, Q3ListViewItem* after )
            : Q3ListViewItem( parent, after )
        {
            setSelectable( false );
        }

        VideoTrackViewItem( Q3ListViewItem* parent,
                            const QString& name,
                            const QString& id,
                            int _trackNumber,
                            const K3b::Msf& length )
            : Q3ListViewItem( parent )
        {
            Q_UNUSED( name );
            setText( 0, QString( "%1. %2" ).arg( _trackNumber ).arg( id ) );
            setText( 1,"" );
            if ( length > 0 ) {
                setText( 2, length.toString() );
                setText( 3, KIO::convertSize( length.mode2Form2Bytes() ) );
            }

            trackNumber = _trackNumber;
            setSelectable( false );
        }

        int trackNumber;

        void updateData( const K3b::VideoCdInfoResultEntry& resultEntry )
        {
            setText( 0, QString( "%1. %2" ).arg( trackNumber ).arg( resultEntry.id ) );
            setText( 1, resultEntry.name );
        }

    };

    class VideoTrackViewCheckItem : public Q3CheckListItem
    {
    public:
        VideoTrackViewCheckItem( Q3ListViewItem* parent,
                                const QString& desc )
            : Q3CheckListItem( parent,
                            QString(),
                            Q3CheckListItem::CheckBox )
        {
            setText( 0, desc );

            setOn( true );
        }

        VideoTrackViewCheckItem( Q3ListView* parent,
                                const QString& desc )
            : Q3CheckListItem( parent,
                            QString(),
                            Q3CheckListItem::CheckBox )
        {
            setText( 0, desc );

            setOn( true );
        }

        VideoTrackViewCheckItem( VideoTrackViewCheckItem* parent,
                                const QString& desc )
            : Q3CheckListItem( parent,
                            QString(),
                            Q3CheckListItem::CheckBox )
        {
            setText( 0, desc );

            setOn( true );
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

    ListView* trackView;
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
    d->trackView = new K3b::ListView( mainWidget() );
    d->trackView->setFullWidth( true );
    d->trackView->setAllColumnsShowFocus( true );
    d->trackView->setSelectionMode( Q3ListView::Single );
    d->trackView->setDragEnabled( true );
    d->trackView->addColumn( i18n( "Item Name" ) );
    d->trackView->addColumn( i18n( "Extracted Name" ) );
    d->trackView->addColumn( i18n( "Length" ) );
    d->trackView->addColumn( i18n( "Size" ) );

    d->trackView->header() ->setClickEnabled( false );

    d->trackView->setItemsRenameable( false );
    d->trackView->setRootIsDecorated( true );

    connect( d->trackView, SIGNAL( contextMenu( K3ListView*, Q3ListViewItem*, const QPoint& ) ),
             this, SLOT( slotContextMenu( K3ListView*, Q3ListViewItem*, const QPoint& ) ) );
    connect( d->trackView, SIGNAL( selectionChanged( Q3ListViewItem* ) ),
             this, SLOT( slotTrackSelectionChanged( Q3ListViewItem* ) ) );
    connect( d->trackView, SIGNAL( clicked( Q3ListViewItem* ) ),
             this, SLOT( slotStateChanged( Q3ListViewItem* ) ) );
    connect( d->trackView, SIGNAL( spacePressed( Q3ListViewItem* ) ),
             this, SLOT( slotStateChanged( Q3ListViewItem* ) ) );

    QVBoxLayout * mainGrid = new QVBoxLayout( mainWidget() );
    mainGrid->addWidget( d->toolBox );
    mainGrid->addWidget( d->trackView );
    mainGrid->setSpacing( 0 );
    mainGrid->setMargin( 0 );

    initActions();

    // setup the toolbox
    d->toolBox->addAction( d->actionCollection->action( "start_rip" ) );
    d->toolBox->addSeparator();
    d->toolBox->addAction( d->actionCollection->action( "view_files" ) );
    d->toolBox->addAction( new KToolBarSpacerAction( d->toolBox ) );
    d->toolBox->addWidget( d->labelLength );
    
    slotTrackSelectionChanged( 0 );

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

    ( ( VideoTrackViewCheckItem* ) d->contentList[ 0 ] ) ->setOpen( true );

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
            ( void ) new VideoTrackViewItem( ( VideoTrackViewCheckItem* ) d->contentList[ 0 ], i18n( "Sequence-%1" , index ), "", index, length );
        } else {
            K3b::Msf length( ( *it ).length() );
            d->videocddatasize += length.mode2Form1Bytes();
            ( ( VideoTrackViewCheckItem* ) d->contentList[ 1 ] ) ->updateData( length );
            ( void ) new VideoTrackViewCheckItem( ( VideoTrackViewCheckItem* ) d->contentList[ 1 ], i18n( "Files" ) );
            ( void ) new VideoTrackViewCheckItem( ( VideoTrackViewCheckItem* ) d->contentList[ 1 ], i18n( "Segments" ) );
        }

        index++;
    }

    ( ( VideoTrackViewCheckItem* ) d->contentList[ 0 ] ) ->updateData( sequenceSize, true );

    d->videooptions ->setVideoCdSource( device()->blockDeviceName() );

    d->videocdinfo = new K3b::VideoCdInfo( this );
    d->videocdinfo->info( device()->blockDeviceName() );

    connect( d->videocdinfo, SIGNAL( infoFinished( bool ) ),
             this, SLOT( slotVideoCdInfoFinished( bool ) ) );

}

void K3b::VideoCdView::slotVideoCdInfoFinished( bool success )
{
    if ( success ) {
        d->videocdinfoResult = d->videocdinfo->result();
        updateDisplay();
    }

    d->trackView->setEnabled( true );
    d->toolBox->setEnabled( true );
    QApplication::restoreOverrideCursor();

}

void K3b::VideoCdView::updateDisplay()
{
    // update the listview

    VideoTrackViewItem * item = ( VideoTrackViewItem* ) d->contentList[ 0 ] ->firstChild();
    int index = 0;
    while ( item ) {
        item->updateData( d->videocdinfoResult.entry( index, K3b::VideoCdInfoResult::SEQUENCE ) );
        item = ( VideoTrackViewItem* ) item->nextSibling();
        index++;
    }

    VideoTrackViewCheckItem* check_item = ( VideoTrackViewCheckItem* ) d->contentList[ 1 ] ->firstChild();
    while ( check_item ) {
        if ( check_item->key( 0, false ).compare( i18n( "Files" ) ) == 0 ) {
            if ( d->domTree.setContent( d->videocdinfoResult.xmlData ) ) {

                QDomElement root = d->domTree.documentElement();
                QDomNode node;
                node = root.firstChild();
                while ( !node.isNull() ) {
                    if ( node.isElement() && node.nodeName() == "filesystem" ) {
                        QDomElement body = node.toElement();
                        buildTree( check_item, body );
                        break;
                    }
                    node = node.nextSibling();
                }
            }
        } else {
            for ( index = 0; index < d->videocdinfoResult.foundEntries( K3b::VideoCdInfoResult::SEGMENT ); index++ ) {
                ( void ) new VideoTrackViewItem( check_item, d->videocdinfoResult.entry( index, K3b::VideoCdInfoResult::SEGMENT ).name, d->videocdinfoResult.entry( index, K3b::VideoCdInfoResult::SEGMENT ).id , index + 1, 0 );
            }
        }
        check_item = ( VideoTrackViewCheckItem* ) check_item->nextSibling();
    }

    if ( !d->videocdinfoResult.volumeId.isEmpty() ) {
        QString description = d->videocdinfoResult.volumeId + " (" + d->videocdinfoResult.type + " " + d->videocdinfoResult.version + ")" ;
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
    
    K3b::createAction( this, i18n( "Check All" ), 0, 0, this,
                       SLOT( slotCheckAll() ), actionCollection(),
                       "check_all" );
    K3b::createAction( this, i18n( "Uncheck All" ), 0, 0, this,
                       SLOT( slotUncheckAll() ), actionCollection(),
                       "decheck_all" );
    K3b::createAction( this, i18n( "Check Track" ), 0, 0, this,
                       SLOT( slotCheck() ), actionCollection(),
                       "check_track" );
    K3b::createAction( this, i18n( "Uncheck Track" ), 0, 0, this,
                       SLOT( slotUncheck() ), actionCollection(),
                       "decheck_track" );
    K3b::createAction( this, i18n( "Start Ripping" ), "tools-rip-video-cd", 0, this,
                       SLOT( startRip() ), actionCollection(),
                       "start_rip" );
    KAction* actionShowDataPart = K3b::createAction( this, i18n("View Files"), "media-optical-data", 0, this,
                                                     SLOT(slotViewFiles()), actionCollection(), "view_files" );
    actionShowDataPart->setToolTip( i18n("View plain data files") );

    // TODO: set the actions tooltips and whatsthis infos

    // setup the popup menu
    d->popupMenu = new KActionMenu( actionCollection() );
    d->popupMenu->addAction( d->actionCollection->action( "check_track" ) );
    d->popupMenu->addAction( d->actionCollection->action( "decheck_track" ) );
    d->popupMenu->addAction( d->actionCollection->action( "check_all" ) );
    d->popupMenu->addAction( d->actionCollection->action( "decheck_all" ) );
    d->popupMenu->addSeparator();
    d->popupMenu->addAction( d->actionCollection->action( "start_rip" ) );
}


void K3b::VideoCdView::slotContextMenu( K3ListView*, Q3ListViewItem*, const QPoint& p )
{
    d->popupMenu->menu()->popup( p );
}


void K3b::VideoCdView::slotTrackSelectionChanged( Q3ListViewItem* item )
{
    actionCollection() ->action( "check_track" ) ->setEnabled( item != 0 );
    actionCollection() ->action( "decheck_track" ) ->setEnabled( item != 0 );
}

void K3b::VideoCdView::slotStateChanged( Q3ListViewItem* item )
{
    /* > QT 3.1
       if ( !item == 0 && item ->isSelectable() ) {
       if ( ( ( VideoTrackViewCheckItem* ) item) ->state() == QCheckListItem::On)
       slotCheck();
       else if ( ( ( VideoTrackViewCheckItem* ) item) ->state() == QCheckListItem::Off)
       slotUncheck();
       }
    */
    if ( !item == 0 && item ->isSelectable() ) {
        if ( ( ( VideoTrackViewCheckItem* ) item) ->isOn() )
            slotCheck();
        else
            slotUncheck();
    }
}

void K3b::VideoCdView::startRip()
{

    int selectedItems  = 0;
    for ( Q3ListViewItemIterator it( d->trackView ); it.current(); ++it ) {
        if ( it.current() ->isSelectable() ) {
            if ( ( ( ( VideoTrackViewCheckItem* ) it.current()) ->key( 0, false ).compare( i18n("Video CD MPEG tracks" ) ) == 0 ) && ( ( VideoTrackViewCheckItem* ) it.current() ) ->isOn() ) {
                d->videooptions ->setVideoCdRipSequences( true );
                selectedItems++;
            }
            else if ( ( ( ( VideoTrackViewCheckItem* ) it.current()) ->key( 0, false ).compare( i18n("Files" ) ) == 0 ) && ( ( VideoTrackViewCheckItem* ) it.current() ) ->isOn() ) {
                d->videooptions ->setVideoCdRipFiles( true );
                selectedItems++;
            }
            else if ( ( ( ( VideoTrackViewCheckItem* ) it.current()) ->key( 0, false ).compare( i18n("Segments" ) ) == 0 ) && ( ( VideoTrackViewCheckItem* ) it.current() ) ->isOn() ) {
                d->videooptions ->setVideoCdRipSegments( true );
                selectedItems++;
            }
        }
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

        kDebug() << QString("(K3b::VideoCdView::startRip())  d->videooptions ->setVideoCdSize( %1)").arg( videocdsize );
        d->videooptions ->setVideoCdSize( videocdsize );
        K3b::VideoCdRippingDialog rip( d->videooptions, this );
        rip.exec();
    }
}

void K3b::VideoCdView::slotCheckAll()
{
    for ( Q3ListViewItemIterator it( d->trackView ); it.current(); ++it )
        if ( it.current() ->isSelectable() )
            ( ( VideoTrackViewCheckItem* ) it.current() ) ->setOn( true );
}

void K3b::VideoCdView::slotUncheckAll()
{
    for ( Q3ListViewItemIterator it( d->trackView ); it.current(); ++it )
        if ( it.current() ->isSelectable() )
            ( ( VideoTrackViewCheckItem* ) it.current() ) ->setOn( false );
}

void K3b::VideoCdView::slotViewFiles()
{
    k3bappcore->appDeviceManager()->mountDisk( device() );
}

void K3b::VideoCdView::slotCheck()
{
    if ( Q3ListViewItem * sel = d->trackView->selectedItem() ) {
        ( ( VideoTrackViewCheckItem* ) sel) ->setOn( true );
        Q3ListViewItem * item = sel ->firstChild();
        while ( item ) {
            if ( item ->isSelectable() )
                ( ( VideoTrackViewCheckItem* ) item) ->setOn( true );

            item = item->nextSibling();
        }
    }
}

void K3b::VideoCdView::slotUncheck()
{
    if ( Q3ListViewItem * sel = d->trackView->selectedItem() ) {
        ( ( VideoTrackViewCheckItem* ) sel) ->setOn( false );
        Q3ListViewItem * item = sel ->firstChild();
        while ( item ) {
            if ( item ->isSelectable() )
                ( ( VideoTrackViewCheckItem* ) item) ->setOn( false );

            item = item->nextSibling();
        }
    }
}

void K3b::VideoCdView::enableInteraction( bool b )
{
    actionCollection()->action( "start_rip" )->setEnabled( b );
}

void K3b::VideoCdView::buildTree( Q3ListViewItem *parentItem, const QDomElement &parentElement, const QString& pname )
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
                buildTree( thisItem, node.toElement(), pname + "_" + txt.toLower() );
            }
            else {
                thisItem->setText( 1, pname + "_" + txt.toLower() );
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

#include "k3bvideocdview.moc"
